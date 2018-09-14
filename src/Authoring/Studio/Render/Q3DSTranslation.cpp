/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "Q3DSTranslation.h"
#include "Q3DStudioRenderer.h"
#include "Q3DSStringTable.h"
#include "Q3DSGraphObjectTranslator.h"

#include "StudioApp.h"
#include "Core.h"
#include "ClientDataModelBridge.h"
#include "StudioFullSystem.h"
#include "IDocumentReader.h"
#include "StudioProjectSettings.h"
#include "SlideSystem.h"

#include <QtCore/qmath.h>

namespace Q3DStudio
{

struct Q3DSEnumStrMap
{
    template <typename T>
    static QString toString(T e)
    {
        return enumMap()[e];
    }
private:
#define INSERT_MAP(enumval) \
    m_map.insert(enumval, #enumval);

    template <typename T>
    struct EnumMap
    {
        QMap<T, QString> m_map;
    };

    struct DataModelDataType : public EnumMap<qt3dsdm::DataModelDataType::Value>
    {
        DataModelDataType()
        {
            INSERT_MAP(qt3dsdm::DataModelDataType::None)
            INSERT_MAP(qt3dsdm::DataModelDataType::Float)
            INSERT_MAP(qt3dsdm::DataModelDataType::Float2)
            INSERT_MAP(qt3dsdm::DataModelDataType::Float3)
            INSERT_MAP(qt3dsdm::DataModelDataType::Long)
            INSERT_MAP(qt3dsdm::DataModelDataType::String)
            INSERT_MAP(qt3dsdm::DataModelDataType::Bool)
            INSERT_MAP(qt3dsdm::DataModelDataType::Long4)
            INSERT_MAP(qt3dsdm::DataModelDataType::StringRef)
            INSERT_MAP(qt3dsdm::DataModelDataType::ObjectRef)
            INSERT_MAP(qt3dsdm::DataModelDataType::StringOrInt)
            INSERT_MAP(qt3dsdm::DataModelDataType::FloatList)
        }
    };
    static DataModelDataType s_dataModelDataType;

    static QMap<qt3dsdm::DataModelDataType::Value, QString> &enumMap()
    {
        return s_dataModelDataType.m_map;
    }
};

Q3DSEnumStrMap::DataModelDataType Q3DSEnumStrMap::s_dataModelDataType;

struct Q3DSValueParser
{
    static QVector3D parseVector(const SValue &value)
    {
        return qt3dsdm::get<QVector3D>(value);
    }

    static QVector3D parseRotationVector(const SValue &value)
    {
        QVector3D rotValue = parseVector(value);
        return QVector3D(rotValue.x(),
                         rotValue.y(),
                         rotValue.z());
    }

    static QColor parseColor(const SValue &value)
    {
        QVector3D vec = parseVector(value);
        QColor c;
        c.setRgbF(qreal(vec.x()), qreal(vec.y()), qreal(vec.z()));
        return c;
    }

    template <typename TEnum>
    static TEnum parseEnum(const SValue &value)
    {
        QString val = value.toQVariant().toString();
        TEnum ret;
        Q3DSEnumMap::enumFromStr(QStringRef(&val), &ret);
        return ret;
    }

    static Q3DSImage *parseImage(Q3DSTranslation *context, const SValue &value)
    {
        qt3dsdm::SLong4 guid = value.getData<qt3dsdm::SLong4>();
        qt3dsdm::Qt3DSDMInstanceHandle theInstance(
            context->reader().GetInstanceForGuid(guid));
        Q3DSGraphObjectTranslator *imageTranslator = context->getOrCreateTranslator(theInstance);
        if (imageTranslator && imageTranslator->graphObject().type() == Q3DSNode::Image) {
            Q3DSImage *theNewImage = static_cast<Q3DSImage *>(&imageTranslator->graphObject());
            return  theNewImage;
        }
        return nullptr;
    }

    static Q3DSGraphObject *parseObjectRef(Q3DSTranslation *context,
                                           qt3dsdm::Qt3DSDMInstanceHandle instance,
                                           const SValue &value)
    {
        qt3dsdm::SObjectRefType objref = value.getData<qt3dsdm::SObjectRefType>();
        qt3dsdm::Qt3DSDMInstanceHandle objinstance(
            context->reader().GetInstanceForObjectRef(instance, objref));
        Q3DSGraphObjectTranslator *translator = context->getOrCreateTranslator(objinstance);
        if (translator)
            return &translator->graphObject();
        return nullptr;
    }
};

struct Q3DSTranslatorDataModelParser
{
    Q3DSTranslation &m_context;
    Qt3DSDMInstanceHandle m_instanceHandle;
    Q3DSTranslatorDataModelParser(Q3DSTranslation &inContext, Qt3DSDMInstanceHandle inInstance)
        : m_context(inContext)
        , m_instanceHandle(inInstance)
    {
    }

    Qt3DSDMInstanceHandle instanceHandle() const
    {
        return m_instanceHandle;
    }

    template <typename TDataType>
    inline Option<TDataType> propertyValue(qt3dsdm::Qt3DSDMPropertyHandle inProperty) const
    {
        Option<SValue> theValue =
            m_context.reader().GetRawInstancePropertyValue(instanceHandle(), inProperty);
        if (theValue.hasValue())
            return qt3dsdm::get<TDataType>(*theValue);
        return Option<TDataType>();
    }

    bool parseProperty(Qt3DSDMPropertyHandle inProperty, float &outValue) const
    {
        Option<float> theValue = propertyValue<float>(inProperty);
        if (theValue.hasValue()) {
            outValue = theValue.getValue();
            return true;
        }
        return false;
    }

    bool parseProperty(Qt3DSDMPropertyHandle inProperty, unsigned int &outValue) const
    {
        Option<int> theValue = propertyValue<int>(inProperty);
        if (theValue.hasValue()) {
            outValue = qMax(unsigned(theValue.getValue()), 0u);
            return true;
        }
        return false;
    }

    bool parseProperty(Qt3DSDMPropertyHandle inProperty, int &outValue) const
    {
        auto theValue = propertyValue<int>(inProperty);
        if (theValue.hasValue()) {
            outValue = *theValue;
            return true;
        }
        return false;
    }

    bool parseRotationOrder(Qt3DSDMPropertyHandle inProperty,
                            Q3DSNode::RotationOrder &outValue) const
    {
        QString temp;
        if (parseProperty(inProperty, temp)) {
            Q3DSEnumMap::enumFromStr(QStringRef(&temp), &outValue);
            return true;
        }
        return false;
    }

    bool parseOrientation(Qt3DSDMPropertyHandle inProperty, Q3DSNode::Orientation &outValue) const
    {
        QString temp;
        if (parseProperty(inProperty, temp)) {
            Q3DSEnumMap::enumFromStr(QStringRef(&temp), &outValue);
            return true;
        }
        return false;
    }

    bool parseProperty(Qt3DSDMPropertyHandle inProperty, bool &outValue) const
    {
        Option<bool> theValue = propertyValue<bool>(inProperty);
        if (theValue.hasValue()) {
            outValue = theValue.getValue();
            return true;
        }
        return false;
    }

    bool parseProperty(Qt3DSDMPropertyHandle inProperty, QVector2D &outValue) const
    {
        Option<qt3dsdm::SFloat2> theValue = propertyValue<qt3dsdm::SFloat2>(inProperty);
        if (theValue.hasValue()) {
            outValue = QVector2D(theValue->m_Floats[0], theValue->m_Floats[1]);
            return true;
        }
        return false;
    }

    bool parseProperty(Qt3DSDMPropertyHandle inProperty, QVector3D &outValue) const
    {
        Option<qt3dsdm::SFloat3> theValue = propertyValue<qt3dsdm::SFloat3>(inProperty);
        if (theValue.hasValue()) {
            outValue = QVector3D(theValue->m_Floats[0], theValue->m_Floats[1],
                                 theValue->m_Floats[2]);
            return true;
        }
        return false;
    }

    bool parseProperty(Qt3DSDMPropertyHandle inProperty, QString &outValue) const
    {
        Option<qt3dsdm::TDataStrPtr> theValue = propertyValue<qt3dsdm::TDataStrPtr>(inProperty);
        if (theValue.hasValue() && *theValue) {
            std::shared_ptr<Q3DSStringTable> strTable = Q3DSStringTable::instance();
            outValue = strTable->GetRenderStringTable().RegisterStr((*theValue)->GetData());
            return true;
        }
        return false;
    }

    bool parseColorProperty(Qt3DSDMPropertyHandle inProperty, QColor &outValue) const
    {
        Option<qt3dsdm::SFloat3> theValue = propertyValue<qt3dsdm::SFloat3>(inProperty);
        if (theValue.hasValue()) {
            outValue.setRgbF(qreal(theValue.getValue().m_Floats[0]),
                             qreal(theValue.getValue().m_Floats[1]),
                             qreal(theValue.getValue().m_Floats[2]));
            return true;
        }
        return false;
    }

    bool parseAndResolveSourcePath(qt3dsdm::Qt3DSDMPropertyHandle inProperty,
                                   QString &outValue) const
    {
        if (parseProperty(inProperty, outValue)) {
            if (!outValue.isEmpty() && outValue[0] != QLatin1Char('#')) {
                Q3DStudio::CFilePath theDirectory
                        = g_StudioApp.GetCore()->GetDoc()->GetDocumentDirectory();
                Q3DStudio::CFilePath theResolvedPath =
                    Q3DStudio::CFilePath::CombineBaseAndRelative(theDirectory,
                                                        Q3DStudio::CString::fromQString(outValue));
                if (theResolvedPath.exists()) {
                    std::shared_ptr<Q3DSStringTable> strTable = Q3DSStringTable::instance();
                    outValue = strTable->GetRenderStringTable().RegisterStr(outValue);
                }
            }
            return true;
        }
        return false;
    }

    template <typename TEnumType>
    bool parseEnumProperty(qt3dsdm::Qt3DSDMPropertyHandle inProperty, TEnumType &ioValue) const
    {
        QString temp;
        if (parseProperty(inProperty, temp)) {
            Q3DSEnumMap::enumFromStr(QStringRef(&temp), &ioValue);
            return true;
        }
        return false;
    }

    bool parsePlaythroughToProperty(qt3dsdm::Qt3DSDMPropertyHandle inProperty,
                                    Q3DSSlide::PlayThrough &playthrough, QVariant &ioValue) const
    {
        Option<qt3dsdm::SStringOrInt> temp = propertyValue<qt3dsdm::SStringOrInt>(inProperty);
        if (temp.hasValue()) {
            const bool isInt = temp->GetType() == qt3dsdm::SStringOrIntTypes::Int;
            QString str = !isInt ? QString::fromWCharArray(
                                       temp->m_Value.getData<qt3dsdm::TDataStrPtr>()->GetData())
                                 : QString("");
            const bool isRef = (!isInt && str[0] == QLatin1Char('#'));
            if (isInt || isRef) {
                playthrough = Q3DSSlide::Value;
                ioValue = isRef ? QVariant::fromValue(str)
                                : QVariant::fromValue(temp->m_Value.getData<long>());
            } else {
                Q3DSEnumMap::enumFromStr(QStringRef(&str), &playthrough);
            }
            return true;
        }
        return false;
    }

    bool parseProperty(qt3dsdm::Qt3DSDMPropertyHandle inProperty, Q3DSImage *&ioImage) const
    {
        Option<qt3dsdm::SLong4> theData = propertyValue<qt3dsdm::SLong4>(inProperty);
        if (theData.hasValue()) {
            qt3dsdm::Qt3DSDMInstanceHandle theInstance(
                m_context.reader().GetInstanceForGuid(*theData));
            Q3DSGraphObjectTranslator *imageTranslator
                    = m_context.getOrCreateTranslator(theInstance);
            if (imageTranslator && imageTranslator->graphObject().type() == Q3DSNode::Image) {
                Q3DSImage *theNewImage
                        = static_cast<Q3DSImage *>(&imageTranslator->graphObject());
                ioImage = theNewImage;
            } else
                ioImage = nullptr;
            return true;
        }
        return false;
    }

    bool parseProperty(Qt3DSDMPropertyHandle inProperty, Q3DSGraphObject *&ioObjRef) const
    {
        Option<qt3dsdm::SObjectRefType> theData
                = propertyValue<qt3dsdm::SObjectRefType>(inProperty);
        ioObjRef = nullptr;
        if (theData.hasValue()) {
            qt3dsdm::Qt3DSDMInstanceHandle theInstance(
                m_context.reader().GetInstanceForObjectRef(m_instanceHandle, *theData));
            Q3DSGraphObjectTranslator *theItemTranslator =
                m_context.getOrCreateTranslator(theInstance);
            if (theItemTranslator)
                ioObjRef = &theItemTranslator->graphObject();
        }
        return true;
    }
};


#define HANDLE_PREFIX(temp_type) \
    { \
        temp_type temp;
#define HANDLE_SUFFIX(name) \
        changeList.append(theItem.set##name(temp)); \
    }
#define HANDLE_PROPERTY(type, name) \
    inContext.objectDefinitions().m_##type.m_##name
#define HANDLE_PARAMS(type, name) \
    HANDLE_PROPERTY(type, name), temp
#define HANDLE_Q3DS_PROPERTY(rtype, type, name, func) \
    HANDLE_PREFIX(rtype) theParser.func(HANDLE_PARAMS(type, name)); HANDLE_SUFFIX(name)
#define HANDLE_Q3DS_PROPERTY2(rtype, type, name, qname, func) \
    HANDLE_PREFIX(rtype) theParser.func(HANDLE_PARAMS(type, name)); HANDLE_SUFFIX(qname)

#define HANDLE_Q3DS_VEC3_PROPERTY(type, name) \
    HANDLE_Q3DS_PROPERTY(QVector3D, type, name, parseProperty)
#define HANDLE_Q3DS_OPACITY_PROPERTY(type, name) \
    HANDLE_Q3DS_PROPERTY2(float, type, name, LocalOpacity, parseOpacityProperty)
#define HANDLE_Q3DS_ROTATION_ORDER_PROPERTY(type, name) \
    HANDLE_Q3DS_PROPERTY(Q3DSNode::RotationOrder, type, name, parseRotationOrder)
#define HANDLE_Q3DS_NODE_ORIENTATION_PROPERTY(type, name) \
    HANDLE_Q3DS_PROPERTY(Q3DSNode::Orientation, type, name, parseOrientation)
#define HANDLE_Q3DS_BOOL_PROPERTY(type, name) \
    HANDLE_Q3DS_PROPERTY(bool, type, name, parseProperty)
#define HANDLE_Q3DS_BOOL_PROPERTY2(type, name, qname) \
    HANDLE_Q3DS_PROPERTY2(bool, type, name, qname, parseProperty)
#define HANDLE_Q3DS_INT_PROPERTY(type, name) \
    HANDLE_Q3DS_PROPERTY(int, type, name, parseProperty)
#define HANDLE_Q3DS_INT_PROPERTY2(type, name, qname) \
    HANDLE_Q3DS_PROPERTY2(int, type, name, qname, parseProperty)
#define HANDLE_Q3DS_COLOR_PROPERTY(type, name) \
    HANDLE_Q3DS_PROPERTY(QColor, type, name, parseColorProperty)
#define HANDLE_Q3DS_COLOR_PROPERTY2(type, name, qname) \
    HANDLE_Q3DS_PROPERTY2(QColor, type, name, qname, parseColorProperty)
#define HANDLE_Q3DS_NAME_PROPERTY \
    HANDLE_Q3DS_PROPERTY2(QString, Named, NameProp, Name, parseProperty)
#define HANDLE_Q3DS_ENUM_PROPERTY(type, name, etype) \
    HANDLE_Q3DS_PROPERTY(etype, type, name, parseEnumProperty)
#define HANDLE_Q3DS_ENUM_PROPERTY2(type, name, qname, etype) \
    HANDLE_Q3DS_PROPERTY2(etype, type, name, qname, parseEnumProperty)
#define HANDLE_Q3DS_SOURCEPATH_PROPERTY(type, name) \
    HANDLE_Q3DS_PROPERTY(QString, type, name, parseAndResolveSourcePath)
#define HANDLE_Q3DS_FLOAT_PROPERTY(type, name) \
    HANDLE_Q3DS_PROPERTY(float, type, name, parseProperty)
#define HANDLE_Q3DS_FLOAT_PROPERTY2(type, name, qname) \
    HANDLE_Q3DS_PROPERTY2(float, type, name, qname, parseProperty)
#define HANDLE_Q3DS_IMAGE_PROPERTY(type, name) \
    HANDLE_Q3DS_PROPERTY(Q3DSImage*, type, name, parseProperty)
#define HANDLE_Q3DS_IMAGE_PROPERTY2(type, name, qname) \
    HANDLE_Q3DS_PROPERTY2(Q3DSImage*, type, name, qname, parseProperty)
#define HANDLE_Q3DS_OBJREF_PROPERTY(type, name) \
    HANDLE_Q3DS_PROPERTY(Q3DSGraphObject*, type, name, parseProperty)
#define HANDLE_Q3DS_STRING_PROPERTY(type, name) \
    HANDLE_Q3DS_PROPERTY(QString, type, name, parseProperty)
#define HANDLE_Q3DS_STRING_PROPERTY2(type, name, qname) \
    HANDLE_Q3DS_PROPERTY2(QString, type, name, qname, parseProperty)
#define HANDLE_Q3DS_VEC2_PROPERTY(type, name) \
    HANDLE_Q3DS_PROPERTY2(float, type, name##U, name##U, parseProperty) \
    HANDLE_Q3DS_PROPERTY2(float, type, name##V, name##V, parseProperty)
#define HANDLE_Q3DS_VEC2_PROPERTY2(type, name, qname) \
    HANDLE_Q3DS_PROPERTY2(float, type, name##U, qname##U, parseProperty) \
    HANDLE_Q3DS_PROPERTY2(float, type, name##V, qname##V, parseProperty)

/* These are not needed for Scene, Material, CustomMaterial, Image, RenderPlugin */
#define HANDLE_Q3DS_START_END_TIMES \
    HANDLE_Q3DS_PROPERTY(qint32, Asset, StartTime, parseProperty) \
    HANDLE_Q3DS_PROPERTY(qint32, Asset, EndTime, parseProperty) \

#define HANDLE_Q3DS_NOTIFY_CHANGES \
    theItem.notifyPropertyChanges(changeList);

#define ITERATE_Q3DS_SCENE_PROPERTIES                               \
    HANDLE_Q3DS_COLOR_PROPERTY2(Scene, BackgroundColor, ClearColor) \
    HANDLE_Q3DS_BOOL_PROPERTY2(Scene, BgColorEnable, UseClearColor) \
    HANDLE_Q3DS_NAME_PROPERTY   \
    HANDLE_Q3DS_NOTIFY_CHANGES

#define ITERATE_Q3DS_NODE_PROPERTIES                              \
    HANDLE_Q3DS_VEC3_PROPERTY(Node, Rotation)                     \
    HANDLE_Q3DS_VEC3_PROPERTY(Node, Position)                     \
    HANDLE_Q3DS_VEC3_PROPERTY(Node, Scale)                        \
    HANDLE_Q3DS_VEC3_PROPERTY(Node, Pivot)                        \
    HANDLE_Q3DS_FLOAT_PROPERTY2(Node, Opacity, LocalOpacity)      \
    HANDLE_Q3DS_ROTATION_ORDER_PROPERTY(Node, RotationOrder)      \
    HANDLE_Q3DS_NODE_ORIENTATION_PROPERTY(Node, Orientation)      \
    HANDLE_Q3DS_BOOL_PROPERTY(Node, IgnoresParent)                \
    HANDLE_Q3DS_INT_PROPERTY2(Node, BoneId, SkeletonId)           \
    HANDLE_Q3DS_START_END_TIMES \
    HANDLE_Q3DS_NAME_PROPERTY   \
    HANDLE_Q3DS_NOTIFY_CHANGES

#define ITERATE_Q3DS_LAYER_PROPERTIES                                                   \
    HANDLE_Q3DS_BOOL_PROPERTY2(Layer, DisableDepthTest, DepthTestDisabled)              \
    HANDLE_Q3DS_BOOL_PROPERTY2(Layer, DisableDepthPrepass, DepthPrePassDisabled)        \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, ProgressiveAA, Q3DSLayerNode::ProgressiveAA)       \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, MultisampleAA, Q3DSLayerNode::MultisampleAA)       \
    HANDLE_Q3DS_BOOL_PROPERTY2(Layer, TemporalAA, TemporalAAEnabled)                    \
    HANDLE_Q3DS_COLOR_PROPERTY(Layer, BackgroundColor)                                  \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, BlendType, Q3DSLayerNode::BlendType)               \
    HANDLE_Q3DS_ENUM_PROPERTY2(Layer, Background, LayerBackground,                      \
        Q3DSLayerNode::LayerBackground)                                                 \
    HANDLE_Q3DS_SOURCEPATH_PROPERTY(Asset, SourcePath)                                  \
    HANDLE_Q3DS_ENUM_PROPERTY2(Layer, HorizontalFieldValues, HorizontalFields,          \
        Q3DSLayerNode::HorizontalFields)                                                \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Left)                                             \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, LeftUnits, Q3DSLayerNode::Units)                   \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Width)                                            \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, WidthUnits, Q3DSLayerNode::Units)                  \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Right)                                            \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, RightUnits, Q3DSLayerNode::Units)                  \
    HANDLE_Q3DS_ENUM_PROPERTY2(Layer, VerticalFieldValues, VerticalFields,              \
        Q3DSLayerNode::VerticalFields)                                                  \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Top)                                              \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, TopUnits, Q3DSLayerNode::Units)                    \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Height)                                           \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, HeightUnits, Q3DSLayerNode::Units)                 \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Bottom)                                           \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, BottomUnits, Q3DSLayerNode::Units)                 \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, AoStrength)                                       \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, AoDistance)                                       \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, AoSoftness)                                       \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, AoBias)                                           \
    HANDLE_Q3DS_BOOL_PROPERTY(Layer, AoDither)                                          \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ShadowStrength)                                   \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ShadowDist)                                       \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ShadowSoftness)                                   \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ShadowBias)                                       \
    HANDLE_Q3DS_IMAGE_PROPERTY(Layer, LightProbe)                                       \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ShadowBias)                                       \
    HANDLE_Q3DS_BOOL_PROPERTY2(Layer, FastIbl, FastIBLEnabled)                          \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ProbeHorizon)                                     \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ProbeFov)                                         \
    HANDLE_Q3DS_IMAGE_PROPERTY(Layer, LightProbe2)                                      \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Probe2Fade)                                       \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Probe2Window)                                     \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Probe2Pos)                                        \
    HANDLE_Q3DS_NOTIFY_CHANGES

#define ITERATE_Q3DS_SLIDE_PROPERTIES \
    HANDLE_Q3DS_ENUM_PROPERTY(Slide, PlayMode, Q3DSSlide::PlayMode)                     \
    HANDLE_Q3DS_ENUM_PROPERTY(Slide, InitialPlayState, Q3DSSlide::InitialPlayState)     \
    HANDLE_Q3DS_START_END_TIMES                                                         \
    HANDLE_Q3DS_NAME_PROPERTY

#define ITERATE_Q3DS_CAMERA_PROPERTIES                                                  \
    HANDLE_Q3DS_FLOAT_PROPERTY(Camera, ClipNear)                                        \
    HANDLE_Q3DS_FLOAT_PROPERTY(Camera, ClipFar)                                         \
    HANDLE_Q3DS_FLOAT_PROPERTY(Camera, Fov)                                             \
    HANDLE_Q3DS_BOOL_PROPERTY(Camera, Orthographic)                                     \
    HANDLE_Q3DS_ENUM_PROPERTY(Camera, ScaleMode, Q3DSCameraNode::ScaleMode)             \
    HANDLE_Q3DS_ENUM_PROPERTY(Camera, ScaleAnchor, Q3DSCameraNode::ScaleAnchor)         \
    HANDLE_Q3DS_START_END_TIMES                                                         \
    HANDLE_Q3DS_NAME_PROPERTY                                                           \
    HANDLE_Q3DS_NOTIFY_CHANGES

#define ITERATE_Q3DS_LIGHT_PROPERTIES                                                   \
    HANDLE_Q3DS_ENUM_PROPERTY(Light, LightType, Q3DSLightNode::LightType)               \
    HANDLE_Q3DS_OBJREF_PROPERTY(Light, Scope)                                           \
    HANDLE_Q3DS_COLOR_PROPERTY2(Light, LightColor, Diffuse)                             \
    HANDLE_Q3DS_COLOR_PROPERTY2(Light, SpecularColor, Specular)                         \
    HANDLE_Q3DS_COLOR_PROPERTY2(Light, AmbientColor, Ambient)                           \
    HANDLE_Q3DS_FLOAT_PROPERTY(Light, Brightness)                                       \
    HANDLE_Q3DS_FLOAT_PROPERTY(Light, LinearFade)                                       \
    HANDLE_Q3DS_FLOAT_PROPERTY(Light, ExpFade)                                          \
    HANDLE_Q3DS_FLOAT_PROPERTY(Light, AreaWidth)                                        \
    HANDLE_Q3DS_FLOAT_PROPERTY(Light, AreaHeight)                                       \
    HANDLE_Q3DS_BOOL_PROPERTY(Light, CastShadow)                                        \
    HANDLE_Q3DS_FLOAT_PROPERTY(Light, ShadowBias)                                       \
    HANDLE_Q3DS_FLOAT_PROPERTY(Light, ShadowFactor)                                     \
    HANDLE_Q3DS_FLOAT_PROPERTY(Light, ShadowMapFar)                                     \
    HANDLE_Q3DS_INT_PROPERTY(Light, ShadowMapRes)                                       \
    HANDLE_Q3DS_FLOAT_PROPERTY(Light, ShadowMapFov)                                     \
    HANDLE_Q3DS_FLOAT_PROPERTY(Light, ShadowFilter)                                     \
    HANDLE_Q3DS_START_END_TIMES                                                         \
    HANDLE_Q3DS_NAME_PROPERTY                                                           \
    HANDLE_Q3DS_NOTIFY_CHANGES

#define ITERATE_Q3DS_MODEL_PROPERTIES                                                   \
    HANDLE_Q3DS_STRING_PROPERTY2(Asset, SourcePath, Mesh)                               \
    HANDLE_Q3DS_ENUM_PROPERTY(Model, Tessellation, Q3DSModelNode::Tessellation)         \
    HANDLE_Q3DS_FLOAT_PROPERTY(Model, EdgeTess)                                         \
    HANDLE_Q3DS_FLOAT_PROPERTY(Model, InnerTess)                                        \
    HANDLE_Q3DS_INT_PROPERTY2(Model, PoseRoot, SkeletonRoot)                            \
    HANDLE_Q3DS_START_END_TIMES                                                         \
    HANDLE_Q3DS_NAME_PROPERTY                                                           \
    HANDLE_Q3DS_NOTIFY_CHANGES

#define ITERATE_Q3DS_DEFAULT_MATERIAL_PROPERTIES                                        \
    HANDLE_Q3DS_ENUM_PROPERTY(Material, ShaderLighting, Q3DSDefaultMaterial::ShaderLighting) \
    HANDLE_Q3DS_ENUM_PROPERTY(Material, BlendMode, Q3DSDefaultMaterial::BlendMode)      \
/*    HANDLE_Q3DS_BOOL_PROPERTY2(Material, VertexColors)                           */   \
    HANDLE_Q3DS_IMAGE_PROPERTY2(MaterialBase, IblProbe, LightProbe)                     \
    HANDLE_Q3DS_COLOR_PROPERTY2(Material, DiffuseColor, Diffuse)                        \
    HANDLE_Q3DS_IMAGE_PROPERTY2(Material, DiffuseMap1, DiffuseMap)                      \
    HANDLE_Q3DS_IMAGE_PROPERTY(Material, DiffuseMap2)                                   \
    HANDLE_Q3DS_IMAGE_PROPERTY(Material, DiffuseMap3)                                   \
    HANDLE_Q3DS_FLOAT_PROPERTY(Material, EmissivePower)                                 \
    HANDLE_Q3DS_COLOR_PROPERTY(Material, EmissiveColor)                                 \
    HANDLE_Q3DS_IMAGE_PROPERTY(Material, EmissiveMap)                                   \
    HANDLE_Q3DS_IMAGE_PROPERTY(Material, EmissiveMap2)                                  \
    HANDLE_Q3DS_IMAGE_PROPERTY(Material, SpecularReflection)                            \
    HANDLE_Q3DS_IMAGE_PROPERTY(Material, SpecularMap)                                   \
    HANDLE_Q3DS_ENUM_PROPERTY(Material, SpecularModel, Q3DSDefaultMaterial::SpecularModel) \
    HANDLE_Q3DS_COLOR_PROPERTY(Material, SpecularTint)                                  \
    HANDLE_Q3DS_FLOAT_PROPERTY(Material, FresnelPower)                                  \
    HANDLE_Q3DS_FLOAT_PROPERTY2(Material, IOR, Ior)                                     \
    HANDLE_Q3DS_FLOAT_PROPERTY(Material, SpecularAmount)                                \
    HANDLE_Q3DS_FLOAT_PROPERTY(Material, SpecularRoughness)                             \
    HANDLE_Q3DS_IMAGE_PROPERTY(Material, RoughnessMap)                                  \
    HANDLE_Q3DS_FLOAT_PROPERTY(Material, Opacity)                                       \
    HANDLE_Q3DS_IMAGE_PROPERTY(Material, OpacityMap)                                    \
    HANDLE_Q3DS_IMAGE_PROPERTY(Material, BumpMap)                                       \
    HANDLE_Q3DS_FLOAT_PROPERTY(Material, BumpAmount)                                    \
    HANDLE_Q3DS_IMAGE_PROPERTY(Material, NormalMap)                                     \
    HANDLE_Q3DS_IMAGE_PROPERTY(Material, DisplacementMap)                               \
    HANDLE_Q3DS_FLOAT_PROPERTY(Material, DisplaceAmount)                                \
    HANDLE_Q3DS_IMAGE_PROPERTY(Material, TranslucencyMap)                               \
    HANDLE_Q3DS_FLOAT_PROPERTY(Material, TranslucentFalloff)                            \
    HANDLE_Q3DS_IMAGE_PROPERTY2(Lightmaps, LightmapIndirect, LightmapIndirectMap)       \
    HANDLE_Q3DS_IMAGE_PROPERTY2(Lightmaps, LightmapRadiosity, LightmapRadiosityMap)     \
    HANDLE_Q3DS_IMAGE_PROPERTY2(Lightmaps, LightmapShadow, LightmapShadowMap)           \
    HANDLE_Q3DS_NAME_PROPERTY                                                           \
    HANDLE_Q3DS_NOTIFY_CHANGES

#define ITERATE_Q3DS_IMAGE_PROPERTIES                                                   \
    HANDLE_Q3DS_STRING_PROPERTY(Asset, SourcePath)                                      \
    HANDLE_Q3DS_STRING_PROPERTY(Image, SubPresentation)                                 \
    HANDLE_Q3DS_VEC2_PROPERTY2(Image, Repeat, Scale)                                    \
    HANDLE_Q3DS_VEC2_PROPERTY(Image, Pivot)                                             \
    HANDLE_Q3DS_FLOAT_PROPERTY(Image, RotationUV)                                       \
    HANDLE_Q3DS_VEC2_PROPERTY(Image, Position)                                          \
    HANDLE_Q3DS_ENUM_PROPERTY2(Image, TextureMapping, MappingMode, Q3DSImage::MappingMode) \
    HANDLE_Q3DS_ENUM_PROPERTY2(Image, TilingU, HorizontalTiling, Q3DSImage::TilingMode) \
    HANDLE_Q3DS_ENUM_PROPERTY2(Image, TilingV, VerticalTiling, Q3DSImage::TilingMode)   \
    HANDLE_Q3DS_NAME_PROPERTY                                                           \
    HANDLE_Q3DS_NOTIFY_CHANGES

#define HANDLE_CHANGE(x)    \
{                           \
    changeList.append(x);   \
    ret = true;             \
}

class Q3DSNodeTranslator : public Q3DSGraphObjectTranslator
{
public:
    Q3DSNodeTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSNode &node)
        : Q3DSGraphObjectTranslator(instance, node)
    {

    }

    void pushTranslation(Q3DSTranslation &inContext) override
    {
        Q3DSGraphObjectTranslator::pushTranslation(inContext);
        Q3DSNode &theItem = static_cast<Q3DSNode &>(graphObject());
        Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
        Q3DSPropertyChangeList changeList;
        ITERATE_Q3DS_NODE_PROPERTIES
        int childCount = inContext.assetGraph().GetChildCount(instanceHandle());
        for (long idx = 0; idx < childCount; ++idx) {
            qt3dsdm::Qt3DSDMInstanceHandle child =
                inContext.assetGraph().GetChild(instanceHandle(), idx);
            Q3DSGraphObjectTranslator *translator = inContext.getOrCreateTranslator(child);
            if (translator && translator->graphObject().isNode()) {
                theItem.appendChildNode(&translator->graphObject());
                translator->pushTranslation(inContext);
            }
        }
        theItem.resolveReferences(*inContext.presentation());
    }

    void appendChild(Q3DSGraphObject &inChild) override
    {
        if (inChild.isNode() == false) {
            QT3DS_ASSERT(false);
            return;
        }

        Q3DSNode &theItem = static_cast<Q3DSNode &>(graphObject());
        Q3DSNode &theChild = static_cast<Q3DSNode &>(inChild);
        theItem.appendChildNode(&theChild);
    }

    void clearChildren() override
    {
        Q3DSNode &theItem = static_cast<Q3DSNode &>(graphObject());
        theItem.removeAllChildNodes();
    }

    void setActive(bool inActive) override
    {
        Q3DSNode &theNode = static_cast<Q3DSNode &>(graphObject());
        if (inActive != theNode.eyeballEnabled()) {
            Q3DSPropertyChangeList changeList;
            changeList.append(theNode.setEyeballEnabled(inActive));
            theNode.notifyPropertyChanges(changeList);
        }
    }

    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value, const QString &name) override
    {
        bool ret = false;
        if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
            return true;

        Q3DSPropertyChangeList changeList;
        Q3DSNode &theItem = static_cast<Q3DSNode &>(graphObject());
        if (name == QLatin1String("position")) {
            HANDLE_CHANGE(theItem.setPosition(Q3DSValueParser::parseVector(value)))
        } else if (name == QLatin1String("rotation")) {
            HANDLE_CHANGE(theItem.setRotation(Q3DSValueParser::parseRotationVector(value)))
        } else if (name == QLatin1String("scale")) {
            HANDLE_CHANGE(theItem.setScale(Q3DSValueParser::parseVector(value)))
        } else if (name == QLatin1String("pivot")) {
            HANDLE_CHANGE(theItem.setPivot(Q3DSValueParser::parseVector(value)))
        } else if (name == QLatin1String("opacity")) {
            HANDLE_CHANGE(theItem.setLocalOpacity(value.getData<float>()))
        } else if (name == QLatin1String("rotationorder")) {
            HANDLE_CHANGE(theItem.setRotationOrder(Q3DSValueParser::parseEnum
                                     <Q3DSNode::RotationOrder>(value)))
        } else if (name == QLatin1String("orientation")) {
            HANDLE_CHANGE(theItem.setOrientation(Q3DSValueParser::parseEnum
                                   <Q3DSNode::Orientation>(value)))
        } else if (name == QLatin1String("boneid")) {
            HANDLE_CHANGE(theItem.setSkeletonId(value.getData<int>()))
        } else if (name == QLatin1String("ignoresparent")) {
            HANDLE_CHANGE(theItem.setIgnoresParent(value.getData<bool>()))
        } else if (name == QLatin1String("eyeball")) {
            HANDLE_CHANGE(theItem.setEyeballEnabled(value.getData<bool>()))
        }
        if (ret)
            theItem.notifyPropertyChanges(changeList);
        return ret;
    }
};

class Q3DSGroupNodeTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSGroupNodeTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSGroupNode &node)
        : Q3DSNodeTranslator(instance, node)
    {
    }
};

class Q3DSComponentNodeTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSComponentNodeTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                Q3DSComponentNode &component)
        : Q3DSNodeTranslator(instance, component)
    {
    }
};

class Q3DSSceneTranslator : public Q3DSGraphObjectTranslator
{
public:
    Q3DSSceneTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSScene &scene)
        : Q3DSGraphObjectTranslator(instance, scene)
    {

    }

    void pushTranslation(Q3DSTranslation &inContext) override
    {
        Q3DSGraphObjectTranslator::pushTranslation(inContext);

        Q3DSScene &theItem = static_cast<Q3DSScene &>(graphObject());
        Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
        Q3DSPropertyChangeList changeList;
        ITERATE_Q3DS_SCENE_PROPERTIES
        theItem.removeAllChildNodes();
        int childCount = inContext.assetGraph().GetChildCount(instanceHandle());
        for (long idx = 0; idx < childCount; ++idx) {
            qt3dsdm::Qt3DSDMInstanceHandle layer =
                inContext.assetGraph().GetChild(instanceHandle(), idx);
            Q3DSGraphObjectTranslator *translator = inContext.getOrCreateTranslator(layer);
            if (translator && translator->graphObject().type() == Q3DSNode::Layer) {
                Q3DSLayerNode *theLayerObj
                        = static_cast<Q3DSLayerNode *>(&translator->graphObject());
                theItem.appendChildNode(theLayerObj);
                translator->pushTranslation(inContext);
            }
        }
        theItem.resolveReferences(*inContext.presentation());
    }

    void appendChild(Q3DSGraphObject &inChild) override
    {
        if (inChild.type() != Q3DSNode::Layer) {
            QT3DS_ASSERT(false);
            return;
        }

        Q3DSGraphObject &theItem = static_cast<Q3DSGraphObject &>(graphObject());
        Q3DSGraphObject &theChild = static_cast<Q3DSGraphObject &>(inChild);
        theItem.appendChildNode(&theChild);
    }

    void clearChildren() override
    {
        Q3DSNode &theItem = static_cast<Q3DSNode &>(graphObject());
        theItem.removeAllChildNodes();
    }
    void setActive(bool) override
    {

    }

    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override
    {
        bool ret = false;
        if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
            return true;

        Q3DSPropertyChangeList changeList;
        Q3DSScene &theItem = static_cast<Q3DSScene &>(graphObject());
        if (name == QLatin1String("bgcolorenable"))
            HANDLE_CHANGE(theItem.setUseClearColor(value.getData<bool>()))
        else if (name == QLatin1String("backgroundcolor"))
            HANDLE_CHANGE(theItem.setClearColor(Q3DSValueParser::parseColor(value)))

        if (ret)
            theItem.notifyPropertyChanges(changeList);
        return ret;
    }
};

class Q3DSCameraTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSCameraTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSCameraNode &camera)
        : Q3DSNodeTranslator(instance, camera)
    {

    }

    void pushTranslation(Q3DSTranslation &inContext) override
    {
        Q3DSNodeTranslator::pushTranslation(inContext);

        Q3DSCameraNode &theItem = static_cast<Q3DSCameraNode &>(graphObject());
        Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
        Q3DSPropertyChangeList changeList;
        ITERATE_Q3DS_CAMERA_PROPERTIES
        theItem.resolveReferences(*inContext.presentation());
    }

    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override
    {
        bool ret = false;
        if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
            return true;

        Q3DSPropertyChangeList changeList;
        Q3DSCameraNode &theItem = static_cast<Q3DSCameraNode &>(graphObject());
        if (name == QLatin1String("orthographic")) {
            HANDLE_CHANGE(theItem.setOrthographic(value.getData<bool>()))
        } else if (name == QLatin1String("clipnear")) {
            HANDLE_CHANGE(theItem.setClipNear(value.getData<float>()))
        } else if (name == QLatin1String("clipfar")) {
            HANDLE_CHANGE(theItem.setClipFar(value.getData<float>()))
        } else if (name == QLatin1String("fov")) {
            HANDLE_CHANGE(theItem.setFov(value.getData<float>()))
        }  else if (name == QLatin1String("scalemode")) {
            HANDLE_CHANGE(theItem.setScaleMode(
                              Q3DSValueParser::parseEnum<Q3DSCameraNode::ScaleMode>(value)))
        } else if (name == QLatin1String("scaleanchor")) {
            HANDLE_CHANGE(theItem.setScaleAnchor(
                              Q3DSValueParser::parseEnum<Q3DSCameraNode::ScaleAnchor>(value)))
        }
        if (ret)
            theItem.notifyPropertyChanges(changeList);
        return ret;
    }
};

class Q3DSLightTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSLightTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSLightNode &light)
        : Q3DSNodeTranslator(instance, light)
    {

    }

    void pushTranslation(Q3DSTranslation &inContext) override
    {
        Q3DSNodeTranslator::pushTranslation(inContext);

        Q3DSLightNode &theItem = static_cast<Q3DSLightNode &>(graphObject());
        Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
        Q3DSPropertyChangeList changeList;
        ITERATE_Q3DS_LIGHT_PROPERTIES
        theItem.resolveReferences(*inContext.presentation());
    }

    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override
    {
        bool ret = false;
        if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
            return true;

        Q3DSPropertyChangeList changeList;
        Q3DSLightNode &theItem = static_cast<Q3DSLightNode &>(graphObject());
        if (name == QLatin1String("scope")) {
            HANDLE_CHANGE(theItem.setScope(Q3DSValueParser::parseObjectRef(&inContext, instance,
                                                                           value)))
            theItem.resolveReferences(*inContext.presentation());
        } else if (name == QLatin1String("lighttype")) {
            HANDLE_CHANGE(theItem.setLightType
                          (Q3DSValueParser::parseEnum<Q3DSLightNode::LightType>(value)))
        } else if (name == QLatin1String("lightdiffuse")) {
            HANDLE_CHANGE(theItem.setDiffuse(Q3DSValueParser::parseColor(value)))
        } else if (name == QLatin1String("lightspecular")) {
            HANDLE_CHANGE(theItem.setSpecular(Q3DSValueParser::parseColor(value)))
        } else if (name == QLatin1String("lightambient")) {
            HANDLE_CHANGE(theItem.setAmbient(Q3DSValueParser::parseColor(value)))
        } else if (name == QLatin1String("brightness")) {
            HANDLE_CHANGE(theItem.setBrightness(value.getData<float>()))
        } else if (name == QLatin1String("linearfade")) {
            HANDLE_CHANGE(theItem.setLinearFade(value.getData<float>()))
        } else if (name == QLatin1String("expfade")) {
            HANDLE_CHANGE(theItem.setExpFade(value.getData<float>()))
        } else if (name == QLatin1String("areawidth")) {
            HANDLE_CHANGE(theItem.setAreaWidth(value.getData<float>()))
        } else if (name == QLatin1String("areaheight")) {
            HANDLE_CHANGE(theItem.setAreaHeight(value.getData<float>()))
        } else if (name == QLatin1String("castshadow")) {
            HANDLE_CHANGE(theItem.setCastShadow(value.getData<bool>()))
        } else if (name == QLatin1String("shdwfactor")) {
            HANDLE_CHANGE(theItem.setShadowFactor(value.getData<float>()))
        } else if (name == QLatin1String("shdwfilter")) {
            HANDLE_CHANGE(theItem.setShadowFilter(value.getData<float>()))
        } else if (name == QLatin1String("shdwmapres")) {
            HANDLE_CHANGE(theItem.setShadowMapRes(value.getData<int>()))
        } else if (name == QLatin1String("shdwbias")) {
            HANDLE_CHANGE(theItem.setShadowBias(value.getData<float>()))
        } else if (name == QLatin1String("shdwmapfar")) {
            HANDLE_CHANGE(theItem.setShadowMapFar(value.getData<float>()))
        } else if (name == QLatin1String("shdwmapfov")) {
            HANDLE_CHANGE(theItem.setShadowMapFov(value.getData<float>()))
        }
        if (ret)
            theItem.notifyPropertyChanges(changeList);
        return ret;
    }
};

class Q3DSModelTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSModelTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSModelNode &model)
        : Q3DSNodeTranslator(instance, model)
    {

    }

    void pushTranslation(Q3DSTranslation &inContext) override
    {
        Q3DSNodeTranslator::pushTranslation(inContext);

        Q3DSModelNode &theItem = static_cast<Q3DSModelNode &>(graphObject());
        Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
        Q3DSPropertyChangeList changeList;
        ITERATE_Q3DS_MODEL_PROPERTIES

        int childCount = inContext.assetGraph().GetChildCount(instanceHandle());
        for (int i = 0; i < childCount; ++i) {
            qt3dsdm::Qt3DSDMInstanceHandle childInstance
                    = inContext.assetGraph().GetChild(instanceHandle(), i);
            Q3DSGraphObjectTranslator *childTranslator
                    = inContext.getOrCreateTranslator(childInstance);
            if (childTranslator) {
                if (isMaterial(childTranslator->graphObject()))
                    theItem.appendChildNode(&childTranslator->graphObject());
                childTranslator->pushTranslation(inContext);
            }
        }
        theItem.resolveReferences(*inContext.presentation());
    }

    bool isMaterial(const Q3DSGraphObject &inChild)
    {
        if (inChild.type() == Q3DSGraphObject::ReferencedMaterial ||
            inChild.type() == Q3DSGraphObject::DefaultMaterial ||
            inChild.type() == Q3DSGraphObject::CustomMaterial) {
            return true;
        }
        return false;
    }

    void appendChild(Q3DSGraphObject &inChild) override
    {
        if (inChild.isNode() || isMaterial(inChild))
            Q3DSNodeTranslator::appendChild(inChild);
        else
            QT3DS_ASSERT(false);
    }

    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override
    {
        bool ret = false;
        if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
            return true;

        Q3DSPropertyChangeList changeList;
        Q3DSModelNode &theItem = static_cast<Q3DSModelNode &>(graphObject());
        if (name == QLatin1String("sourcepath")) {
            HANDLE_CHANGE(theItem.setMesh(value.toQVariant().toString()))
            theItem.resolveReferences(*inContext.presentation());
        } else if (name == QLatin1String("poseroot")) {
            HANDLE_CHANGE(theItem.setSkeletonRoot(value.getData<int>()))
        } else if (name == QLatin1String("tessellation")) {
            HANDLE_CHANGE(theItem.setTessellation(
                              Q3DSValueParser::parseEnum<Q3DSModelNode::Tessellation>(value)))
        } else if (name == QLatin1String("edgetess")) {
            HANDLE_CHANGE(theItem.setEdgeTess(value.getData<float>()))
        } else if (name == QLatin1String("innertess")) {
            HANDLE_CHANGE(theItem.setInnerTess(value.getData<float>()))
        }
        if (ret)
            theItem.notifyPropertyChanges(changeList);
        return ret;
    }
};

class Q3DSImageTranslator : public Q3DSGraphObjectTranslator
{
public:
    Q3DSImageTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSImage &image)
        : Q3DSGraphObjectTranslator(instance, image)
    {

    }

    void pushTranslation(Q3DSTranslation &inContext) override
    {
        Q3DSGraphObjectTranslator::pushTranslation(inContext);

        Q3DSImage &theItem = static_cast<Q3DSImage &>(graphObject());
        Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
        Q3DSPropertyChangeList changeList;
        ITERATE_Q3DS_IMAGE_PROPERTIES
        theItem.resolveReferences(*inContext.presentation());
    }

    void appendChild(Q3DSGraphObject &) override
    {

    }

    void setActive(bool) override
    {
    }

    void clearChildren() override
    {
    }

    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override
    {
        bool ret = false;
        if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
            return true;

        Q3DSPropertyChangeList changeList;
        Q3DSImage &theItem = static_cast<Q3DSImage &>(graphObject());
        if (name == QLatin1String("scaleu")) {
            HANDLE_CHANGE(theItem.setScaleU(value.getData<float>()))
        } else if (name == QLatin1String("scalev")) {
            HANDLE_CHANGE(theItem.setScaleV(value.getData<float>()))
        } else if (name == QLatin1String("subpresentation")) {
            HANDLE_CHANGE(theItem.setSubPresentation(value.toQVariant().toString()))
            theItem.resolveReferences(*inContext.presentation());
        } else if (name == QLatin1String("mappingmode")) {
            HANDLE_CHANGE(theItem.setMappingMode(
                              Q3DSValueParser::parseEnum<Q3DSImage::MappingMode>(value)))
        } else if (name == QLatin1String("tilingmodehorz")) {
            HANDLE_CHANGE(theItem.setHorizontalTiling(
                              Q3DSValueParser::parseEnum<Q3DSImage::TilingMode>(value)))
        } else if (name == QLatin1String("tilingmodevert")) {
            HANDLE_CHANGE(theItem.setVerticalTiling(
                              Q3DSValueParser::parseEnum<Q3DSImage::TilingMode>(value)))
        } else if (name == QLatin1String("rotationuv")) {
            HANDLE_CHANGE(theItem.setRotationUV(value.getData<float>()))
        } else if (name == QLatin1String("positionu")) {
            HANDLE_CHANGE(theItem.setPositionU(value.getData<float>()))
        } else if (name == QLatin1String("positionv")) {
            HANDLE_CHANGE(theItem.setPositionV(value.getData<float>()))
        } else if (name == QLatin1String("pivotu")) {
            HANDLE_CHANGE(theItem.setPivotU(value.getData<float>()))
        } else if (name == QLatin1String("pivotv")) {
            HANDLE_CHANGE(theItem.setPivotV(value.getData<float>()))
        }
        if (ret)
            theItem.notifyPropertyChanges(changeList);
        return ret;
    }
};

class Q3DSDefaultMaterialTranslator : public Q3DSGraphObjectTranslator
{
public:
    Q3DSDefaultMaterialTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                  Q3DSDefaultMaterial &material)
        : Q3DSGraphObjectTranslator(instance, material)
    {

    }

    void pushTranslation(Q3DSTranslation &inContext) override
    {
        Q3DSGraphObjectTranslator::pushTranslation(inContext);

        Q3DSDefaultMaterial &theItem = static_cast<Q3DSDefaultMaterial &>(graphObject());
        Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
        Q3DSPropertyChangeList changeList;
        ITERATE_Q3DS_DEFAULT_MATERIAL_PROPERTIES
        int childCount = inContext.assetGraph().GetChildCount(instanceHandle());
        for (int i = 0; i < childCount; ++i) {
            qt3dsdm::Qt3DSDMInstanceHandle childInstance
                    = inContext.assetGraph().GetChild(instanceHandle(), i);
            Q3DSGraphObjectTranslator *childTranslator
                    = inContext.getOrCreateTranslator(childInstance);
            if (childTranslator->graphObject().type() == Q3DSGraphObject::Image)
                theItem.appendChildNode(&childTranslator->graphObject());
            childTranslator->pushTranslation(inContext);
        }
        theItem.resolveReferences(*inContext.presentation());
    }

    void setActive(bool) override
    {
    }

    void clearChildren() override
    {
    }

    void appendChild(Q3DSGraphObject &) override
    {
    }

    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override
    {
        bool ret = false;
        if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
            return true;

        Q3DSPropertyChangeList changeList;
        Q3DSDefaultMaterial &theItem = static_cast<Q3DSDefaultMaterial &>(graphObject());
        if (name == QLatin1String("shaderlighting")) {
            HANDLE_CHANGE(theItem.setShaderLighting(
                              Q3DSValueParser::parseEnum<
                                                    Q3DSDefaultMaterial::ShaderLighting>(value)))
        } else if (name == QLatin1String("blendmode")) {
            HANDLE_CHANGE(theItem.setBlendMode(
                              Q3DSValueParser::parseEnum<Q3DSDefaultMaterial::BlendMode>(value)))
        }/* else if (name == QLatin1String("vertexcolors")) {
            ret = true;
        } */else if (name == QLatin1String("diffuse")) {
            HANDLE_CHANGE(theItem.setDiffuse(Q3DSValueParser::parseColor(value)))
        } else if (name == QLatin1String("diffusemap")) {
            HANDLE_CHANGE(theItem.setDiffuseMap(Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("diffusemap2")) {
            HANDLE_CHANGE(theItem.setDiffuseMap2(Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("diffusemap3")) {
            HANDLE_CHANGE(theItem.setDiffuseMap3(Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("specularreflection")) {
            HANDLE_CHANGE(theItem.setSpecularReflection(
                              Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("speculartint")) {
            HANDLE_CHANGE(theItem.setSpecularTint(Q3DSValueParser::parseColor(value)))
        } else if (name == QLatin1String("specularamount")) {
            HANDLE_CHANGE(theItem.setSpecularAmount(value.getData<float>()))
        } else if (name == QLatin1String("specularmap")) {
            HANDLE_CHANGE(theItem.setSpecularMap(Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("specularmodel")) {
            HANDLE_CHANGE(theItem.setSpecularModel(
                              Q3DSValueParser::parseEnum<Q3DSDefaultMaterial::SpecularModel>(value)))
        } else if (name == QLatin1String("specularroughness")) {
            HANDLE_CHANGE(theItem.setSpecularRoughness(value.getData<float>()))
        } else if (name == QLatin1String("roughnessmap")) {
            HANDLE_CHANGE(theItem.setRoughnessMap(Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("fresnelPower")) {
            HANDLE_CHANGE(theItem.setFresnelPower(value.getData<float>()))
        } else if (name == QLatin1String("ior")) {
            HANDLE_CHANGE(theItem.setIor(value.getData<float>()))
        } else if (name == QLatin1String("bumpmap")) {
            HANDLE_CHANGE(theItem.setBumpMap(Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("normalmap")) {
            HANDLE_CHANGE(theItem.setNormalMap(Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("bumpamount")) {
            HANDLE_CHANGE(theItem.setBumpAmount(value.getData<float>()))
        } else if (name == QLatin1String("displacementmap")) {
            HANDLE_CHANGE(theItem.setDisplacementMap(
                              Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("displaceamount")) {
            HANDLE_CHANGE(theItem.setDisplaceAmount(value.getData<float>()))
        } else if (name == QLatin1String("opacity")) {
            HANDLE_CHANGE(theItem.setOpacity(value.getData<float>()))
        } else if (name == QLatin1String("opacitymap")) {
            HANDLE_CHANGE(theItem.setOpacityMap(Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("emissivecolor")) {
            HANDLE_CHANGE(theItem.setEmissiveColor(Q3DSValueParser::parseColor(value)))
        } else if (name == QLatin1String("emissivepower")) {
            HANDLE_CHANGE(theItem.setEmissivePower(value.getData<float>()))
        } else if (name == QLatin1String("emissivemap")) {
            HANDLE_CHANGE(theItem.setEmissiveMap(Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("emissivemap2")) {
            HANDLE_CHANGE(theItem.setEmissiveMap2(Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("translucencymap")) {
            HANDLE_CHANGE(theItem.setTranslucencyMap(
                              Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("translucentfalloff")) {
            HANDLE_CHANGE(theItem.setTranslucentFalloff(value.getData<float>()))
        } else if (name == QLatin1String("diffuselightwrap")) {
            HANDLE_CHANGE(theItem.setDiffuseLightWrap(value.getData<float>()))
        } else if (name == QLatin1String("iblprobe")) {
            HANDLE_CHANGE(theItem.setLightProbe(Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("lightmapindirect")) {
            HANDLE_CHANGE(theItem.setLightmapIndirectMap(
                              Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("lightmapradiosity")) {
            HANDLE_CHANGE(theItem.setLightmapRadiosityMap(
                              Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("lightmapshadow")) {
            HANDLE_CHANGE(theItem.setLightmapShadowMap(
                              Q3DSValueParser::parseImage(&inContext, value)))
        }
        if (ret)
            theItem.notifyPropertyChanges(changeList);
        return ret;
    }
};

class Q3DSLayerTranslator : public Q3DSNodeTranslator
{
public:
    Q3DSLayerTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSLayerNode &layer)
        : Q3DSNodeTranslator(instance, layer)
    {

    }

    void pushTranslation(Q3DSTranslation &inContext) override
    {
        Q3DSNodeTranslator::pushTranslation(inContext);

        Q3DSLayerNode &theItem = static_cast<Q3DSLayerNode &>(graphObject());
        Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
        Q3DSPropertyChangeList changeList;
        ITERATE_Q3DS_LAYER_PROPERTIES

        int childCount = inContext.assetGraph().GetChildCount(instanceHandle());
        for (int i = 0; i < childCount; ++i) {
            qt3dsdm::Qt3DSDMInstanceHandle childInstance
                    = inContext.assetGraph().GetChild(instanceHandle(), i);
            Q3DSGraphObjectTranslator *childTranslator
                    = inContext.getOrCreateTranslator(childInstance);
            if (childTranslator && childTranslator->graphObject().isNode()
                    && childTranslator->graphObject().parent() == nullptr) {
                theItem.appendChildNode(&childTranslator->graphObject());
                childTranslator->pushTranslation(inContext);
            }
        }
        theItem.resolveReferences(*inContext.presentation());
    }

    void appendChild(Q3DSGraphObject &inChild) override
    {
        if (inChild.isNode()) {
            Q3DSNodeTranslator::appendChild(inChild);
        } else if (inChild.type() == Q3DSNode::Effect) {
            Q3DSLayerNode &theItem = static_cast<Q3DSLayerNode &>(graphObject());
            theItem.appendChildNode(&inChild);
        }/* TODO: how to handle render plugins
        else if (inChild.m_Type == GraphObjectTypes::RenderPlugin) {
            SLayer &theItem = static_cast<SLayer &>(GetGraphObject());
            theItem.m_RenderPlugin = &static_cast<qt3ds::render::SRenderPlugin &>(inChild);
        }*/
    }

    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override
    {
        bool ret = false;
        if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
            return true;

        Q3DSPropertyChangeList changeList;
        Q3DSLayerNode &theItem = static_cast<Q3DSLayerNode &>(graphObject());
        if (name == QLatin1String("disabledepthtest")) {
            HANDLE_CHANGE(theItem.setDepthTestDisabled(value.getData<bool>()))
        } else if (name == QLatin1String("disabledepthprepass")) {
            HANDLE_CHANGE(theItem.setDepthPrePassDisabled(value.getData<bool>()))
        } else if (name == QLatin1String("progressiveaa")) {
            HANDLE_CHANGE(theItem.setProgressiveAA(Q3DSValueParser::parseEnum
                                     <Q3DSLayerNode::ProgressiveAA>(value)))
        } else if (name == QLatin1String("multisampleaa")) {
            HANDLE_CHANGE(theItem.setMultisampleAA(Q3DSValueParser::parseEnum
                                     <Q3DSLayerNode::MultisampleAA>(value)))
        } else if (name == QLatin1String("temporalaa")) {
            HANDLE_CHANGE(theItem.setTemporalAAEnabled(value.getData<bool>()))
        } else if (name == QLatin1String("background")) {
            HANDLE_CHANGE(theItem.setLayerBackground(Q3DSValueParser::parseEnum
                                       <Q3DSLayerNode::LayerBackground>(value)))
        } else if (name == QLatin1String("backgroundcolor")) {
            HANDLE_CHANGE(theItem.setBackgroundColor(Q3DSValueParser::parseColor(value)))
        } else if (name == QLatin1String("blendtype")) {
            HANDLE_CHANGE(theItem.setBlendType(Q3DSValueParser::parseEnum
                                       <Q3DSLayerNode::BlendType>(value)))
        } else if (name == QLatin1String("horzfields")) {
            HANDLE_CHANGE(theItem.setHorizontalFields(Q3DSValueParser::parseEnum
                                        <Q3DSLayerNode::HorizontalFields>(value)))
        } else if (name == QLatin1String("left")) {
            HANDLE_CHANGE(theItem.setLeft(value.getData<float>()))
        } else if (name == QLatin1String("leftunits")) {
            HANDLE_CHANGE(theItem.setLeftUnits(Q3DSValueParser::parseEnum
                                        <Q3DSLayerNode::Units>(value)))
        } else if (name == QLatin1String("width")) {
            HANDLE_CHANGE(theItem.setWidth(value.getData<float>()))
        } else if (name == QLatin1String("widthunits")) {
            HANDLE_CHANGE(theItem.setLeftUnits(Q3DSValueParser::parseEnum
                                        <Q3DSLayerNode::Units>(value)))
        } else if (name == QLatin1String("right")) {
            HANDLE_CHANGE(theItem.setRight(value.getData<float>()))
        } else if (name == QLatin1String("rightunits")) {
            HANDLE_CHANGE(theItem.setRightUnits(Q3DSValueParser::parseEnum
                                        <Q3DSLayerNode::Units>(value)))
        } else if (name == QLatin1String("vertfields")) {
            HANDLE_CHANGE(theItem.setVerticalFields(Q3DSValueParser::parseEnum
                                        <Q3DSLayerNode::VerticalFields>(value)))
        } else if (name == QLatin1String("top")) {
            HANDLE_CHANGE(theItem.setTop(value.getData<float>()))
        } else if (name == QLatin1String("topunits")) {
            HANDLE_CHANGE(theItem.setTopUnits(Q3DSValueParser::parseEnum
                                        <Q3DSLayerNode::Units>(value)))
        } else if (name == QLatin1String("height")) {
            HANDLE_CHANGE(theItem.setHeight(value.getData<float>()))
        } else if (name == QLatin1String("heightunits")) {
            HANDLE_CHANGE(theItem.setHeightUnits(Q3DSValueParser::parseEnum
                                        <Q3DSLayerNode::Units>(value)))
        } else if (name == QLatin1String("bottom")) {
            HANDLE_CHANGE(theItem.setBottom(value.getData<float>()))
        } else if (name == QLatin1String("bottomunits")) {
            HANDLE_CHANGE(theItem.setBottomUnits(Q3DSValueParser::parseEnum
                                        <Q3DSLayerNode::Units>(value)))
        } else if (name == QLatin1String("sourcepath")) {
            HANDLE_CHANGE(theItem.setSourcePath(value.toQVariant().toString()))
            theItem.resolveReferences(*inContext.presentation());
        } else if (name == QLatin1String("aostrength")) {
            HANDLE_CHANGE(theItem.setAoStrength(value.getData<float>()))
        } else if (name == QLatin1String("aodistance")) {
            HANDLE_CHANGE(theItem.setAoDistance(value.getData<float>()))
        } else if (name == QLatin1String("aosoftness")) {
            HANDLE_CHANGE(theItem.setAoSoftness(value.getData<float>()))
        } else if (name == QLatin1String("aobias")) {
            HANDLE_CHANGE(theItem.setAoBias(value.getData<float>()))
        } else if (name == QLatin1String("aosamplerate")) {
            HANDLE_CHANGE(theItem.setAoSampleRate(value.getData<int>()))
        } else if (name == QLatin1String("aodither")) {
            HANDLE_CHANGE(theItem.setAoDither(value.getData<bool>()))
        } else if (name == QLatin1String("shadowstrength")) {
            HANDLE_CHANGE(theItem.setShadowStrength(value.getData<float>()))
        } else if (name == QLatin1String("shadowdist")) {
            HANDLE_CHANGE(theItem.setShadowDist(value.getData<float>()))
        } else if (name == QLatin1String("shadowsoftness")) {
            HANDLE_CHANGE(theItem.setShadowSoftness(value.getData<float>()))
        } else if (name == QLatin1String("shadowbias")) {
            HANDLE_CHANGE(theItem.setShadowBias(value.getData<float>()))
        } else if (name == QLatin1String("lightprobe")) {
            HANDLE_CHANGE(theItem.setLightProbe(Q3DSValueParser::parseImage(&inContext, value)))
        } else if (name == QLatin1String("probebright")) {
            HANDLE_CHANGE(theItem.setBottom(value.getData<float>()))
        } else if (name == QLatin1String("fastibl")) {
            HANDLE_CHANGE(theItem.setBottom(value.getData<float>()))
        } else if (name == QLatin1String("probehorizon")) {
            HANDLE_CHANGE(theItem.setBottom(value.getData<float>()))
        } else if (name == QLatin1String("probefov")) {
            HANDLE_CHANGE(theItem.setBottom(value.getData<float>()))
        } else if (name == QLatin1String("lightprobe2")) {
            HANDLE_CHANGE(theItem.setBottom(value.getData<float>()))
        } else if (name == QLatin1String("probe2fade")) {
            HANDLE_CHANGE(theItem.setBottom(value.getData<float>()))
        } else if (name == QLatin1String("probe2window")) {
            HANDLE_CHANGE(theItem.setBottom(value.getData<float>()))
        } else if (name == QLatin1String("probe2pos")) {
            HANDLE_CHANGE(theItem.setBottom(value.getData<float>()))
        }
        if (ret)
            theItem.notifyPropertyChanges(changeList);
        return ret;
    }
};

class Q3DSSlideTranslator : public Q3DSGraphObjectTranslator
{
public:
    Q3DSSlideTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSSlide &slide)
        : Q3DSGraphObjectTranslator(instance, slide), m_isMaster(false)
    {

    }

    void pushTranslation(Q3DSTranslation &inContext) override
    {
        Q3DSGraphObjectTranslator::pushTranslation(inContext);

        Q3DSSlide &theItem = static_cast<Q3DSSlide &>(graphObject());
        Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
        Q3DSPropertyChangeList changeList;
        ITERATE_Q3DS_SLIDE_PROPERTIES

        Q3DSSlide::PlayThrough playthrough;
        QVariant value;
        if (theParser.parsePlaythroughToProperty(HANDLE_PROPERTY(Slide, PlaythroughTo),
                                                 playthrough, value)) {
            changeList.append(theItem.setPlayThrough(playthrough));
            if (playthrough == Q3DSSlide::Value)
                changeList.append(theItem.setPlayThroughValue(value));
        }
        HANDLE_Q3DS_NOTIFY_CHANGES
        theItem.resolveReferences(*inContext.presentation());
    }

    void appendChild(Q3DSGraphObject &inChild) override
    {
        if (inChild.type() != Q3DSNode::Slide && !masterSlide()) {
            QT3DS_ASSERT(false);
            return;
        }

        graphObject().appendChildNode(&inChild);
    }

    void clearChildren() override
    {

    }

    void setActive(bool) override
    {

    }

    bool updateProperty(Q3DSTranslation &inContext,
                        qt3dsdm::Qt3DSDMInstanceHandle instance,
                        qt3dsdm::Qt3DSDMPropertyHandle property,
                        qt3dsdm::SValue &value,
                        const QString &name) override
    {
        bool ret = false;
        if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
            return true;

        Q3DSPropertyChangeList changeList;
        Q3DSSlide &theItem = static_cast<Q3DSSlide &>(graphObject());
        if (name == QLatin1String("playmode")) {
            HANDLE_CHANGE(theItem.setPlayMode(
                              Q3DSValueParser::parseEnum<Q3DSSlide::PlayMode>(value)))
        } else if (name == QLatin1String("playthroughto")) {
            HANDLE_CHANGE(theItem.setPlayThrough(
                              Q3DSValueParser::parseEnum<Q3DSSlide::PlayThrough>(value)))
        } else if (name == QLatin1String("initialplaystate")) {
            HANDLE_CHANGE(theItem.setInitialPlayState(
                              Q3DSValueParser::parseEnum<Q3DSSlide::InitialPlayState>(value)))
        }
        if (ret)
            theItem.notifyPropertyChanges(changeList);
        return ret;
    }

    bool masterSlide() const
    {
        return m_isMaster;
    }

    void setMasterSlide(bool master)
    {
        m_isMaster = master;
    }

private:
    bool m_isMaster;
};

Q3DSTranslation::Q3DSTranslation(Q3DStudioRenderer &inRenderer)
    : m_studioRenderer(inRenderer)
    , m_doc(*g_StudioApp.GetCore()->GetDoc())
    , m_reader(m_doc.GetDocumentReader())
    , m_objectDefinitions(
          m_doc.GetStudioSystem()->GetClientDataModelBridge()->GetObjectDefinitions())
    , m_studioSystem(*m_doc.GetStudioSystem())
    , m_fullSystem(*m_doc.GetStudioSystem()->GetFullSystem())
    , m_assetGraph(*m_doc.GetAssetGraph())
    , m_engine(inRenderer.engine())
    , m_presentation(new Q3DSUipPresentation())
{
    qt3dsdm::Qt3DSDMInstanceHandle sceneRoot = m_assetGraph.GetRoot(0);
    m_graphIterator.ClearResults();
    m_assetGraph.GetDepthFirst(m_graphIterator, sceneRoot);
    for (; !m_graphIterator.IsDone(); ++m_graphIterator) {
        qt3dsdm::Qt3DSDMInstanceHandle instance(m_graphIterator.GetCurrent());
        getOrCreateTranslator(instance);
    }

    std::shared_ptr<qt3dsdm::ISlideCore> slideCore = m_fullSystem.GetSlideCore();
    std::shared_ptr<qt3dsdm::ISlideSystem> slideSystem = m_fullSystem.GetSlideSystem();
    qt3dsdm::TSlideHandleList slideList;
    slideCore->GetSlides(slideList);
    for (qt3dsdm::TSlideHandleList::iterator it = slideList.begin(); it < slideList.end(); ++it) {
        qt3dsdm::Qt3DSDMInstanceHandle instance(slideCore->GetSlideInstance(*it));
        Q3DSGraphObjectTranslator *translator = getOrCreateTranslator(instance);
        Q3DSSlide &slide = static_cast<Q3DSSlide&>(translator->graphObject());
        if (m_reader.IsMasterSlide(*it)) {
            m_slideTranslators.push_front(translator);
            static_cast<Q3DSSlideTranslator *>(m_slideTranslators[0])->setMasterSlide(true);
        } else {
            m_slideTranslators.push_back(translator);
        }
        qt3dsdm::TInstanceHandleList slideInstances;
        slideSystem->GetAssociatedInstances(*it, slideInstances);
        for (unsigned int i = 0; i < slideInstances.size(); ++i) {
            qt3dsdm::Qt3DSDMInstanceHandle inst(slideInstances[i]);
            if (m_translatorMap.contains(inst)) {
                THandleTranslatorPairList &theList = *m_translatorMap.find(inst);
                ThandleTranslatorOption t
                        = findTranslator(theList, qt3dsdm::Qt3DSDMInstanceHandle());
                if (t.hasValue() && t->second->graphObject().type() != Q3DSGraphObject::Slide)
                    slide.addObject(&t->second->graphObject());
            }
        }
    }
    m_presentation->setMasterSlide(
                static_cast<Q3DSSlide*>(&m_slideTranslators[0]->graphObject()));
    for (int i = 1; i < m_slideTranslators.size(); ++i)
        m_slideTranslators[0]->appendChild(m_slideTranslators[i]->graphObject());

    qt3dsdm::IStudioFullSystemSignalProvider *theProvider = m_fullSystem.GetSignalProvider();
    m_signalConnections.push_back(
        theProvider->ConnectInstanceCreated(std::bind(&Q3DSTranslation::markDirty,
                                                      this, std::placeholders::_1)));
    m_signalConnections.push_back(theProvider->ConnectInstanceDeleted(
        std::bind(&Q3DSTranslation::releaseTranslation, this, std::placeholders::_1)));
    m_signalConnections.push_back(
        theProvider->ConnectInstancePropertyValue(std::bind(&Q3DSTranslation::markPropertyDirty,
                                                            this, std::placeholders::_1,
                                                            std::placeholders::_2)));
    m_signalConnections.push_back(m_assetGraph.ConnectChildAdded(
        std::bind(&Q3DSTranslation::markGraphInstanceDirty, this, std::placeholders::_1,
                  std::placeholders::_2)));
    m_signalConnections.push_back(m_assetGraph.ConnectChildMoved(
        std::bind(&Q3DSTranslation::markGraphInstanceDirty, this, std::placeholders::_1,
                  std::placeholders::_2)));
    m_signalConnections.push_back(m_assetGraph.ConnectChildRemoved(
        std::bind(&Q3DSTranslation::markGraphInstanceDirty, this, std::placeholders::_1,
                  std::placeholders::_2)));
    m_signalConnections.push_back(theProvider->ConnectBeginComponentSeconds(
        std::bind(&Q3DSTranslation::markBeginComponentSeconds, this, std::placeholders::_1)));
    m_signalConnections.push_back(theProvider->ConnectComponentSeconds(
        std::bind(&Q3DSTranslation::markComponentSeconds, this, std::placeholders::_1)));
}

void Q3DSTranslation::markDirty(qt3dsdm::Qt3DSDMInstanceHandle)
{

}

void Q3DSTranslation::markPropertyDirty(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                        qt3dsdm::Qt3DSDMPropertyHandle property)
{
    SValue value;
    qt3dsdm::IPropertySystem *prop = m_doc.GetPropertySystem();
    prop->GetInstancePropertyValue(instance, property, value);
    QString name = QString::fromWCharArray(prop->GetName(property).wide_str());

    // ignore these properties
    if (name == QLatin1String("shy"))
        return;

    TInstanceToTranslatorMap::iterator theTranslatorList;
    if (!m_translatorMap.contains(instance))
        theTranslatorList = m_translatorMap.insert(instance, THandleTranslatorPairList());
    else
        theTranslatorList = m_translatorMap.find(instance);
    THandleTranslatorPairList &theList = *theTranslatorList;
    ThandleTranslatorOption t = findTranslator(theList, qt3dsdm::Qt3DSDMInstanceHandle());
    if (!t.isEmpty()) {
        t.getValue().second->updateProperty(*this, instance, property, value, name);
        m_studioRenderer.RequestRender();
    }
}

void Q3DSTranslation::releaseTranslation(qt3dsdm::Qt3DSDMInstanceHandle instance)
{

}

void Q3DSTranslation::markGraphInstanceDirty(int instance, int parent)
{

}

void Q3DSTranslation::markBeginComponentSeconds(qt3dsdm::Qt3DSDMSlideHandle slide)
{

}

void Q3DSTranslation::markComponentSeconds(qt3dsdm::Qt3DSDMSlideHandle)
{

}

QByteArray Q3DSTranslation::getInstanceObjectId(qt3dsdm::Qt3DSDMInstanceHandle instance)
{
    QByteArray ret;
    Q3DStudio::CString theId = m_reader.GetFileId(instance);
    if (!theId.size())
        theId = m_reader.GetName(instance);

    if (theId.size()) {
        qt3ds::foundation::CRegisteredString rid
                = Q3DSStringTable::instance()->GetRenderStringTable()
                    .RegisterStr(theId.toQString());
        ret = rid.qstring().toLatin1();
    }
    return ret;
}

Q3DSTranslation::ThandleTranslatorOption Q3DSTranslation::findTranslator(
        Q3DSTranslation::THandleTranslatorPairList &list,
        qt3dsdm::Qt3DSDMInstanceHandle instance)
{
    for (THandleTranslatorPairList::Iterator it = list.begin();
         it < list.end(); ++it) {
        if (it->first == instance) {
            return Q3DSTranslation::ThandleTranslatorOption(THandleTranslatorPair(instance,
                                                                                  it->second));
        }
    }
    return Q3DSTranslation::ThandleTranslatorOption();
}

void Q3DSTranslation::setPresentationData()
{
    CStudioProjectSettings *settings = m_doc.GetCore()->GetStudioProjectSettings();
    m_presentation_data.m_author = settings->GetAuthor();
    m_presentation_data.m_company = settings->GetCompany();
    m_presentation_data.m_width = settings->GetPresentationSize().x;
    m_presentation_data.m_height = settings->GetPresentationSize().y;
    m_presentation_data.m_srcPath = m_doc.GetDocumentPath().GetPath().toQString();

    m_presentation->setSourceFile(m_presentation_data.m_srcPath);
    m_presentation->setAuthor(m_presentation_data.m_author);
    m_presentation->setCompany(m_presentation_data.m_author);
    m_presentation->setPresentationWidth(m_presentation_data.m_width);
    m_presentation->setPresentationHeight(m_presentation_data.m_height);
    m_presentation->setMaintainAspectRatio(settings->GetMaintainAspect());
    m_presentation->setPresentationRotation(settings->GetRotatePresentation()
                                            ? Q3DSUipPresentation::Clockwise90
                                            : Q3DSUipPresentation::NoRotation);
}

Q3DSGraphObjectTranslator *Q3DSTranslation::createTranslator(
        qt3dsdm::Qt3DSDMInstanceHandle instance)
{
    Q3DSGraphObjectTranslator *translator = nullptr;
    qt3dsdm::ComposerObjectTypes::Enum type = m_objectDefinitions.GetType(instance);
    qt3dsdm::Qt3DSDMInstanceHandle parentClass = m_reader.GetFirstBaseClass(instance);
    if (type == qt3dsdm::ComposerObjectTypes::Unknown && parentClass.Valid())
        type = m_objectDefinitions.GetType(parentClass);

    QByteArray id = getInstanceObjectId(instance);

    // For the subset of possible instances, pick out the valid translators.
    switch (type) {
    case qt3dsdm::ComposerObjectTypes::Group: {
        translator = new Q3DSGroupNodeTranslator(instance,
                                                 *m_presentation->newObject<Q3DSGroupNode>(id));
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Component: {
        Q3DSComponentNode &component = *m_presentation->newObject<Q3DSComponentNode>(id);
        translator = new Q3DSComponentNodeTranslator(instance, component);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Scene: {
        translator = new Q3DSSceneTranslator(instance, *m_presentation->newObject<Q3DSScene>(id));
        m_scene = static_cast<Q3DSScene *>(&translator->graphObject());
        m_presentation->setScene(m_scene);
        setPresentationData();
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Layer: {
        translator = new Q3DSLayerTranslator(instance,
                                             *m_presentation->newObject<Q3DSLayerNode>(id));
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Slide: {
        translator = new Q3DSSlideTranslator(instance, *m_presentation->newObject<Q3DSSlide>(id));
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Camera: {
        translator = new Q3DSCameraTranslator(instance,
                                              *m_presentation->newObject<Q3DSCameraNode>(id));
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Light: {
        translator = new Q3DSLightTranslator(instance,
                                             *m_presentation->newObject<Q3DSLightNode>(id));
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Model: {
        translator = new Q3DSModelTranslator(instance,
                                             *m_presentation->newObject<Q3DSModelNode>(id));
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Material: {
        Q3DSDefaultMaterial &material = *m_presentation->newObject<Q3DSDefaultMaterial>(id);
        translator = new Q3DSDefaultMaterialTranslator(instance, material);
        break;
    }
    case qt3dsdm::ComposerObjectTypes::Image: {
        translator = new Q3DSImageTranslator(instance, *m_presentation->newObject<Q3DSImage>(id));
        break;
    }
        // TODO: Text, ReferencedMaterial, Alias, Effect, CustomMaterial, RenderPlugin?
    default:
        break;
    }
    return translator;
}

Q3DSGraphObjectTranslator *Q3DSTranslation::getOrCreateTranslator(
        qt3dsdm::Qt3DSDMInstanceHandle instance)
{
    return getOrCreateTranslator(instance, qt3dsdm::Qt3DSDMInstanceHandle());
}

Q3DSGraphObjectTranslator *Q3DSTranslation::getOrCreateTranslator(
        qt3dsdm::Qt3DSDMInstanceHandle instance, qt3dsdm::Qt3DSDMInstanceHandle aliasInstance)
{
    TInstanceToTranslatorMap::iterator theTranslatorList;
    if (!m_translatorMap.contains(instance))
        theTranslatorList = m_translatorMap.insert(instance, THandleTranslatorPairList());
    else
        theTranslatorList = m_translatorMap.find(instance);
    THandleTranslatorPairList &theList = *theTranslatorList;
    ThandleTranslatorOption theExistingTranslator = findTranslator(theList, aliasInstance);

    if (theExistingTranslator.hasValue())
        return theExistingTranslator->second;
    if (m_reader.IsInstance(instance) == false)
        return nullptr;

    Q3DSGraphObjectTranslator *theNewTranslator = createTranslator(instance);
    if (theNewTranslator != nullptr) {
        theNewTranslator->setAliasInstanceHandle(aliasInstance);
        m_dirtySet.insert(*theNewTranslator);
        theList.push_back(THandleTranslatorPair(aliasInstance, theNewTranslator));
    }

    return theNewTranslator;
}

void Q3DSTranslation::clearDirtySet()
{
    for (unsigned int idx = 0; idx < m_dirtySet.size(); ++idx) {
        if (m_reader.IsInstance(m_dirtySet[idx]->instanceHandle())
                && m_dirtySet[idx]->dirty()) {
            m_dirtySet[idx]->pushTranslation(*this);
        }
    }
    m_dirtySet.clear();
}

void Q3DSTranslation::prepareRender(const QRect &rect, const QSize &size)
{
    if (!m_scene)
        return;
    clearDirtySet();
    if (m_engine->presentationCount() == 0) {
        m_engine->setPresentation(m_presentation.data());
#if (Q3DS_ENABLE_PROFILEUI == 1)
        m_engine->setProfileUiVisible(true, true);
#endif
        m_studioRenderer.SetViewRect(m_studioRenderer.viewRect());
    }
    if (rect != m_rect || size != m_size) {
        m_engine->sceneManager()->updateSizes(size, 1.0, rect, true);
        m_rect = rect;
        m_size = size;
    }
}

}
