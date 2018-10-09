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
#include "Q3DSGraphObjectTranslator.h"
#include "Q3DSTranslators.h"
#include "Q3DSStringTable.h"

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
        Option<SValue> theValue
                = m_context.reader().GetRawInstancePropertyValue(instanceHandle(), inProperty);
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
                Q3DStudio::CFilePath theResolvedPath
                        = Q3DStudio::CFilePath::CombineBaseAndRelative(theDirectory,
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
                                 : QString();
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
            } else {
                ioImage = nullptr;
            }
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
        list.append(theItem.set##name(temp)); \
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
    theItem.notifyPropertyChanges(list);

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

#define ITERATE_Q3DS_REFERENCED_MATERIAL_PROPERTIES                                     \
    HANDLE_Q3DS_OBJREF_PROPERTY(ReferencedMaterial, ReferencedMaterial)                 \
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

#define ITERATE_Q3DS_TEXT_PROPERTIES                                                    \
    HANDLE_Q3DS_STRING_PROPERTY2(Text, TextString, Text)                                \
    HANDLE_Q3DS_COLOR_PROPERTY2(Text, TextColor, Color)                                 \
    HANDLE_Q3DS_STRING_PROPERTY(Text, Font)                                             \
    HANDLE_Q3DS_FLOAT_PROPERTY(Text, Size)                                              \
    HANDLE_Q3DS_ENUM_PROPERTY2(Text, HorzAlign, HorizontalAlignment, Q3DSTextNode::HorizontalAlignment) \
    HANDLE_Q3DS_ENUM_PROPERTY2(Text, VertAlign, VerticalAlignment, Q3DSTextNode::VerticalAlignment)     \
    HANDLE_Q3DS_FLOAT_PROPERTY(Text, Leading)                                           \
    HANDLE_Q3DS_FLOAT_PROPERTY(Text, Tracking)                                          \
    HANDLE_Q3DS_NAME_PROPERTY                                                           \
    HANDLE_Q3DS_NOTIFY_CHANGES

#define HANDLE_CHANGE(list, ret, x)     \
{                                       \
    int size = list.count();            \
    list.append(x);                     \
    ret = size < list.count();          \
}

Q3DSNodeTranslator::Q3DSNodeTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSNode &node)
    : Q3DSGraphObjectTranslator(instance, node)
{

}

void Q3DSNodeTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSGraphObjectTranslator::pushTranslation(inContext);
    Q3DSNode &theItem = static_cast<Q3DSNode &>(graphObject());
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
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

void Q3DSNodeTranslator::appendChild(Q3DSGraphObject &inChild)
{
    if (inChild.isNode() == false) {
        QT3DS_ASSERT(false);
        return;
    }

    Q3DSNode &theItem = static_cast<Q3DSNode &>(graphObject());
    Q3DSNode &theChild = static_cast<Q3DSNode &>(inChild);
    theItem.appendChildNode(&theChild);
}

void Q3DSNodeTranslator::clearChildren()
{
    Q3DSNode &theItem = static_cast<Q3DSNode &>(graphObject());
    theItem.removeAllChildNodes();
}

void Q3DSNodeTranslator::setActive(bool inActive)
{
    Q3DSNode &theNode = static_cast<Q3DSNode &>(graphObject());
    if (inActive != theNode.eyeballEnabled()) {
        Q3DSPropertyChangeList list;
        list.append(theNode.setEyeballEnabled(inActive));
        theNode.notifyPropertyChanges(list);
    }
}

bool Q3DSNodeTranslator::updateProperty(Q3DSTranslation &inContext,
                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                    qt3dsdm::Qt3DSDMPropertyHandle property,
                    qt3dsdm::SValue &value, const QString &name)
{
    bool ret = false;
    if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSNode &theItem = static_cast<Q3DSNode &>(graphObject());
    if (name == QLatin1String("position")) {
        HANDLE_CHANGE(list, ret, theItem.setPosition(Q3DSValueParser::parseVector(value)))
    } else if (name == QLatin1String("rotation")) {
        HANDLE_CHANGE(list, ret, theItem.setRotation(Q3DSValueParser::parseVector(value)))
    } else if (name == QLatin1String("scale")) {
        HANDLE_CHANGE(list, ret, theItem.setScale(Q3DSValueParser::parseVector(value)))
    } else if (name == QLatin1String("pivot")) {
        HANDLE_CHANGE(list, ret, theItem.setPivot(Q3DSValueParser::parseVector(value)))
    } else if (name == QLatin1String("opacity")) {
        HANDLE_CHANGE(list, ret, theItem.setLocalOpacity(value.getData<float>()))
    } else if (name == QLatin1String("rotationorder")) {
        HANDLE_CHANGE(list, ret, theItem.setRotationOrder(Q3DSValueParser::parseEnum
                                 <Q3DSNode::RotationOrder>(value)))
    } else if (name == QLatin1String("orientation")) {
        HANDLE_CHANGE(list, ret, theItem.setOrientation(Q3DSValueParser::parseEnum
                               <Q3DSNode::Orientation>(value)))
    } else if (name == QLatin1String("boneid")) {
        HANDLE_CHANGE(list, ret, theItem.setSkeletonId(value.getData<int>()))
    } else if (name == QLatin1String("ignoresparent")) {
        HANDLE_CHANGE(list, ret, theItem.setIgnoresParent(value.getData<bool>()))
    } else if (name == QLatin1String("eyeball")) {
        HANDLE_CHANGE(list, ret, theItem.setEyeballEnabled(value.getData<bool>()))
    }
    if (ret)
        theItem.notifyPropertyChanges(list);
    return ret;
}


Q3DSGroupNodeTranslator::Q3DSGroupNodeTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                 Q3DSGroupNode &node)
    : Q3DSNodeTranslator(instance, node)
{
}


Q3DSComponentNodeTranslator::Q3DSComponentNodeTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                         Q3DSComponentNode &component)
    : Q3DSNodeTranslator(instance, component)
{
}


Q3DSSceneTranslator::Q3DSSceneTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSScene &scene)
    : Q3DSGraphObjectTranslator(instance, scene)
{

}

void Q3DSSceneTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSGraphObjectTranslator::pushTranslation(inContext);

    Q3DSScene &theItem = static_cast<Q3DSScene &>(graphObject());
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
    ITERATE_Q3DS_SCENE_PROPERTIES
    theItem.removeAllChildNodes();
    int childCount = inContext.assetGraph().GetChildCount(instanceHandle());
    QVector<Q3DSGraphObjectTranslator *> translators;
    for (long idx = 0; idx < childCount; ++idx) {
        qt3dsdm::Qt3DSDMInstanceHandle layer =
            inContext.assetGraph().GetChild(instanceHandle(), idx);
        Q3DSGraphObjectTranslator *translator = inContext.getOrCreateTranslator(layer);
        if (translator && translator->graphObject().type() == Q3DSNode::Layer) {
            Q3DSLayerNode *theLayerObj
                    = static_cast<Q3DSLayerNode *>(&translator->graphObject());
            theItem.appendChildNode(theLayerObj);
            translators << translator;
        }
    }
    theItem.resolveReferences(*inContext.presentation());
    for (auto t : qAsConst(translators))
        t->pushTranslation(inContext);
}

void Q3DSSceneTranslator::appendChild(Q3DSGraphObject &inChild)
{
    if (inChild.type() != Q3DSNode::Layer) {
        QT3DS_ASSERT(false);
        return;
    }

    Q3DSGraphObject &theItem = static_cast<Q3DSGraphObject &>(graphObject());
    Q3DSGraphObject &theChild = static_cast<Q3DSGraphObject &>(inChild);
    theItem.appendChildNode(&theChild);
}

void Q3DSSceneTranslator::clearChildren()
{
    Q3DSNode &theItem = static_cast<Q3DSNode &>(graphObject());
    theItem.removeAllChildNodes();
}
void Q3DSSceneTranslator::setActive(bool)
{

}

bool Q3DSSceneTranslator::updateProperty(Q3DSTranslation &inContext,
                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                    qt3dsdm::Qt3DSDMPropertyHandle property,
                    qt3dsdm::SValue &value,
                    const QString &name)
{
    bool ret = false;
    if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSScene &theItem = static_cast<Q3DSScene &>(graphObject());
    if (name == QLatin1String("bgcolorenable"))
        HANDLE_CHANGE(list, ret, theItem.setUseClearColor(value.getData<bool>()))
    else if (name == QLatin1String("backgroundcolor"))
        HANDLE_CHANGE(list, ret, theItem.setClearColor(Q3DSValueParser::parseColor(value)))

    if (ret)
        theItem.notifyPropertyChanges(list);
    return ret;
}


Q3DSCameraTranslator::Q3DSCameraTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                           Q3DSCameraNode &camera)
    : Q3DSNodeTranslator(instance, camera)
{

}

void Q3DSCameraTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSNodeTranslator::pushTranslation(inContext);

    Q3DSCameraNode &theItem = static_cast<Q3DSCameraNode &>(graphObject());
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
    ITERATE_Q3DS_CAMERA_PROPERTIES
    theItem.resolveReferences(*inContext.presentation());
}

bool Q3DSCameraTranslator::updateProperty(Q3DSTranslation &inContext,
                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                    qt3dsdm::Qt3DSDMPropertyHandle property,
                    qt3dsdm::SValue &value,
                    const QString &name)
{
    bool ret = false;

    // we'll handle this
    if (name != QLatin1String("eyeball")) {
        if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
            return true;
    }

    Q3DSPropertyChangeList list;
    Q3DSCameraNode &theItem = static_cast<Q3DSCameraNode &>(graphObject());
    if (name == QLatin1String("orthographic")) {
        HANDLE_CHANGE(list, ret, theItem.setOrthographic(value.getData<bool>()))
    } else if (name == QLatin1String("clipnear")) {
        HANDLE_CHANGE(list, ret, theItem.setClipNear(value.getData<float>()))
    } else if (name == QLatin1String("clipfar")) {
        HANDLE_CHANGE(list, ret, theItem.setClipFar(value.getData<float>()))
    } else if (name == QLatin1String("fov")) {
        HANDLE_CHANGE(list, ret, theItem.setFov(value.getData<float>()))
    }  else if (name == QLatin1String("scalemode")) {
        HANDLE_CHANGE(list, ret, theItem.setScaleMode(
                          Q3DSValueParser::parseEnum<Q3DSCameraNode::ScaleMode>(value)))
    } else if (name == QLatin1String("scaleanchor")) {
        HANDLE_CHANGE(list, ret, theItem.setScaleAnchor(
                          Q3DSValueParser::parseEnum<Q3DSCameraNode::ScaleAnchor>(value)))
    } else if (name == QLatin1String("eyeball")) {
        if (m_editCameraEnabled) {
            m_activeState = value.getData<bool>();
            return true;
        } else {
            return Q3DSNodeTranslator::updateProperty(inContext, instance,
                                                      property, value, name);
        }
    }
    if (ret)
        theItem.notifyPropertyChanges(list);
    return ret;
}

void Q3DSCameraTranslator::setActive(bool inActive)
{
    if (m_editCameraEnabled)
        m_activeState = inActive;
}

void Q3DSCameraTranslator::setEditCameraEnabled(bool enabled)
{
    if (m_editCameraEnabled != enabled) {
        Q3DSCameraNode &theItem = static_cast<Q3DSCameraNode &>(graphObject());
        m_editCameraEnabled = enabled;
        Q3DSPropertyChangeList list;
        if (enabled) {
            m_activeState = theItem.eyeballEnabled();
            list.append(theItem.setEyeballEnabled(false));
        } else {
            list.append(theItem.setEyeballEnabled(m_activeState));
        }
        theItem.notifyPropertyChanges(list);
    }
}


Q3DSLightTranslator::Q3DSLightTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                         Q3DSLightNode &light)
    : Q3DSNodeTranslator(instance, light)
{

}

void Q3DSLightTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSNodeTranslator::pushTranslation(inContext);

    Q3DSLightNode &theItem = static_cast<Q3DSLightNode &>(graphObject());
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
    ITERATE_Q3DS_LIGHT_PROPERTIES
    theItem.resolveReferences(*inContext.presentation());
}

bool Q3DSLightTranslator::updateProperty(Q3DSTranslation &inContext,
                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                    qt3dsdm::Qt3DSDMPropertyHandle property,
                    qt3dsdm::SValue &value,
                    const QString &name)
{
    bool ret = false;
    if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSLightNode &theItem = static_cast<Q3DSLightNode &>(graphObject());
    if (name == QLatin1String("scope")) {
        HANDLE_CHANGE(list, ret, theItem.setScope(
                          Q3DSValueParser::parseObjectRef(&inContext, instance, value)))
        theItem.resolveReferences(*inContext.presentation());
    } else if (name == QLatin1String("lighttype")) {
        HANDLE_CHANGE(list, ret, theItem.setLightType
                      (Q3DSValueParser::parseEnum<Q3DSLightNode::LightType>(value)))
    } else if (name == QLatin1String("lightdiffuse")) {
        HANDLE_CHANGE(list, ret, theItem.setDiffuse(Q3DSValueParser::parseColor(value)))
    } else if (name == QLatin1String("lightspecular")) {
        HANDLE_CHANGE(list, ret, theItem.setSpecular(Q3DSValueParser::parseColor(value)))
    } else if (name == QLatin1String("lightambient")) {
        HANDLE_CHANGE(list, ret, theItem.setAmbient(Q3DSValueParser::parseColor(value)))
    } else if (name == QLatin1String("brightness")) {
        HANDLE_CHANGE(list, ret, theItem.setBrightness(value.getData<float>()))
    } else if (name == QLatin1String("linearfade")) {
        HANDLE_CHANGE(list, ret, theItem.setLinearFade(value.getData<float>()))
    } else if (name == QLatin1String("expfade")) {
        HANDLE_CHANGE(list, ret, theItem.setExpFade(value.getData<float>()))
    } else if (name == QLatin1String("areawidth")) {
        HANDLE_CHANGE(list, ret, theItem.setAreaWidth(value.getData<float>()))
    } else if (name == QLatin1String("areaheight")) {
        HANDLE_CHANGE(list, ret, theItem.setAreaHeight(value.getData<float>()))
    } else if (name == QLatin1String("castshadow")) {
        HANDLE_CHANGE(list, ret, theItem.setCastShadow(value.getData<bool>()))
    } else if (name == QLatin1String("shdwfactor")) {
        HANDLE_CHANGE(list, ret, theItem.setShadowFactor(value.getData<float>()))
    } else if (name == QLatin1String("shdwfilter")) {
        HANDLE_CHANGE(list, ret, theItem.setShadowFilter(value.getData<float>()))
    } else if (name == QLatin1String("shdwmapres")) {
        HANDLE_CHANGE(list, ret, theItem.setShadowMapRes(value.getData<int>()))
    } else if (name == QLatin1String("shdwbias")) {
        HANDLE_CHANGE(list, ret, theItem.setShadowBias(value.getData<float>()))
    } else if (name == QLatin1String("shdwmapfar")) {
        HANDLE_CHANGE(list, ret, theItem.setShadowMapFar(value.getData<float>()))
    } else if (name == QLatin1String("shdwmapfov")) {
        HANDLE_CHANGE(list, ret, theItem.setShadowMapFov(value.getData<float>()))
    }
    if (ret)
        theItem.notifyPropertyChanges(list);
    return ret;
}


Q3DSModelTranslator::Q3DSModelTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                         Q3DSModelNode &model)
    : Q3DSNodeTranslator(instance, model)
{

}

bool Q3DSModelTranslator::canAddChild(Q3DSGraphObjectTranslator *child)
{
    if (!child->graphObject().parent())
        return true;
    Q_ASSERT_X(child->graphObject().parent() == &graphObject(),
               __FUNCTION__, "Child has another parent");
    return false;
}

void Q3DSModelTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSNodeTranslator::pushTranslation(inContext);

    Q3DSModelNode &theItem = static_cast<Q3DSModelNode &>(graphObject());
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
    ITERATE_Q3DS_MODEL_PROPERTIES

    int childCount = inContext.assetGraph().GetChildCount(instanceHandle());
    for (int i = 0; i < childCount; ++i) {
        qt3dsdm::Qt3DSDMInstanceHandle childInstance
                = inContext.assetGraph().GetChild(instanceHandle(), i);
        Q3DSGraphObjectTranslator *childTranslator
                = inContext.getOrCreateTranslator(childInstance);
        if (childTranslator) {
            if (isMaterial(childTranslator->graphObject()) && canAddChild(childTranslator)) {
                theItem.appendChildNode(&childTranslator->graphObject());
                childTranslator->pushTranslation(inContext);
            }
        }
    }
    theItem.resolveReferences(*inContext.presentation());
}

bool Q3DSModelTranslator::isMaterial(const Q3DSGraphObject &inChild) const
{
    if (inChild.type() == Q3DSGraphObject::ReferencedMaterial ||
        inChild.type() == Q3DSGraphObject::DefaultMaterial ||
        inChild.type() == Q3DSGraphObject::CustomMaterial) {
        return true;
    }
    return false;
}

void Q3DSModelTranslator::appendChild(Q3DSGraphObject &inChild)
{
    if (inChild.isNode() || isMaterial(inChild))
        Q3DSNodeTranslator::appendChild(inChild);
    else
        QT3DS_ASSERT(false);
}

bool Q3DSModelTranslator::updateProperty(Q3DSTranslation &inContext,
                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                    qt3dsdm::Qt3DSDMPropertyHandle property,
                    qt3dsdm::SValue &value,
                    const QString &name)
{
    bool ret = false;
    if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSModelNode &theItem = static_cast<Q3DSModelNode &>(graphObject());
    if (name == QLatin1String("sourcepath")) {
        HANDLE_CHANGE(list, ret, theItem.setMesh(value.toQVariant().toString()))
        theItem.resolveReferences(*inContext.presentation());
    } else if (name == QLatin1String("poseroot")) {
        HANDLE_CHANGE(list, ret, theItem.setSkeletonRoot(value.getData<int>()))
    } else if (name == QLatin1String("tessellation")) {
        HANDLE_CHANGE(list, ret, theItem.setTessellation(
                          Q3DSValueParser::parseEnum<Q3DSModelNode::Tessellation>(value)))
    } else if (name == QLatin1String("edgetess")) {
        HANDLE_CHANGE(list, ret, theItem.setEdgeTess(value.getData<float>()))
    } else if (name == QLatin1String("innertess")) {
        HANDLE_CHANGE(list, ret, theItem.setInnerTess(value.getData<float>()))
    }
    if (ret)
        theItem.notifyPropertyChanges(list);
    return ret;
}


Q3DSImageTranslator::Q3DSImageTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSImage &image)
    : Q3DSGraphObjectTranslator(instance, image)
{

}

void Q3DSImageTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSGraphObjectTranslator::pushTranslation(inContext);

    Q3DSImage &theItem = static_cast<Q3DSImage &>(graphObject());
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
    ITERATE_Q3DS_IMAGE_PROPERTIES
    theItem.resolveReferences(*inContext.presentation());
}

void Q3DSImageTranslator::appendChild(Q3DSGraphObject &)
{

}

void Q3DSImageTranslator::setActive(bool)
{
}

void Q3DSImageTranslator::clearChildren()
{
}

bool Q3DSImageTranslator::updateProperty(Q3DSTranslation &inContext,
                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                    qt3dsdm::Qt3DSDMPropertyHandle property,
                    qt3dsdm::SValue &value,
                    const QString &name)
{
    bool ret = false;
    if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSImage &theItem = static_cast<Q3DSImage &>(graphObject());
    if (name == QLatin1String("scaleu")) {
        HANDLE_CHANGE(list, ret, theItem.setScaleU(value.getData<float>()))
    } else if (name == QLatin1String("scalev")) {
        HANDLE_CHANGE(list, ret, theItem.setScaleV(value.getData<float>()))
    } else if (name == QLatin1String("subpresentation")) {
        HANDLE_CHANGE(list, ret, theItem.setSubPresentation(value.toQVariant().toString()))
        theItem.resolveReferences(*inContext.presentation());
    } else if (name == QLatin1String("mappingmode")) {
        HANDLE_CHANGE(list, ret, theItem.setMappingMode(
                          Q3DSValueParser::parseEnum<Q3DSImage::MappingMode>(value)))
    } else if (name == QLatin1String("tilingmodehorz")) {
        HANDLE_CHANGE(list, ret, theItem.setHorizontalTiling(
                          Q3DSValueParser::parseEnum<Q3DSImage::TilingMode>(value)))
    } else if (name == QLatin1String("tilingmodevert")) {
        HANDLE_CHANGE(list, ret, theItem.setVerticalTiling(
                          Q3DSValueParser::parseEnum<Q3DSImage::TilingMode>(value)))
    } else if (name == QLatin1String("rotationuv")) {
        HANDLE_CHANGE(list, ret, theItem.setRotationUV(value.getData<float>()))
    } else if (name == QLatin1String("positionu")) {
        HANDLE_CHANGE(list, ret, theItem.setPositionU(value.getData<float>()))
    } else if (name == QLatin1String("positionv")) {
        HANDLE_CHANGE(list, ret, theItem.setPositionV(value.getData<float>()))
    } else if (name == QLatin1String("pivotu")) {
        HANDLE_CHANGE(list, ret, theItem.setPivotU(value.getData<float>()))
    } else if (name == QLatin1String("pivotv")) {
        HANDLE_CHANGE(list, ret, theItem.setPivotV(value.getData<float>()))
    }
    if (ret)
        theItem.notifyPropertyChanges(list);
    return ret;
}


Q3DSDefaultMaterialTranslator::Q3DSDefaultMaterialTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                             Q3DSDefaultMaterial &material)
    : Q3DSGraphObjectTranslator(instance, material)
{

}

void Q3DSDefaultMaterialTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSGraphObjectTranslator::pushTranslation(inContext);

    Q3DSDefaultMaterial &theItem = static_cast<Q3DSDefaultMaterial &>(graphObject());
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
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

void Q3DSDefaultMaterialTranslator::setActive(bool)
{
}

void Q3DSDefaultMaterialTranslator::clearChildren()
{
}

void Q3DSDefaultMaterialTranslator::appendChild(Q3DSGraphObject &)
{
}

bool Q3DSDefaultMaterialTranslator::updateProperty(Q3DSTranslation &inContext,
                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                    qt3dsdm::Qt3DSDMPropertyHandle property,
                    qt3dsdm::SValue &value,
                    const QString &name)
{
    bool ret = false;
    if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSDefaultMaterial &theItem = static_cast<Q3DSDefaultMaterial &>(graphObject());
    if (name == QLatin1String("shaderlighting")) {
        HANDLE_CHANGE(list, ret, theItem.setShaderLighting(
                          Q3DSValueParser::parseEnum<
                                                Q3DSDefaultMaterial::ShaderLighting>(value)))
    } else if (name == QLatin1String("blendmode")) {
        HANDLE_CHANGE(list, ret, theItem.setBlendMode(
                          Q3DSValueParser::parseEnum<Q3DSDefaultMaterial::BlendMode>(value)))
    }/* else if (name == QLatin1String("vertexcolors")) {
        ret = true;
    } */else if (name == QLatin1String("diffuse")) {
        HANDLE_CHANGE(list, ret, theItem.setDiffuse(Q3DSValueParser::parseColor(value)))
    } else if (name == QLatin1String("diffusemap")) {
        HANDLE_CHANGE(list, ret, theItem.setDiffuseMap(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("diffusemap2")) {
        HANDLE_CHANGE(list, ret, theItem.setDiffuseMap2(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("diffusemap3")) {
        HANDLE_CHANGE(list, ret, theItem.setDiffuseMap3(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("specularreflection")) {
        HANDLE_CHANGE(list, ret, theItem.setSpecularReflection(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("speculartint")) {
        HANDLE_CHANGE(list, ret, theItem.setSpecularTint(Q3DSValueParser::parseColor(value)))
    } else if (name == QLatin1String("specularamount")) {
        HANDLE_CHANGE(list, ret, theItem.setSpecularAmount(value.getData<float>()))
    } else if (name == QLatin1String("specularmap")) {
        HANDLE_CHANGE(list, ret, theItem.setSpecularMap(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("specularmodel")) {
        HANDLE_CHANGE(list, ret, theItem.setSpecularModel(
                          Q3DSValueParser::parseEnum<Q3DSDefaultMaterial::SpecularModel>(value)))
    } else if (name == QLatin1String("specularroughness")) {
        HANDLE_CHANGE(list, ret, theItem.setSpecularRoughness(value.getData<float>()))
    } else if (name == QLatin1String("roughnessmap")) {
        HANDLE_CHANGE(list, ret, theItem.setRoughnessMap(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("fresnelPower")) {
        HANDLE_CHANGE(list, ret, theItem.setFresnelPower(value.getData<float>()))
    } else if (name == QLatin1String("ior")) {
        HANDLE_CHANGE(list, ret, theItem.setIor(value.getData<float>()))
    } else if (name == QLatin1String("bumpmap")) {
        HANDLE_CHANGE(list, ret, theItem.setBumpMap(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("normalmap")) {
        HANDLE_CHANGE(list, ret, theItem.setNormalMap(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("bumpamount")) {
        HANDLE_CHANGE(list, ret, theItem.setBumpAmount(value.getData<float>()))
    } else if (name == QLatin1String("displacementmap")) {
        HANDLE_CHANGE(list, ret, theItem.setDisplacementMap(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("displaceamount")) {
        HANDLE_CHANGE(list, ret, theItem.setDisplaceAmount(value.getData<float>()))
    } else if (name == QLatin1String("opacity")) {
        HANDLE_CHANGE(list, ret, theItem.setOpacity(value.getData<float>()))
    } else if (name == QLatin1String("opacitymap")) {
        HANDLE_CHANGE(list, ret, theItem.setOpacityMap(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("emissivecolor")) {
        HANDLE_CHANGE(list, ret, theItem.setEmissiveColor(
                          Q3DSValueParser::parseColor(value)))
    } else if (name == QLatin1String("emissivepower")) {
        HANDLE_CHANGE(list, ret, theItem.setEmissivePower(value.getData<float>()))
    } else if (name == QLatin1String("emissivemap")) {
        HANDLE_CHANGE(list, ret, theItem.setEmissiveMap(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("emissivemap2")) {
        HANDLE_CHANGE(list, ret, theItem.setEmissiveMap2(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("translucencymap")) {
        HANDLE_CHANGE(list, ret, theItem.setTranslucencyMap(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("translucentfalloff")) {
        HANDLE_CHANGE(list, ret, theItem.setTranslucentFalloff(value.getData<float>()))
    } else if (name == QLatin1String("diffuselightwrap")) {
        HANDLE_CHANGE(list, ret, theItem.setDiffuseLightWrap(value.getData<float>()))
    } else if (name == QLatin1String("iblprobe")) {
        HANDLE_CHANGE(list, ret, theItem.setLightProbe(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("lightmapindirect")) {
        HANDLE_CHANGE(list, ret, theItem.setLightmapIndirectMap(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("lightmapradiosity")) {
        HANDLE_CHANGE(list, ret, theItem.setLightmapRadiosityMap(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("lightmapshadow")) {
        HANDLE_CHANGE(list, ret, theItem.setLightmapShadowMap(
                          Q3DSValueParser::parseImage(&inContext, value)))
    }
    if (ret)
        theItem.notifyPropertyChanges(list);
    return ret;
}


Q3DSReferencedMaterialTranslator::Q3DSReferencedMaterialTranslator(
        qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSReferencedMaterial &material)
    : Q3DSGraphObjectTranslator(instance, material)
{

}

void Q3DSReferencedMaterialTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSGraphObjectTranslator::pushTranslation(inContext);

    Q3DSReferencedMaterial &theItem = static_cast<Q3DSReferencedMaterial &>(graphObject());
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
    ITERATE_Q3DS_REFERENCED_MATERIAL_PROPERTIES

    if (theItem.referencedMaterial() == &theItem) {
        qCCritical(qt3ds::INVALID_OPERATION, "Referenced material is referencing itself.");
    } else {
        if (theItem.lightmapIndirectMap())
            theItem.appendChildNode(theItem.lightmapIndirectMap());
        if (theItem.lightmapRadiosityMap())
            theItem.appendChildNode(theItem.lightmapRadiosityMap());
        if (theItem.lightmapShadowMap())
            theItem.appendChildNode(theItem.lightmapShadowMap());
    }
    theItem.resolveReferences(*inContext.presentation());
}

void Q3DSReferencedMaterialTranslator::setActive(bool)
{
}

void Q3DSReferencedMaterialTranslator::clearChildren()
{
}

void Q3DSReferencedMaterialTranslator::appendChild(Q3DSGraphObject &)
{
}

bool Q3DSReferencedMaterialTranslator::updateProperty(Q3DSTranslation &inContext,
                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                    qt3dsdm::Qt3DSDMPropertyHandle property,
                    qt3dsdm::SValue &value,
                    const QString &name)
{
    bool ret = false;
    if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSReferencedMaterial &theItem = static_cast<Q3DSReferencedMaterial &>(graphObject());
    if (name == QLatin1String("referencedmaterial")) {
        HANDLE_CHANGE(list, ret, theItem.setReferencedMaterial(
                          Q3DSValueParser::parseObjectRef(&inContext, instance, value)))
    }
    if (ret)
        theItem.notifyPropertyChanges(list);
    return ret;
}


Q3DSLayerTranslator::Q3DSLayerTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                         Q3DSLayerNode &layer)
    : Q3DSNodeTranslator(instance, layer)
{

}

void Q3DSLayerTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSNodeTranslator::pushTranslation(inContext);

    Q3DSLayerNode &theItem = static_cast<Q3DSLayerNode &>(graphObject());
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
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

void Q3DSLayerTranslator::appendChild(Q3DSGraphObject &inChild)
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

bool Q3DSLayerTranslator::updateProperty(Q3DSTranslation &inContext,
                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                    qt3dsdm::Qt3DSDMPropertyHandle property,
                    qt3dsdm::SValue &value,
                    const QString &name)
{
    bool ret = false;
    if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSLayerNode &theItem = static_cast<Q3DSLayerNode &>(graphObject());
    if (name == QLatin1String("disabledepthtest")) {
        HANDLE_CHANGE(list, ret, theItem.setDepthTestDisabled(value.getData<bool>()))
    } else if (name == QLatin1String("disabledepthprepass")) {
        HANDLE_CHANGE(list, ret, theItem.setDepthPrePassDisabled(value.getData<bool>()))
    } else if (name == QLatin1String("progressiveaa")) {
        HANDLE_CHANGE(list, ret, theItem.setProgressiveAA(Q3DSValueParser::parseEnum
                                 <Q3DSLayerNode::ProgressiveAA>(value)))
    } else if (name == QLatin1String("multisampleaa")) {
        HANDLE_CHANGE(list, ret, theItem.setMultisampleAA(Q3DSValueParser::parseEnum
                                 <Q3DSLayerNode::MultisampleAA>(value)))
    } else if (name == QLatin1String("temporalaa")) {
        HANDLE_CHANGE(list, ret, theItem.setTemporalAAEnabled(value.getData<bool>()))
    } else if (name == QLatin1String("background")) {
        HANDLE_CHANGE(list, ret, theItem.setLayerBackground(Q3DSValueParser::parseEnum
                                   <Q3DSLayerNode::LayerBackground>(value)))
    } else if (name == QLatin1String("backgroundcolor")) {
        HANDLE_CHANGE(list, ret, theItem.setBackgroundColor(Q3DSValueParser::parseColor(value)))
    } else if (name == QLatin1String("blendtype")) {
        HANDLE_CHANGE(list, ret, theItem.setBlendType(Q3DSValueParser::parseEnum
                                   <Q3DSLayerNode::BlendType>(value)))
    } else if (name == QLatin1String("horzfields")) {
        HANDLE_CHANGE(list, ret, theItem.setHorizontalFields(Q3DSValueParser::parseEnum
                                    <Q3DSLayerNode::HorizontalFields>(value)))
    } else if (name == QLatin1String("left")) {
        HANDLE_CHANGE(list, ret, theItem.setLeft(value.getData<float>()))
    } else if (name == QLatin1String("leftunits")) {
        HANDLE_CHANGE(list, ret, theItem.setLeftUnits(Q3DSValueParser::parseEnum
                                    <Q3DSLayerNode::Units>(value)))
    } else if (name == QLatin1String("width")) {
        HANDLE_CHANGE(list, ret, theItem.setWidth(value.getData<float>()))
    } else if (name == QLatin1String("widthunits")) {
        HANDLE_CHANGE(list, ret, theItem.setLeftUnits(Q3DSValueParser::parseEnum
                                    <Q3DSLayerNode::Units>(value)))
    } else if (name == QLatin1String("right")) {
        HANDLE_CHANGE(list, ret, theItem.setRight(value.getData<float>()))
    } else if (name == QLatin1String("rightunits")) {
        HANDLE_CHANGE(list, ret, theItem.setRightUnits(Q3DSValueParser::parseEnum
                                    <Q3DSLayerNode::Units>(value)))
    } else if (name == QLatin1String("vertfields")) {
        HANDLE_CHANGE(list, ret, theItem.setVerticalFields(Q3DSValueParser::parseEnum
                                    <Q3DSLayerNode::VerticalFields>(value)))
    } else if (name == QLatin1String("top")) {
        HANDLE_CHANGE(list, ret, theItem.setTop(value.getData<float>()))
    } else if (name == QLatin1String("topunits")) {
        HANDLE_CHANGE(list, ret, theItem.setTopUnits(Q3DSValueParser::parseEnum
                                    <Q3DSLayerNode::Units>(value)))
    } else if (name == QLatin1String("height")) {
        HANDLE_CHANGE(list, ret, theItem.setHeight(value.getData<float>()))
    } else if (name == QLatin1String("heightunits")) {
        HANDLE_CHANGE(list, ret, theItem.setHeightUnits(Q3DSValueParser::parseEnum
                                    <Q3DSLayerNode::Units>(value)))
    } else if (name == QLatin1String("bottom")) {
        HANDLE_CHANGE(list, ret, theItem.setBottom(value.getData<float>()))
    } else if (name == QLatin1String("bottomunits")) {
        HANDLE_CHANGE(list, ret, theItem.setBottomUnits(Q3DSValueParser::parseEnum
                                    <Q3DSLayerNode::Units>(value)))
    } else if (name == QLatin1String("sourcepath")) {
        HANDLE_CHANGE(list, ret, theItem.setSourcePath(value.toQVariant().toString()))
        theItem.resolveReferences(*inContext.presentation());
    } else if (name == QLatin1String("aostrength")) {
        HANDLE_CHANGE(list, ret, theItem.setAoStrength(value.getData<float>()))
    } else if (name == QLatin1String("aodistance")) {
        HANDLE_CHANGE(list, ret, theItem.setAoDistance(value.getData<float>()))
    } else if (name == QLatin1String("aosoftness")) {
        HANDLE_CHANGE(list, ret, theItem.setAoSoftness(value.getData<float>()))
    } else if (name == QLatin1String("aobias")) {
        HANDLE_CHANGE(list, ret, theItem.setAoBias(value.getData<float>()))
    } else if (name == QLatin1String("aosamplerate")) {
        HANDLE_CHANGE(list, ret, theItem.setAoSampleRate(value.getData<int>()))
    } else if (name == QLatin1String("aodither")) {
        HANDLE_CHANGE(list, ret, theItem.setAoDither(value.getData<bool>()))
    } else if (name == QLatin1String("shadowstrength")) {
        HANDLE_CHANGE(list, ret, theItem.setShadowStrength(value.getData<float>()))
    } else if (name == QLatin1String("shadowdist")) {
        HANDLE_CHANGE(list, ret, theItem.setShadowDist(value.getData<float>()))
    } else if (name == QLatin1String("shadowsoftness")) {
        HANDLE_CHANGE(list, ret, theItem.setShadowSoftness(value.getData<float>()))
    } else if (name == QLatin1String("shadowbias")) {
        HANDLE_CHANGE(list, ret, theItem.setShadowBias(value.getData<float>()))
    } else if (name == QLatin1String("lightprobe")) {
        HANDLE_CHANGE(list, ret, theItem.setLightProbe(
                          Q3DSValueParser::parseImage(&inContext, value)))
    } else if (name == QLatin1String("probebright")) {
        HANDLE_CHANGE(list, ret, theItem.setBottom(value.getData<float>()))
    } else if (name == QLatin1String("fastibl")) {
        HANDLE_CHANGE(list, ret, theItem.setBottom(value.getData<float>()))
    } else if (name == QLatin1String("probehorizon")) {
        HANDLE_CHANGE(list, ret, theItem.setBottom(value.getData<float>()))
    } else if (name == QLatin1String("probefov")) {
        HANDLE_CHANGE(list, ret, theItem.setBottom(value.getData<float>()))
    } else if (name == QLatin1String("lightprobe2")) {
        HANDLE_CHANGE(list, ret, theItem.setBottom(value.getData<float>()))
    } else if (name == QLatin1String("probe2fade")) {
        HANDLE_CHANGE(list, ret, theItem.setBottom(value.getData<float>()))
    } else if (name == QLatin1String("probe2window")) {
        HANDLE_CHANGE(list, ret, theItem.setBottom(value.getData<float>()))
    } else if (name == QLatin1String("probe2pos")) {
        HANDLE_CHANGE(list, ret, theItem.setBottom(value.getData<float>()))
    }
    if (ret)
        theItem.notifyPropertyChanges(list);
    return ret;
}


Q3DSSlideTranslator::Q3DSSlideTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSSlide &slide)
    : Q3DSGraphObjectTranslator(instance, slide), m_isMaster(false)
{

}

void Q3DSSlideTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSGraphObjectTranslator::pushTranslation(inContext);

    Q3DSSlide &theItem = static_cast<Q3DSSlide &>(graphObject());
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
    ITERATE_Q3DS_SLIDE_PROPERTIES

    Q3DSSlide::PlayThrough playthrough;
    QVariant value;
    if (theParser.parsePlaythroughToProperty(HANDLE_PROPERTY(Slide, PlaythroughTo),
                                             playthrough, value)) {
        list.append(theItem.setPlayThrough(playthrough));
        if (playthrough == Q3DSSlide::Value)
            list.append(theItem.setPlayThroughValue(value));
    }
    HANDLE_Q3DS_NOTIFY_CHANGES
    theItem.resolveReferences(*inContext.presentation());
}

void Q3DSSlideTranslator::appendChild(Q3DSGraphObject &inChild)
{
    if (inChild.type() != Q3DSNode::Slide && !masterSlide()) {
        QT3DS_ASSERT(false);
        return;
    }

    graphObject().appendChildNode(&inChild);
}

void Q3DSSlideTranslator::clearChildren()
{

}

void Q3DSSlideTranslator::setActive(bool)
{

}

bool Q3DSSlideTranslator::updateProperty(Q3DSTranslation &inContext,
                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                    qt3dsdm::Qt3DSDMPropertyHandle property,
                    qt3dsdm::SValue &value,
                    const QString &name)
{
    bool ret = false;
    if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSSlide &theItem = static_cast<Q3DSSlide &>(graphObject());
    if (name == QLatin1String("playmode")) {
        HANDLE_CHANGE(list, ret, theItem.setPlayMode(
                          Q3DSValueParser::parseEnum<Q3DSSlide::PlayMode>(value)))
    } else if (name == QLatin1String("playthroughto")) {
        HANDLE_CHANGE(list, ret, theItem.setPlayThrough(
                          Q3DSValueParser::parseEnum<Q3DSSlide::PlayThrough>(value)))
    } else if (name == QLatin1String("initialplaystate")) {
        HANDLE_CHANGE(list, ret, theItem.setInitialPlayState(
                          Q3DSValueParser::parseEnum<Q3DSSlide::InitialPlayState>(value)))
    }
    if (ret)
        theItem.notifyPropertyChanges(list);
    return ret;
}

bool Q3DSSlideTranslator::masterSlide() const
{
    return m_isMaster;
}

void Q3DSSlideTranslator::setMasterSlide(bool master)
{
    m_isMaster = master;
}


Q3DSTextTranslator::Q3DSTextTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DSTextNode &node)
    : Q3DSNodeTranslator(instance, node)
{

}

void Q3DSTextTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSNodeTranslator::pushTranslation(inContext);

    Q3DSTextNode &theItem = static_cast<Q3DSTextNode &>(graphObject());
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
    ITERATE_Q3DS_TEXT_PROPERTIES
}

bool Q3DSTextTranslator::updateProperty(Q3DSTranslation &inContext,
                    qt3dsdm::Qt3DSDMInstanceHandle instance,
                    qt3dsdm::Qt3DSDMPropertyHandle property,
                    qt3dsdm::SValue &value,
                    const QString &name)
{
    bool ret = false;
    if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSTextNode &theItem = static_cast<Q3DSTextNode &>(graphObject());
    if (name == QLatin1String("textstring")) {
        HANDLE_CHANGE(list, ret, theItem.setText(value.toQVariant().toString()))
    } else if (name == QLatin1String("textcolor")) {
        HANDLE_CHANGE(list, ret, theItem.setColor(Q3DSValueParser::parseColor(value)))
    } else if (name == QLatin1String("font")) {
        HANDLE_CHANGE(list, ret, theItem.setFont(value.toQVariant().toString()))
    } else if (name == QLatin1String("size")) {
        HANDLE_CHANGE(list, ret, theItem.setSize(value.getData<float>()))
    } else if (name == QLatin1String("horzalign")) {
        HANDLE_CHANGE(list, ret, theItem.setHorizontalAlignment(
                          Q3DSValueParser::parseEnum<Q3DSTextNode::HorizontalAlignment>(value)))
    } else if (name == QLatin1String("vertalign")) {
        HANDLE_CHANGE(list, ret, theItem.setVerticalAlignment(
                          Q3DSValueParser::parseEnum<Q3DSTextNode::VerticalAlignment>(value)))
    } else if (name == QLatin1String("leading")) {
        HANDLE_CHANGE(list, ret, theItem.setLeading(value.getData<float>()))
    } else if (name == QLatin1String("tracking")) {
        HANDLE_CHANGE(list, ret, theItem.setTracking(value.getData<float>()))
    }
    if (ret)
        theItem.notifyPropertyChanges(list);
    return ret;
}

}
