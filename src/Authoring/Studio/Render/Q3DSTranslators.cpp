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
#include "StudioCoreSystem.h"

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

    static Q3DSImage *parseImage(Q3DSTranslation *context, const SValue &value,
                                 Q3DSGraphObject *parent)
    {
        qt3dsdm::SLong4 guid = value.getData<qt3dsdm::SLong4>();
        qt3dsdm::Qt3DSDMInstanceHandle theInstance(
            context->reader().GetInstanceForGuid(guid));
        Q3DSGraphObjectTranslator *imageTranslator = context->getOrCreateTranslator(theInstance);
        if (imageTranslator && imageTranslator->graphObject().type() == Q3DSNode::Image) {
            Q3DSImage *theNewImage = static_cast<Q3DSImage *>(&imageTranslator->graphObject());
            if (!theNewImage->parent())
                parent->appendChildNode(theNewImage);
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
                QString theDirectory
                        = g_StudioApp.GetCore()->GetDoc()->GetDocumentDirectory();
                QString theResolvedPath
                        = Q3DStudio::CFilePath::CombineBaseAndRelative(theDirectory, outValue);
                if (QFileInfo(theResolvedPath).exists())
                    outValue = theResolvedPath;
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
            QString str = !isInt ? temp->m_Value.getData<qt3dsdm::TDataStrPtr>()->toQString()
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
    bool parseProperty(Qt3DSDMPropertyHandle inProperty, qt3dsdm::SObjectRefType &ioObjRef) const
    {
        Option<qt3dsdm::SObjectRefType> theData
                = propertyValue<qt3dsdm::SObjectRefType>(inProperty);
        if (theData.hasValue()) {
            ioObjRef = *theData;
            return true;
        }
        return false;
    }

};

#define HANDLE_PREFIX(temp_type) \
    { \
        temp_type temp;
#define DEFAULT_HANDLE_SUFFIX(name) \
        list.append(theItem.set##name(temp)); \
    }
#define HANDLE_SUFFIX(name) \
    DEFAULT_HANDLE_SUFFIX(name)

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

#define ITERATE_Q3DS_LAYER_PROPERTIES(editMode)                                   \
    HANDLE_Q3DS_BOOL_PROPERTY2(Layer, DisableDepthTest, DepthTestDisabled)        \
    HANDLE_Q3DS_BOOL_PROPERTY2(Layer, DisableDepthPrepass, DepthPrePassDisabled)  \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, ProgressiveAA, Q3DSLayerNode::ProgressiveAA) \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, MultisampleAA, Q3DSLayerNode::MultisampleAA) \
    HANDLE_Q3DS_BOOL_PROPERTY2(Layer, TemporalAA, TemporalAAEnabled)              \
    HANDLE_Q3DS_COLOR_PROPERTY(Layer, BackgroundColor)                            \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, BlendType, Q3DSLayerNode::BlendType)         \
    HANDLE_Q3DS_ENUM_PROPERTY2(Layer, Background, LayerBackground,                \
        Q3DSLayerNode::LayerBackground)                                           \
    HANDLE_Q3DS_SOURCEPATH_PROPERTY(Asset, SourcePath)                            \
    HANDLE_Q3DS_ENUM_PROPERTY2(Layer, HorizontalFieldValues, HorizontalFields,    \
        Q3DSLayerNode::HorizontalFields)                                          \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Left)                                       \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, LeftUnits, Q3DSLayerNode::Units)             \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Width)                                      \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, WidthUnits, Q3DSLayerNode::Units)            \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Right)                                      \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, RightUnits, Q3DSLayerNode::Units)            \
    HANDLE_Q3DS_ENUM_PROPERTY2(Layer, VerticalFieldValues, VerticalFields,        \
        Q3DSLayerNode::VerticalFields)                                            \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Top)                                        \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, TopUnits, Q3DSLayerNode::Units)              \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Height)                                     \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, HeightUnits, Q3DSLayerNode::Units)           \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Bottom)                                     \
    HANDLE_Q3DS_ENUM_PROPERTY(Layer, BottomUnits, Q3DSLayerNode::Units)           \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, AoStrength)                                 \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, AoDistance)                                 \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, AoSoftness)                                 \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, AoBias)                                     \
    HANDLE_Q3DS_INT_PROPERTY2(Layer, AoSamplerate, AoSampleRate)                  \
    HANDLE_Q3DS_BOOL_PROPERTY(Layer, AoDither)                                    \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ShadowStrength)                             \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ShadowDist)                                 \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ShadowSoftness)                             \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ShadowBias)                                 \
    HANDLE_Q3DS_IMAGE_PROPERTY(Layer, LightProbe)                                 \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ProbeBright)                                \
    HANDLE_Q3DS_BOOL_PROPERTY2(Layer, FastIbl, FastIBLEnabled)                    \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ProbeHorizon)                               \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, ProbeFov)                                   \
    HANDLE_Q3DS_IMAGE_PROPERTY(Layer, LightProbe2)                                \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Probe2Fade)                                 \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Probe2Window)                               \
    HANDLE_Q3DS_FLOAT_PROPERTY(Layer, Probe2Pos)                                  \
    if (!editMode)                                                                \
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

// TODO: Add dropshadow text properties
// TODO: Add text bounding/wrap properties

#define ITERATE_Q3DS_EFFECT_PROPERTIES                                                  \
    HANDLE_Q3DS_BOOL_PROPERTY2(Asset, Eyeball, EyeballEnabled)                          \
    HANDLE_Q3DS_NOTIFY_CHANGES

#define ITERATE_Q3DS_CUSTOM_MATERIAL_PROPERTIES                                         \
    HANDLE_Q3DS_IMAGE_PROPERTY2(MaterialBase, IblProbe, LightProbe)                     \
    HANDLE_Q3DS_IMAGE_PROPERTY2(Lightmaps, LightmapIndirect, LightmapIndirectMap)       \
    HANDLE_Q3DS_IMAGE_PROPERTY2(Lightmaps, LightmapRadiosity, LightmapRadiosityMap)     \
    HANDLE_Q3DS_IMAGE_PROPERTY2(Lightmaps, LightmapShadow, LightmapShadowMap)           \
    HANDLE_Q3DS_NOTIFY_CHANGES

const QSet<QString> Q3DSDefaultMaterialTranslator::s_recompileProperties
{
    QStringLiteral("shaderlighting"),
    QStringLiteral("diffusemap"),
    QStringLiteral("diffusemap2"),
    QStringLiteral("diffusemap3"),
    QStringLiteral("specularreflection"),
    QStringLiteral("specularmap"),
    QStringLiteral("specularmodel"),
    QStringLiteral("specularroughness"),
    QStringLiteral("roughnessmap"),
    QStringLiteral("bumpmap"),
    QStringLiteral("normalmap"),
    QStringLiteral("displacementmap"),
    QStringLiteral("opacitymap"),
    QStringLiteral("emissivemap"),
    QStringLiteral("emissivemap2"),
    QStringLiteral("translucencymap"),
    QStringLiteral("iblprobe")
};

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
        if (translator && translator->graphObject().isNode()
                && translator->graphObject().parent() == nullptr) {
            theItem.appendChildNode(&translator->graphObject());
            translator->pushTranslationIfDirty(inContext);
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
    if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSNode &theItem = static_cast<Q3DSNode &>(graphObject());
    if (name == QLatin1String("position")) {
        list.append(theItem.setPosition(Q3DSValueParser::parseVector(value)));
    } else if (name == QLatin1String("rotation")) {
        list.append(theItem.setRotation(Q3DSValueParser::parseVector(value)));
    } else if (name == QLatin1String("scale")) {
        list.append(theItem.setScale(Q3DSValueParser::parseVector(value)));
    } else if (name == QLatin1String("pivot")) {
        list.append(theItem.setPivot(Q3DSValueParser::parseVector(value)));
    } else if (name == QLatin1String("opacity")) {
        list.append(theItem.setLocalOpacity(value.getData<float>()));
    } else if (name == QLatin1String("rotationorder")) {
        list.append(theItem.setRotationOrder(Q3DSValueParser::parseEnum
                                 <Q3DSNode::RotationOrder>(value)));
    } else if (name == QLatin1String("orientation")) {
        list.append(theItem.setOrientation(Q3DSValueParser::parseEnum
                               <Q3DSNode::Orientation>(value)));
    } else if (name == QLatin1String("boneid")) {
        list.append(theItem.setSkeletonId(value.getData<int>()));
    } else if (name == QLatin1String("ignoresparent")) {
        list.append(theItem.setIgnoresParent(value.getData<bool>()));
    } else if (name == QLatin1String("eyeball")) {
        list.append(theItem.setEyeballEnabled(value.getData<bool>()));
    }
    if (list.count()) {
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

void Q3DSNodeTranslator::copyProperties(Q3DSGraphObject *target, bool ignoreReferenced)
{
    Q3DSPropertyChangeList list;
    Q3DSGraphObjectTranslator::copyProperties(target, ignoreReferenced);

    Q3DSNode *targetNode = static_cast<Q3DSNode *>(target);
    const Q3DSNode &theItem = *graphObject<Q3DSNode>();
    if (!ignoreReferenced) {
        list.append(targetNode->setPosition(theItem.position()));
        list.append(targetNode->setScale(theItem.scale()));
        list.append(targetNode->setRotation(theItem.rotation()));
        list.append(targetNode->setPivot(theItem.pivot()));
        list.append(targetNode->setLocalOpacity(theItem.localOpacity()));
    }
    list.append(targetNode->setRotationOrder(theItem.rotationOrder()));
    list.append(targetNode->setOrientation(theItem.orientation()));
    list.append(targetNode->setSkeletonId(theItem.skeletonId()));
    list.append(targetNode->setIgnoresParent(theItem.ignoresParent()));
    list.append(targetNode->setEyeballEnabled(theItem.eyeballEnabled()));
    targetNode->notifyPropertyChanges(list);
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
        qt3dsdm::Qt3DSDMInstanceHandle child =
            inContext.assetGraph().GetChild(instanceHandle(), idx);
        Q3DSGraphObjectTranslator *translator = inContext.getOrCreateTranslator(child);
        theItem.appendChildNode(&translator->graphObject());
        translators << translator;
    }
    theItem.resolveReferences(*inContext.presentation());
    for (auto t : qAsConst(translators))
        t->pushTranslationIfDirty(inContext);
}

void Q3DSSceneTranslator::appendChild(Q3DSGraphObject &inChild)
{
    if (inChild.type() != Q3DSNode::Layer && !isMaterial(inChild)) {
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
    if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSScene &theItem = static_cast<Q3DSScene &>(graphObject());
    if (name == QLatin1String("bgcolorenable"))
        list.append(theItem.setUseClearColor(value.getData<bool>()));
    else if (name == QLatin1String("backgroundcolor"))
        list.append(theItem.setClearColor(Q3DSValueParser::parseColor(value)));

    if (list.count()) {
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

void Q3DSSceneTranslator::copyProperties(Q3DSGraphObject *target, bool ignoreReferenced)
{
    Q3DSGraphObjectTranslator::copyProperties(target, ignoreReferenced);
    Q3DSPropertyChangeList list;
    Q3DSScene *targetScene = static_cast<Q3DSScene *>(target);
    const Q3DSScene &theItem = *graphObject<Q3DSScene>();
    list.append(targetScene->setUseClearColor(theItem.useClearColor()));
    list.append(targetScene->setClearColor(theItem.clearColor()));
    targetScene->notifyPropertyChanges(list);
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
    // we'll handle this
    if (name != QLatin1String("eyeball")) {
        if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
            return true;
    }

    Q3DSPropertyChangeList list;
    Q3DSCameraNode &theItem = static_cast<Q3DSCameraNode &>(graphObject());
    if (name == QLatin1String("orthographic")) {
        list.append(theItem.setOrthographic(value.getData<bool>()));
    } else if (name == QLatin1String("clipnear")) {
        list.append(theItem.setClipNear(value.getData<float>()));
    } else if (name == QLatin1String("clipfar")) {
        list.append(theItem.setClipFar(value.getData<float>()));
    } else if (name == QLatin1String("fov")) {
        list.append(theItem.setFov(value.getData<float>()));
    }  else if (name == QLatin1String("scalemode")) {
        list.append(theItem.setScaleMode(
                          Q3DSValueParser::parseEnum<Q3DSCameraNode::ScaleMode>(value)));
    } else if (name == QLatin1String("scaleanchor")) {
        list.append(theItem.setScaleAnchor(
                          Q3DSValueParser::parseEnum<Q3DSCameraNode::ScaleAnchor>(value)));
    } else if (name == QLatin1String("eyeball")) {
        if (m_editCameraEnabled) {
            m_activeState = value.getData<bool>();
            return true;
        } else {
            return Q3DSNodeTranslator::updateProperty(inContext, instance,
                                                      property, value, name);
        }
    }
    if (list.count()) {
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

void Q3DSCameraTranslator::copyProperties(Q3DSGraphObject *target, bool ignoreReferenced)
{
    Q3DSNodeTranslator::copyProperties(target, ignoreReferenced);
    Q3DSPropertyChangeList list;
    Q3DSCameraNode *targetCamera = static_cast<Q3DSCameraNode *>(target);
    const Q3DSCameraNode &theItem = *graphObject<Q3DSCameraNode>();
    list.append(targetCamera->setOrthographic(theItem.orthographic()));
    list.append(targetCamera->setClipNear(theItem.clipNear()));
    list.append(targetCamera->setClipFar(theItem.clipFar()));
    list.append(targetCamera->setFov(theItem.fov()));
    list.append(targetCamera->setScaleMode(theItem.scaleMode()));
    list.append(targetCamera->setScaleAnchor(theItem.scaleAnchor()));
    list.append(targetCamera->setEyeballEnabled(theItem.eyeballEnabled()));
    targetCamera->notifyPropertyChanges(list);
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
    // we'll handle this
    if (name != QLatin1String("eyeball")) {
        if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
            return true;
    }

    Q3DSPropertyChangeList list;
    Q3DSLightNode &theItem = static_cast<Q3DSLightNode &>(graphObject());
    if (name == QLatin1String("scope")) {
        list.append(theItem.setScope(
                          Q3DSValueParser::parseObjectRef(&inContext, instance, value)));
        theItem.resolveReferences(*inContext.presentation());
    } else if (name == QLatin1String("lighttype")) {
        list.append(theItem.setLightType
                      (Q3DSValueParser::parseEnum<Q3DSLightNode::LightType>(value)));
    } else if (name == QLatin1String("lightdiffuse")) {
        list.append(theItem.setDiffuse(Q3DSValueParser::parseColor(value)));
    } else if (name == QLatin1String("lightspecular")) {
        list.append(theItem.setSpecular(Q3DSValueParser::parseColor(value)));
    } else if (name == QLatin1String("lightambient")) {
        list.append(theItem.setAmbient(Q3DSValueParser::parseColor(value)));
    } else if (name == QLatin1String("brightness")) {
        list.append(theItem.setBrightness(value.getData<float>()));
    } else if (name == QLatin1String("linearfade")) {
        list.append(theItem.setLinearFade(value.getData<float>()));
    } else if (name == QLatin1String("expfade")) {
        list.append(theItem.setExpFade(value.getData<float>()));
    } else if (name == QLatin1String("areawidth")) {
        list.append(theItem.setAreaWidth(value.getData<float>()));
    } else if (name == QLatin1String("areaheight")) {
        list.append(theItem.setAreaHeight(value.getData<float>()));
    } else if (name == QLatin1String("castshadow")) {
        list.append(theItem.setCastShadow(value.getData<bool>()));
    } else if (name == QLatin1String("shdwfactor")) {
        list.append(theItem.setShadowFactor(value.getData<float>()));
    } else if (name == QLatin1String("shdwfilter")) {
        list.append(theItem.setShadowFilter(value.getData<float>()));
    } else if (name == QLatin1String("shdwmapres")) {
        list.append(theItem.setShadowMapRes(value.getData<int>()));
    } else if (name == QLatin1String("shdwbias")) {
        list.append(theItem.setShadowBias(value.getData<float>()));
    } else if (name == QLatin1String("shdwmapfar")) {
        list.append(theItem.setShadowMapFar(value.getData<float>()));
    } else if (name == QLatin1String("shdwmapfov")) {
        list.append(theItem.setShadowMapFov(value.getData<float>()));
    } else if (name == QLatin1String("eyeball")) {
        if (m_editLightEnabled) {
            m_activeState = value.getData<bool>();
            return true;
        } else {
            return Q3DSNodeTranslator::updateProperty(inContext, instance,
                                                      property, value, name);
        }
    }
    if (list.count()) {
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

void Q3DSLightTranslator::setActive(bool inActive)
{
    if (m_editLightEnabled)
        m_activeState = inActive;
}

void Q3DSLightTranslator::copyProperties(Q3DSGraphObject *target, bool ignoreReferenced)
{
    Q3DSNodeTranslator::copyProperties(target, ignoreReferenced);
    Q3DSPropertyChangeList list;
    Q3DSLightNode *targetLight = static_cast<Q3DSLightNode *>(target);
    const Q3DSLightNode &theItem = *graphObject<Q3DSLightNode>();
    list.append(targetLight->setScope(theItem.scope()));
    list.append(targetLight->setLightType(theItem.lightType()));
    list.append(targetLight->setDiffuse(theItem.diffuse()));
    list.append(targetLight->setSpecular(theItem.specular()));
    list.append(targetLight->setAmbient(theItem.ambient()));
    list.append(targetLight->setBrightness(theItem.brightness()));
    list.append(targetLight->setLinearFade(theItem.linearFade()));
    list.append(targetLight->setExpFade(theItem.expFade()));
    list.append(targetLight->setAreaWidth(theItem.areaWidth()));
    list.append(targetLight->setAreaHeight(theItem.areaHeight()));
    list.append(targetLight->setCastShadow(theItem.castShadow()));
    list.append(targetLight->setShadowFactor(theItem.shadowFactor()));
    list.append(targetLight->setShadowFilter(theItem.shadowFilter()));
    list.append(targetLight->setShadowMapRes(theItem.shadowMapRes()));
    list.append(targetLight->setShadowBias(theItem.shadowBias()));
    list.append(targetLight->setShadowMapFar(theItem.shadowMapFar()));
    list.append(targetLight->setShadowMapFov(theItem.shadowMapFov()));
    targetLight->notifyPropertyChanges(list);
}

void Q3DSLightTranslator::setEditLightEnabled(bool enabled)
{
    if (m_editLightEnabled != enabled) {
        Q3DSLightNode &theItem = static_cast<Q3DSLightNode &>(graphObject());
        m_editLightEnabled = enabled;
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
                childTranslator->pushTranslationIfDirty(inContext);
            }
        }
    }
    theItem.resolveReferences(*inContext.presentation());
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
    if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSModelNode &theItem = static_cast<Q3DSModelNode &>(graphObject());
    if (name == QLatin1String("sourcepath")) {
        list.append(theItem.setMesh(value.toQVariant().toString()));
        theItem.resolveReferences(*inContext.presentation());
    } else if (name == QLatin1String("poseroot")) {
        list.append(theItem.setSkeletonRoot(value.getData<int>()));
    } else if (name == QLatin1String("tessellation")) {
        list.append(theItem.setTessellation(
                          Q3DSValueParser::parseEnum<Q3DSModelNode::Tessellation>(value)));
    } else if (name == QLatin1String("edgetess")) {
        list.append(theItem.setEdgeTess(value.getData<float>()));
    } else if (name == QLatin1String("innertess")) {
        list.append(theItem.setInnerTess(value.getData<float>()));
    }
    if (list.count()) {
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

void Q3DSModelTranslator::copyProperties(Q3DSGraphObject *target, bool ignoreReferenced)
{
    Q3DSNodeTranslator::copyProperties(target, ignoreReferenced);
    Q3DSPropertyChangeList list;
    Q3DSModelNode *targetModel = static_cast<Q3DSModelNode *>(target);
    const Q3DSModelNode &theItem = *graphObject<Q3DSModelNode>();
    list.append(targetModel->setMesh(theItem.sourcePath()));
    list.append(targetModel->setSkeletonRoot(theItem.skeletonRoot()));
    list.append(targetModel->setTessellation(theItem.tessellation()));
    list.append(targetModel->setEdgeTess(theItem.edgeTess()));
    list.append(targetModel->setInnerTess(theItem.innerTess()));
    targetModel->notifyPropertyChanges(list);
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
    if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSImage &theItem = static_cast<Q3DSImage &>(graphObject());
    if (name == QLatin1String("scaleu")) {
        list.append(theItem.setScaleU(value.getData<float>()));
    } else if (name == QLatin1String("scalev")) {
        list.append(theItem.setScaleV(value.getData<float>()));
    } else if (name == QLatin1String("subpresentation")) {
        list.append(theItem.setSubPresentation(value.toQVariant().toString()));
        theItem.resolveReferences(*inContext.presentation());
    } else if (name == QLatin1String("mappingmode")) {
        list.append(theItem.setMappingMode(
                          Q3DSValueParser::parseEnum<Q3DSImage::MappingMode>(value)));
    } else if (name == QLatin1String("tilingmodehorz")) {
        list.append(theItem.setHorizontalTiling(
                          Q3DSValueParser::parseEnum<Q3DSImage::TilingMode>(value)));
    } else if (name == QLatin1String("tilingmodevert")) {
        list.append(theItem.setVerticalTiling(
                          Q3DSValueParser::parseEnum<Q3DSImage::TilingMode>(value)));
    } else if (name == QLatin1String("rotationuv")) {
        list.append(theItem.setRotationUV(value.getData<float>()));
    } else if (name == QLatin1String("positionu")) {
        list.append(theItem.setPositionU(value.getData<float>()));
    } else if (name == QLatin1String("positionv")) {
        list.append(theItem.setPositionV(value.getData<float>()));
    } else if (name == QLatin1String("pivotu")) {
        list.append(theItem.setPivotU(value.getData<float>()));
    } else if (name == QLatin1String("pivotv")) {
        list.append(theItem.setPivotV(value.getData<float>()));
    } else if (name == QLatin1String("sourcepath")) {
        list.append(theItem.setSourcePath(value.toQVariant().toString()));
        theItem.resolveReferences(*inContext.presentation());
    }
    if (list.count()) {
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

void Q3DSImageTranslator::copyProperties(Q3DSGraphObject *target, bool ignoreReferenced)
{
    Q3DSGraphObjectTranslator::copyProperties(target, ignoreReferenced);
    Q3DSPropertyChangeList list;
    Q3DSImage *targetImage = static_cast<Q3DSImage *>(target);
    const Q3DSImage &theItem = *graphObject<Q3DSImage>();
    list.append(targetImage->setScaleU(theItem.scaleU()));
    list.append(targetImage->setScaleV(theItem.scaleV()));
    list.append(targetImage->setSubPresentation(theItem.subPresentation()));
    list.append(targetImage->setMappingMode(theItem.mappingMode()));
    list.append(targetImage->setHorizontalTiling(theItem.horizontalTiling()));
    list.append(targetImage->setVerticalTiling(theItem.verticalTiling()));
    list.append(targetImage->setRotationUV(theItem.rotationUV()));
    list.append(targetImage->setPositionU(theItem.positionU()));
    list.append(targetImage->setPositionV(theItem.positionV()));
    list.append(targetImage->setPivotU(theItem.pivotU()));
    list.append(targetImage->setPivotV(theItem.pivotV()));
    targetImage->notifyPropertyChanges(list);
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
        appendChild(childTranslator->graphObject());
        childTranslator->pushTranslationIfDirty(inContext);
    }
    theItem.resolveReferences(*inContext.presentation());

    for (auto &change : qAsConst(list)) {
        if (s_recompileProperties.contains(change.nameStr()))
            m_specifiedImageMaps.insert(change.nameStr());
    }
}

void Q3DSDefaultMaterialTranslator::setActive(bool)
{
}

void Q3DSDefaultMaterialTranslator::clearChildren()
{
}

void Q3DSDefaultMaterialTranslator::appendChild(Q3DSGraphObject &child)
{
    Q3DSDefaultMaterial &theItem = static_cast<Q3DSDefaultMaterial &>(graphObject());
    theItem.appendChildNode(&child);
}

bool Q3DSDefaultMaterialTranslator::updateProperty(Q3DSTranslation &inContext,
                                                   qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                   qt3dsdm::Qt3DSDMPropertyHandle property,
                                                   qt3dsdm::SValue &value,
                                                   const QString &name)
{
    if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSDefaultMaterial &theItem = static_cast<Q3DSDefaultMaterial &>(graphObject());
    if (name == QLatin1String("shaderlighting")) {
        list.append(theItem.setShaderLighting(
                          Q3DSValueParser::parseEnum<
                                                Q3DSDefaultMaterial::ShaderLighting>(value)));
    } else if (name == QLatin1String("blendmode")) {
        list.append(theItem.setBlendMode(
                          Q3DSValueParser::parseEnum<Q3DSDefaultMaterial::BlendMode>(value)));
    }/* else if (name == QLatin1String("vertexcolors")) {
        ret = true;
    } */else if (name == QLatin1String("diffuse")) {
        list.append(theItem.setDiffuse(Q3DSValueParser::parseColor(value)));
    } else if (name == QLatin1String("diffusemap")) {
        list.append(theItem.setDiffuseMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("diffusemap2")) {
        list.append(theItem.setDiffuseMap2(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("diffusemap3")) {
        list.append(theItem.setDiffuseMap3(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("specularreflection")) {
        list.append(theItem.setSpecularReflection(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("speculartint")) {
        list.append(theItem.setSpecularTint(Q3DSValueParser::parseColor(value)));
    } else if (name == QLatin1String("specularamount")) {
        list.append(theItem.setSpecularAmount(value.getData<float>()));
    } else if (name == QLatin1String("specularmap")) {
        list.append(theItem.setSpecularMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("specularmodel")) {
        list.append(theItem.setSpecularModel(
                          Q3DSValueParser::parseEnum<Q3DSDefaultMaterial::SpecularModel>(value)));
    } else if (name == QLatin1String("specularroughness")) {
        list.append(theItem.setSpecularRoughness(value.getData<float>()));
    } else if (name == QLatin1String("roughnessmap")) {
        list.append(theItem.setRoughnessMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("fresnelPower")) {
        list.append(theItem.setFresnelPower(value.getData<float>()));
    } else if (name == QLatin1String("ior")) {
        list.append(theItem.setIor(value.getData<float>()));
    } else if (name == QLatin1String("bumpmap")) {
        list.append(theItem.setBumpMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("normalmap")) {
        list.append(theItem.setNormalMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("bumpamount")) {
        list.append(theItem.setBumpAmount(value.getData<float>()));
    } else if (name == QLatin1String("displacementmap")) {
        list.append(theItem.setDisplacementMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("displaceamount")) {
        list.append(theItem.setDisplaceAmount(value.getData<float>()));
    } else if (name == QLatin1String("opacity")) {
        list.append(theItem.setOpacity(value.getData<float>()));
    } else if (name == QLatin1String("opacitymap")) {
        list.append(theItem.setOpacityMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("emissivecolor")) {
        list.append(theItem.setEmissiveColor(
                          Q3DSValueParser::parseColor(value)));
    } else if (name == QLatin1String("emissivepower")) {
        list.append(theItem.setEmissivePower(value.getData<float>()));
    } else if (name == QLatin1String("emissivemap")) {
        list.append(theItem.setEmissiveMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("emissivemap2")) {
        list.append(theItem.setEmissiveMap2(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("translucencymap")) {
        list.append(theItem.setTranslucencyMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("translucentfalloff")) {
        list.append(theItem.setTranslucentFalloff(value.getData<float>()));
    } else if (name == QLatin1String("diffuselightwrap")) {
        list.append(theItem.setDiffuseLightWrap(value.getData<float>()));
    } else if (name == QLatin1String("iblprobe")) {
        list.append(theItem.setLightProbe(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("lightmapindirect")) {
        list.append(theItem.setLightmapIndirectMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("lightmapradiosity")) {
        list.append(theItem.setLightmapRadiosityMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("lightmapshadow")) {
        list.append(theItem.setLightmapShadowMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    }
    if (list.count()) {
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

void Q3DSDefaultMaterialTranslator::copyProperties(Q3DSGraphObject *target, bool ignoreReferenced)
{
    Q3DSGraphObjectTranslator::copyProperties(target, ignoreReferenced);
    Q3DSDefaultMaterial *targetMaterial = static_cast<Q3DSDefaultMaterial *>(target);
    const Q3DSDefaultMaterial &theItem = *graphObject<Q3DSDefaultMaterial>();
    Q3DSPropertyChangeList list;
    list.append(targetMaterial->setShaderLighting(theItem.shaderLighting()));
    list.append(targetMaterial->setBlendMode(theItem.blendMode()));
    list.append(targetMaterial->setDiffuse(theItem.diffuse()));
    list.append(targetMaterial->setDiffuseMap(theItem.diffuseMap()));
    list.append(targetMaterial->setDiffuseMap2(theItem.diffuseMap2()));
    list.append(targetMaterial->setDiffuseMap3(theItem.diffuseMap3()));
    list.append(targetMaterial->setSpecularReflection(theItem.specularReflection()));
    list.append(targetMaterial->setSpecularTint(theItem.specularTint()));
    list.append(targetMaterial->setSpecularAmount(theItem.specularAmount()));
    list.append(targetMaterial->setSpecularMap(theItem.specularMap()));
    list.append(targetMaterial->setSpecularModel(theItem.specularModel()));
    list.append(targetMaterial->setSpecularRoughness(theItem.specularRoughness()));
    list.append(targetMaterial->setRoughnessMap(theItem.roughnessMap()));
    list.append(targetMaterial->setFresnelPower(theItem.fresnelPower()));
    list.append(targetMaterial->setIor(theItem.ior()));
    list.append(targetMaterial->setBumpMap(theItem.bumpMap()));
    list.append(targetMaterial->setNormalMap(theItem.normalMap()));
    list.append(targetMaterial->setBumpAmount(theItem.bumpAmount()));
    list.append(targetMaterial->setDisplacementMap(theItem.displacementMap()));
    list.append(targetMaterial->setDisplaceAmount(theItem.displaceAmount()));
    list.append(targetMaterial->setOpacity(theItem.opacity()));
    list.append(targetMaterial->setOpacityMap(theItem.opacityMap()));
    list.append(targetMaterial->setEmissiveColor(theItem.emissiveColor()));
    list.append(targetMaterial->setEmissivePower(theItem.emissivePower()));
    list.append(targetMaterial->setEmissiveMap(theItem.emissiveMap()));
    list.append(targetMaterial->setEmissiveMap2(theItem.emissiveMap2()));
    list.append(targetMaterial->setTranslucencyMap(theItem.translucencyMap()));
    list.append(targetMaterial->setTranslucentFalloff(theItem.translucentFalloff()));
    list.append(targetMaterial->setDiffuseLightWrap(theItem.diffuseLightWrap()));
    list.append(targetMaterial->setLightmapIndirectMap(theItem.lightmapIndirectMap()));
    list.append(targetMaterial->setLightmapRadiosityMap(theItem.lightmapRadiosityMap()));
    list.append(targetMaterial->setLightmapShadowMap(theItem.lightmapShadowMap()));
    list.append(targetMaterial->setLightProbe(theItem.lightProbe()));
    targetMaterial->notifyPropertyChanges(list);
}

bool Q3DSDefaultMaterialTranslator::shaderRequiresRecompilation(
        Q3DSTranslation &inContext, const qt3dsdm::SValue &value, const QString &name,
        qt3dsdm::AdditionalMetaDataType::Value type)
{
    if (!s_recompileProperties.contains(name))
        return false;
    if (type == qt3dsdm::AdditionalMetaDataType::Image) {
        Q3DSImage *image = Q3DSValueParser::parseImage(&inContext, value, &graphObject());
        if (m_specifiedImageMaps.contains(name) && image == nullptr) {
            m_specifiedImageMaps.remove(name);
            return true;
        } else if (!m_specifiedImageMaps.contains(name) && image != nullptr) {
            m_specifiedImageMaps.insert(name);
            return true;
        }
    } else {
        Q3DSDefaultMaterial &theItem = *graphObject<Q3DSDefaultMaterial>();
        if (name == QLatin1String("shaderlighting") && theItem.shaderLighting()
                != Q3DSValueParser::parseEnum<Q3DSDefaultMaterial::ShaderLighting>(value)) {
            return true;
        } else if (name == QLatin1String("specularmodel") && theItem.specularModel()
                != Q3DSValueParser::parseEnum<Q3DSDefaultMaterial::SpecularModel>(value)) {
            return true;
        }
    }
    return false;
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
    if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSReferencedMaterial &theItem = static_cast<Q3DSReferencedMaterial &>(graphObject());
    if (name == QLatin1String("referencedmaterial")) {
        list.append(theItem.setReferencedMaterial(
                          Q3DSValueParser::parseObjectRef(&inContext, instance, value)));
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

void Q3DSReferencedMaterialTranslator::copyProperties(Q3DSGraphObject *target,
                                                      bool ignoreReferenced)
{
    Q3DSGraphObjectTranslator::copyProperties(target, ignoreReferenced);
    Q3DSReferencedMaterial *targetMaterial
            = static_cast<Q3DSReferencedMaterial *>(target);
    const Q3DSReferencedMaterial &theItem = *graphObject<Q3DSReferencedMaterial>();
    Q3DSPropertyChangeList list;
    list.append(targetMaterial->setReferencedMaterial(theItem.referencedMaterial()));
    list.append(targetMaterial->setLightProbe(theItem.lightProbe()));
    list.append(targetMaterial->setLightmapIndirectMap(theItem.lightmapIndirectMap()));
    list.append(targetMaterial->setLightmapRadiosityMap(theItem.lightmapRadiosityMap()));
    list.append(targetMaterial->setLightmapShadowMap(theItem.lightmapShadowMap()));
    targetMaterial->notifyPropertyChanges(list);
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
#undef HANDLE_SUFFIX
#define HANDLE_SUFFIX(name)                       \
        if (m_editLayerEnabled)                   \
            m_active##name = temp;                \
        else                                      \
            list.append(theItem.set##name(temp)); \
     }
    ITERATE_Q3DS_LAYER_PROPERTIES(m_editLayerEnabled)
#undef HANDLE_SUFFIX
#define HANDLE_SUFFIX(name) DEFAULT_HANDLE_SUFFIX(name)

    int childCount = inContext.assetGraph().GetChildCount(instanceHandle());
    for (int i = 0; i < childCount; ++i) {
        qt3dsdm::Qt3DSDMInstanceHandle childInstance
                = inContext.assetGraph().GetChild(instanceHandle(), i);
        Q3DSGraphObjectTranslator *childTranslator
                = inContext.getOrCreateTranslator(childInstance);
        if (childTranslator) {
            if (childTranslator->graphObject().parent() == nullptr)
                appendChild(childTranslator->graphObject());
            childTranslator->pushTranslationIfDirty(inContext);
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

#define UPDATE_SIMPLE_PROPERTY(type, name)                     \
    if (m_editLayerEnabled)                                    \
        m_active##name = value.getData<type>();                \
    else                                                       \
        list.append(theItem.set##name(value.getData<type>()));

#define UPDATE_ENUM_PROPERTY(enumType, name)                      \
    auto enumValue = Q3DSValueParser::parseEnum<enumType>(value); \
    if (m_editLayerEnabled) {                                     \
        m_active##name = enumValue;                               \
        return true;                                              \
    } else {                                                      \
        list.append(theItem.set##name(enumValue));                \
    }

bool Q3DSLayerTranslator::updateProperty(Q3DSTranslation &inContext,
                                         qt3dsdm::Qt3DSDMInstanceHandle instance,
                                         qt3dsdm::Qt3DSDMPropertyHandle property,
                                         qt3dsdm::SValue &value,
                                         const QString &name)
{
    if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSLayerNode &theItem = static_cast<Q3DSLayerNode &>(graphObject());
    if (name == QLatin1String("disabledepthtest")) {
        UPDATE_SIMPLE_PROPERTY(bool, DepthTestDisabled)
    } else if (name == QLatin1String("disabledepthprepass")) {
        UPDATE_SIMPLE_PROPERTY(bool, DepthPrePassDisabled)
    } else if (name == QLatin1String("progressiveaa")) {
        UPDATE_ENUM_PROPERTY(Q3DSLayerNode::ProgressiveAA, ProgressiveAA)
    } else if (name == QLatin1String("multisampleaa")) {
        UPDATE_ENUM_PROPERTY(Q3DSLayerNode::MultisampleAA, MultisampleAA)
    } else if (name == QLatin1String("temporalaa")) {
        UPDATE_SIMPLE_PROPERTY(bool, TemporalAAEnabled)
    } else if (name == QLatin1String("background")) {
        UPDATE_ENUM_PROPERTY(Q3DSLayerNode::LayerBackground, LayerBackground)
    } else if (name == QLatin1String("backgroundcolor")) {
        QColor color = Q3DSValueParser::parseColor(value);
        if (m_editLayerEnabled)
            m_activeBackgroundColor = color;
        else
            list.append(theItem.setBackgroundColor(color));
    } else if (name == QLatin1String("blendtype")) {
        UPDATE_ENUM_PROPERTY(Q3DSLayerNode::BlendType, BlendType)
    } else if (name == QLatin1String("horzfields")) {
        UPDATE_ENUM_PROPERTY(Q3DSLayerNode::HorizontalFields, HorizontalFields)
    } else if (name == QLatin1String("left")) {
        UPDATE_SIMPLE_PROPERTY(float, Left)
    } else if (name == QLatin1String("leftunits")) {
        UPDATE_ENUM_PROPERTY(Q3DSLayerNode::Units, LeftUnits)
    } else if (name == QLatin1String("width")) {
        UPDATE_SIMPLE_PROPERTY(float, Width)
    } else if (name == QLatin1String("widthunits")) {
        UPDATE_ENUM_PROPERTY(Q3DSLayerNode::Units, WidthUnits)
    } else if (name == QLatin1String("right")) {
        UPDATE_SIMPLE_PROPERTY(float, Right)
    } else if (name == QLatin1String("rightunits")) {
        UPDATE_ENUM_PROPERTY(Q3DSLayerNode::Units, RightUnits)
    } else if (name == QLatin1String("vertfields")) {
        UPDATE_ENUM_PROPERTY(Q3DSLayerNode::VerticalFields, VerticalFields)
    } else if (name == QLatin1String("top")) {
        UPDATE_SIMPLE_PROPERTY(float, Top)
    } else if (name == QLatin1String("topunits")) {
        UPDATE_ENUM_PROPERTY(Q3DSLayerNode::Units, TopUnits)
    } else if (name == QLatin1String("height")) {
        UPDATE_SIMPLE_PROPERTY(float, Height)
    } else if (name == QLatin1String("heightunits")) {
        UPDATE_ENUM_PROPERTY(Q3DSLayerNode::Units, HeightUnits)
    } else if (name == QLatin1String("bottom")) {
        UPDATE_SIMPLE_PROPERTY(float, Bottom)
    } else if (name == QLatin1String("bottomunits")) {
        UPDATE_ENUM_PROPERTY(Q3DSLayerNode::Units, BottomUnits)
    } else if (name == QLatin1String("sourcepath")) {
        if (m_editLayerEnabled) {
            m_activeSourcePath = value.toQVariant().toString();
        } else {
            list.append(theItem.setSourcePath(value.toQVariant().toString()));
            theItem.resolveReferences(*inContext.presentation());
        }
    } else if (name == QLatin1String("aostrength")) {
        UPDATE_SIMPLE_PROPERTY(float, AoStrength)
    } else if (name == QLatin1String("aodistance")) {
        UPDATE_SIMPLE_PROPERTY(float, AoDistance)
    } else if (name == QLatin1String("aosoftness")) {
        UPDATE_SIMPLE_PROPERTY(float, AoSoftness)
    } else if (name == QLatin1String("aobias")) {
        UPDATE_SIMPLE_PROPERTY(float, AoBias)
    } else if (name == QLatin1String("aosamplerate")) {
        UPDATE_SIMPLE_PROPERTY(int, AoSampleRate)
    } else if (name == QLatin1String("aodither")) {
        UPDATE_SIMPLE_PROPERTY(bool, AoDither)
    } else if (name == QLatin1String("shadowstrength")) {
        UPDATE_SIMPLE_PROPERTY(float, ShadowStrength)
    } else if (name == QLatin1String("shadowdist")) {
        UPDATE_SIMPLE_PROPERTY(float, ShadowDist)
    } else if (name == QLatin1String("shadowsoftness")) {
        UPDATE_SIMPLE_PROPERTY(float, ShadowSoftness)
    } else if (name == QLatin1String("shadowbias")) {
        UPDATE_SIMPLE_PROPERTY(float, ShadowBias)
    } else if (name == QLatin1String("lightprobe")) {
        Q3DSImage *image = Q3DSValueParser::parseImage(&inContext, value, &graphObject());
        if (m_editLayerEnabled)
            m_activeLightProbe = image;
        else
            list.append(theItem.setLightProbe(image));
    } else if (name == QLatin1String("probebright")) {
        UPDATE_SIMPLE_PROPERTY(float, ProbeBright)
    } else if (name == QLatin1String("fastibl")) {
        UPDATE_SIMPLE_PROPERTY(bool, FastIBLEnabled)
    } else if (name == QLatin1String("probehorizon")) {
        UPDATE_SIMPLE_PROPERTY(float, ProbeHorizon)
    } else if (name == QLatin1String("probefov")) {
        UPDATE_SIMPLE_PROPERTY(float, ProbeFov)
    } else if (name == QLatin1String("lightprobe2")) {
        Q3DSImage *image = Q3DSValueParser::parseImage(&inContext, value, &graphObject());
        if (m_editLayerEnabled)
            m_activeLightProbe2 = image;
        else
            list.append(theItem.setLightProbe2(image));
    } else if (name == QLatin1String("probe2fade")) {
        UPDATE_SIMPLE_PROPERTY(float, Probe2Fade)
    } else if (name == QLatin1String("probe2window")) {
        UPDATE_SIMPLE_PROPERTY(float, Probe2Window)
    } else if (name == QLatin1String("probe2pos")) {
        UPDATE_SIMPLE_PROPERTY(float, Probe2Pos)
    }
    if (list.count()) {
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

void Q3DSLayerTranslator::copyProperties(Q3DSGraphObject *target, bool ignoreReferenced)
{
    Q3DSGraphObjectTranslator::copyProperties(target, ignoreReferenced);
    Q3DSLayerNode *targetLayer = static_cast<Q3DSLayerNode *>(target);
    const Q3DSLayerNode &theItem = *graphObject<Q3DSLayerNode>();
    Q3DSPropertyChangeList list;
    list.append(targetLayer->setDepthTestDisabled(theItem.depthTestDisabled()));
    list.append(targetLayer->setDepthPrePassDisabled(theItem.depthPrePassDisabled()));
    list.append(targetLayer->setTemporalAAEnabled(theItem.temporalAAEnabled()));
    list.append(targetLayer->setFastIBLEnabled(theItem.fastIBLEnabled()));
    list.append(targetLayer->setProgressiveAA(theItem.progressiveAA()));
    list.append(targetLayer->setMultisampleAA(theItem.multisampleAA()));
    list.append(targetLayer->setLayerBackground(theItem.layerBackground()));
    list.append(targetLayer->setBackgroundColor(theItem.backgroundColor()));
    list.append(targetLayer->setBlendType(theItem.blendType()));
    list.append(targetLayer->setHorizontalFields(theItem.horizontalFields()));
    list.append(targetLayer->setLeft(theItem.left()));
    list.append(targetLayer->setLeftUnits(theItem.leftUnits()));
    list.append(targetLayer->setWidth(theItem.width()));
    list.append(targetLayer->setWidthUnits(theItem.widthUnits()));
    list.append(targetLayer->setRight(theItem.right()));
    list.append(targetLayer->setRightUnits(theItem.rightUnits()));
    list.append(targetLayer->setVerticalFields(theItem.verticalFields()));
    list.append(targetLayer->setTop(theItem.top()));
    list.append(targetLayer->setTopUnits(theItem.topUnits()));
    list.append(targetLayer->setHeight(theItem.height()));
    list.append(targetLayer->setHeightUnits(theItem.heightUnits()));
    list.append(targetLayer->setBottom(theItem.bottom()));
    list.append(targetLayer->setBottomUnits(theItem.bottomUnits()));
    list.append(targetLayer->setSourcePath(theItem.sourcePath()));
    list.append(targetLayer->setAoStrength(theItem.aoStrength()));
    list.append(targetLayer->setAoDistance(theItem.aoDistance()));
    list.append(targetLayer->setAoSoftness(theItem.aoSoftness()));
    list.append(targetLayer->setAoBias(theItem.aoBias()));
    list.append(targetLayer->setAoSampleRate(theItem.aoSampleRate()));
    list.append(targetLayer->setAoDither(theItem.aoDither()));
    list.append(targetLayer->setShadowStrength(theItem.shadowStrength()));
    list.append(targetLayer->setShadowDist(theItem.shadowDist()));
    list.append(targetLayer->setShadowSoftness(theItem.shadowSoftness()));
    list.append(targetLayer->setShadowBias(theItem.shadowBias()));
    list.append(targetLayer->setLightProbe(theItem.lightProbe()));
    list.append(targetLayer->setProbeBright(theItem.probeBright()));
    list.append(targetLayer->setProbeHorizon(theItem.probeHorizon()));
    list.append(targetLayer->setProbeFov(theItem.probeFov()));
    list.append(targetLayer->setLightProbe2(theItem.lightProbe2()));
    list.append(targetLayer->setProbe2Fade(theItem.probe2Fade()));
    list.append(targetLayer->setProbe2Window(theItem.probe2Window()));
    list.append(targetLayer->setProbe2Pos(theItem.probe2Window()));
    targetLayer->notifyPropertyChanges(list);
}

#define SWAP_PROPERTY_FOR_EDIT(name, getFunc, editValue) \
    if (m_editLayerEnabled) {                            \
        m_active##name = theItem.getFunc();              \
        list.append(theItem.set##name(editValue));       \
    } else {                                             \
        list.append(theItem.set##name(m_active##name));  \
    }

void Q3DSLayerTranslator::setEditLayerEnabled(Q3DSTranslation &inContext, bool enabled)
{
    if (m_editLayerEnabled != enabled) {
        Q3DSLayerNode &theItem = static_cast<Q3DSLayerNode &>(graphObject());
        m_editLayerEnabled = enabled;
        Q3DSPropertyChangeList list;

        SWAP_PROPERTY_FOR_EDIT(DepthTestDisabled, depthTestDisabled, false)
        SWAP_PROPERTY_FOR_EDIT(DepthPrePassDisabled, depthPrePassDisabled, false)
        SWAP_PROPERTY_FOR_EDIT(ProgressiveAA, progressiveAA, Q3DSLayerNode::NoPAA)
        SWAP_PROPERTY_FOR_EDIT(MultisampleAA, multisampleAA, Q3DSLayerNode::NoMSAA)
        SWAP_PROPERTY_FOR_EDIT(TemporalAAEnabled, temporalAAEnabled, false)
        SWAP_PROPERTY_FOR_EDIT(LayerBackground, layerBackground, Q3DSLayerNode::Transparent)
        SWAP_PROPERTY_FOR_EDIT(BackgroundColor, backgroundColor, QColor(Qt::black))
        SWAP_PROPERTY_FOR_EDIT(BlendType, blendType, Q3DSLayerNode::Normal)
        SWAP_PROPERTY_FOR_EDIT(HorizontalFields, horizontalFields, Q3DSLayerNode::LeftWidth)
        SWAP_PROPERTY_FOR_EDIT(Left, left, 0.f)
        SWAP_PROPERTY_FOR_EDIT(LeftUnits, leftUnits, Q3DSLayerNode::Percent)
        SWAP_PROPERTY_FOR_EDIT(Width, width, 100.f)
        SWAP_PROPERTY_FOR_EDIT(WidthUnits, widthUnits, Q3DSLayerNode::Percent)
        SWAP_PROPERTY_FOR_EDIT(Right, right, 0.f)
        SWAP_PROPERTY_FOR_EDIT(RightUnits, rightUnits, Q3DSLayerNode::Percent)
        SWAP_PROPERTY_FOR_EDIT(VerticalFields, verticalFields, Q3DSLayerNode::TopHeight)
        SWAP_PROPERTY_FOR_EDIT(Top, top, 0.f)
        SWAP_PROPERTY_FOR_EDIT(TopUnits, topUnits, Q3DSLayerNode::Percent)
        SWAP_PROPERTY_FOR_EDIT(Height, height, 100.f)
        SWAP_PROPERTY_FOR_EDIT(HeightUnits, heightUnits, Q3DSLayerNode::Percent)
        SWAP_PROPERTY_FOR_EDIT(Bottom, bottom, 0.f)
        SWAP_PROPERTY_FOR_EDIT(BottomUnits, bottomUnits, Q3DSLayerNode::Percent)
        SWAP_PROPERTY_FOR_EDIT(SourcePath, sourcePath, {})
        SWAP_PROPERTY_FOR_EDIT(AoStrength, aoStrength, 0.f)
        SWAP_PROPERTY_FOR_EDIT(AoDistance, aoDistance, 5.f)
        SWAP_PROPERTY_FOR_EDIT(AoSoftness, aoSoftness, 50.f)
        SWAP_PROPERTY_FOR_EDIT(AoBias, aoBias, 0.f)
        SWAP_PROPERTY_FOR_EDIT(AoSampleRate, aoSampleRate, 2)
        SWAP_PROPERTY_FOR_EDIT(AoDither, aoDither, true)
        SWAP_PROPERTY_FOR_EDIT(ShadowStrength, shadowStrength, 0.f)
        SWAP_PROPERTY_FOR_EDIT(ShadowDist, shadowDist, 10.f)
        SWAP_PROPERTY_FOR_EDIT(ShadowSoftness, shadowSoftness, 100.f)
        SWAP_PROPERTY_FOR_EDIT(ShadowBias, shadowBias, 0.f)
        SWAP_PROPERTY_FOR_EDIT(LightProbe, lightProbe, nullptr)
        SWAP_PROPERTY_FOR_EDIT(ProbeBright, probeBright, 0.f)
        SWAP_PROPERTY_FOR_EDIT(FastIBLEnabled, fastIBLEnabled, true)
        SWAP_PROPERTY_FOR_EDIT(ProbeHorizon, probeHorizon, -1.f)
        SWAP_PROPERTY_FOR_EDIT(ProbeFov, probeFov, 180.f)
        SWAP_PROPERTY_FOR_EDIT(LightProbe2, lightProbe2, nullptr)
        SWAP_PROPERTY_FOR_EDIT(Probe2Fade, probe2Fade, 1.f)
        SWAP_PROPERTY_FOR_EDIT(Probe2Window, probe2Window, 1.f)
        SWAP_PROPERTY_FOR_EDIT(Probe2Pos, probe2Pos, 0.5f)

        theItem.resolveReferences(*inContext.presentation());
        theItem.notifyPropertyChanges(list);
    }
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
    if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSSlide &theItem = static_cast<Q3DSSlide &>(graphObject());
    if (name == QLatin1String("playmode")) {
        list.append(theItem.setPlayMode(
                          Q3DSValueParser::parseEnum<Q3DSSlide::PlayMode>(value)));
    } else if (name == QLatin1String("playthroughto")) {
        list.append(theItem.setPlayThrough(
                          Q3DSValueParser::parseEnum<Q3DSSlide::PlayThrough>(value)));
    } else if (name == QLatin1String("initialplaystate")) {
        list.append(theItem.setInitialPlayState(
                          Q3DSValueParser::parseEnum<Q3DSSlide::InitialPlayState>(value)));
    }
    if (list.count()) {
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

void Q3DSSlideTranslator::copyProperties(Q3DSGraphObject *target, bool ignoreReferenced)
{
    Q3DSGraphObjectTranslator::copyProperties(target, ignoreReferenced);
    Q3DSSlide *targetSlide = static_cast<Q3DSSlide *>(target);
    const Q3DSSlide &theItem = *graphObject<Q3DSSlide>();
    Q3DSPropertyChangeList list;
    list.append(targetSlide->setPlayMode(theItem.playMode()));
    list.append(targetSlide->setPlayThrough(theItem.playThrough()));
    list.append(targetSlide->setInitialPlayState(theItem.initialPlayState()));
    targetSlide->notifyPropertyChanges(list);
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
    if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSTextNode &theItem = static_cast<Q3DSTextNode &>(graphObject());
    if (name == QLatin1String("textstring")) {
        list.append(theItem.setText(value.toQVariant().toString()));
    } else if (name == QLatin1String("textcolor")) {
        list.append(theItem.setColor(Q3DSValueParser::parseColor(value)));
    } else if (name == QLatin1String("font")) {
        list.append(theItem.setFont(value.toQVariant().toString()));
    } else if (name == QLatin1String("size")) {
        list.append(theItem.setSize(value.getData<float>()));
    } else if (name == QLatin1String("horzalign")) {
        list.append(theItem.setHorizontalAlignment(
                          Q3DSValueParser::parseEnum<Q3DSTextNode::HorizontalAlignment>(value)));
    } else if (name == QLatin1String("vertalign")) {
        list.append(theItem.setVerticalAlignment(
                          Q3DSValueParser::parseEnum<Q3DSTextNode::VerticalAlignment>(value)));
    } else if (name == QLatin1String("leading")) {
        list.append(theItem.setLeading(value.getData<float>()));
    } else if (name == QLatin1String("tracking")) {
        list.append(theItem.setTracking(value.getData<float>()));
    }
    // TODO handle dropshadow properties (commit 4e97e4edc636b306eb1009cc5c6c189d78ae7774)
    // TODO handle wordwarp/boundingbox (commit d53d85e208b5c400142f0389ef16f073229af908)
    if (list.count()) {
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

void Q3DSTextTranslator::copyProperties(Q3DSGraphObject *target, bool ignoreReferenced)
{
    Q3DSNodeTranslator::copyProperties(target, ignoreReferenced);
    Q3DSTextNode *targetText = static_cast<Q3DSTextNode *>(target);
    const Q3DSTextNode &theItem = *graphObject<Q3DSTextNode>();
    Q3DSPropertyChangeList list;
    list.append(targetText->setText(theItem.text()));
    list.append(targetText->setColor(theItem.color()));
    list.append(targetText->setFont(theItem.font()));
    list.append(targetText->setSize(theItem.size()));
    list.append(targetText->setHorizontalAlignment(theItem.horizontalAlignment()));
    list.append(targetText->setVerticalAlignment(theItem.verticalAlignment()));
    list.append(targetText->setLeading(theItem.leading()));
    list.append(targetText->setTracking(theItem.tracking()));
    targetText->notifyPropertyChanges(list);
}

Q3DSDynamicObjectTranslator::Q3DSDynamicObjectTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                         Q3DSGraphObject &graphObject)
    : Q3DSGraphObjectTranslator(instance, graphObject)
{

}

void Q3DSDynamicObjectTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSGraphObjectTranslator::pushTranslation(inContext);
    Q3DSGraphObject &theItem = graphObject();
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
    const QMap<QString, Q3DSMaterial::PropertyElement> *properties;

    if (theItem.type() == Q3DSGraphObject::Effect) {
        Q3DSEffectInstance *effect = graphObject<Q3DSEffectInstance>();
        properties = &effect->effect()->properties();
    } else if (theItem.type() == Q3DSGraphObject::CustomMaterial) {
        Q3DSCustomMaterialInstance *material = graphObject<Q3DSCustomMaterialInstance>();
        properties = &material->material()->properties();
    } else {
        Q_ASSERT_X(false, __FUNCTION__, "Incorrect graphObject type");
    }

    std::shared_ptr<qt3dsdm::IDataCore> datacore
            = inContext.fullSystem().GetCoreSystem()->GetDataCore();

    const auto propertyKeys = properties->keys();
    for (auto property : propertyKeys) {
        const Q3DSMaterial::PropertyElement &element = (*properties)[property];
        qt3dsdm::Qt3DSDMPropertyHandle theProperty =
            inContext.reader().FindProperty(instanceHandle(), property);
        if (!datacore->IsProperty(theProperty))
            continue;

        Option<qt3dsdm::SValue> theValueOpt
                = inContext.reader().GetInstancePropertyValue(instanceHandle(), theProperty);
        if (theValueOpt.hasValue()) {
            qt3dsdm::SValue &theValue(*theValueOpt);
            switch (qt3dsdm::GetValueType(theValue)) {
            case qt3dsdm::DataModelDataType::Long:
                if (element.type == Q3DS::PropertyType::Long)
                    theItem.setProperty(qPrintable(property),theValue.toQVariant());
                break;
            case qt3dsdm::DataModelDataType::Bool:
                if (element.type == Q3DS::PropertyType::Boolean)
                    theItem.setProperty(qPrintable(property), theValue.toQVariant());
                break;
            case qt3dsdm::DataModelDataType::Float:
                if (element.type == Q3DS::PropertyType::Float)
                    theItem.setProperty(qPrintable(property), theValue.toQVariant());
                break;
            case qt3dsdm::DataModelDataType::Float2:
                if (element.type == Q3DS::PropertyType::Float2)
                    theItem.setProperty(qPrintable(property), theValue.toQVariant());
                break;
            case qt3dsdm::DataModelDataType::Float3:
                if (element.type == Q3DS::PropertyType::Vector)
                    theItem.setProperty(qPrintable(property), theValue.toQVariant());
                break;
            // Could be either an enum or a texture.
            case qt3dsdm::DataModelDataType::String: {
                qt3dsdm::TDataStrPtr theData = qt3dsdm::get<qt3dsdm::TDataStrPtr>(theValue);
                if (theData) {
                    QString assetPath = inContext.presentation()
                            ->assetFileName(theValue.toQVariant().toString(), nullptr);
                    QFileInfo info(assetPath);
                    if (info.exists() && info.isFile())
                        theItem.setProperty(qPrintable(property), QVariant::fromValue(assetPath));
                    else
                        theItem.setProperty(qPrintable(property), theValue.toQVariant());
                }
            } break;
            default:
                Q_ASSERT_X(false, __FUNCTION__, "Incorrect datatype for effect.");
            }
        }
    }
}

void Q3DSDynamicObjectTranslator::setActive(bool)
{
}

void Q3DSDynamicObjectTranslator::clearChildren()
{
}

void Q3DSDynamicObjectTranslator::appendChild(Q3DSGraphObject &)
{
}

bool Q3DSDynamicObjectTranslator::updateProperty(Q3DSTranslation &inContext,
                                                 qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                 qt3dsdm::Qt3DSDMPropertyHandle property,
                                                 qt3dsdm::SValue &value,
                                                 const QString &name)
{
    if (Q3DSGraphObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSEffectInstance &theItem = static_cast<Q3DSEffectInstance &>(graphObject());
    QByteArray key = name.toUtf8();

    if (theItem.dynamicPropertyNames().contains(key)) {
        Q3DSPropertyChangeList list;
        if (qt3dsdm::GetValueType(value) == qt3dsdm::DataModelDataType::String) {
            QString assetPath = inContext.presentation()
                    ->assetFileName(value.toQVariant().toString(), nullptr);
            QFileInfo info(assetPath);
            if (info.exists() && info.isFile())
                theItem.setProperty(qPrintable(name), QVariant::fromValue(assetPath));
            else
                theItem.setProperty(qPrintable(name), value.toQVariant());
        } else {
            theItem.setProperty(qPrintable(name), value.toQVariant());
        }
        list.append(Q3DSPropertyChange(name));
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

void Q3DSDynamicObjectTranslator::copyProperties(Q3DSGraphObject *target, bool ignoreReferenced)
{
    Q3DSGraphObjectTranslator::copyProperties(target, ignoreReferenced);
    Q3DSGraphObject *targetObject = static_cast<Q3DSGraphObject *>(target);
    const Q3DSGraphObject &theItem = *graphObject<Q3DSGraphObject>();
    Q3DSPropertyChangeList list;
    const QVector<QByteArray> &propertyNames = theItem.dynamicPropertyNames();
    const QVector<QVariant> &propertyValues = theItem.dynamicPropertyValues();

    for (int i = 0; i < propertyNames.size(); ++i) {
        targetObject->setProperty(propertyNames[i], propertyValues[i]);
        list.append(Q3DSPropertyChange(propertyNames[i]));
    }
    targetObject->notifyPropertyChanges(list);
}

Q3DSEffectTranslator::Q3DSEffectTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                           Q3DSEffectInstance &effect)
    : Q3DSDynamicObjectTranslator(instance, effect)
{

}

void Q3DSEffectTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSDynamicObjectTranslator::pushTranslation(inContext);
    Q3DSEffectInstance &theItem = static_cast<Q3DSEffectInstance &>(graphObject());
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
    ITERATE_Q3DS_EFFECT_PROPERTIES
}

void Q3DSEffectTranslator::copyProperties(Q3DSGraphObject *target, bool ignoreReferenced)
{
    Q3DSDynamicObjectTranslator::copyProperties(target, ignoreReferenced);
    Q3DSEffectInstance *targetEffect = static_cast<Q3DSEffectInstance *>(target);
    const Q3DSEffectInstance &theItem = *graphObject<Q3DSEffectInstance>();
    targetEffect->setEyeballEnabled(theItem.eyeballEnabled());
}

bool Q3DSEffectTranslator::updateProperty(Q3DSTranslation &inContext,
                                          qt3dsdm::Qt3DSDMInstanceHandle instance,
                                          qt3dsdm::Qt3DSDMPropertyHandle property,
                                          qt3dsdm::SValue &value,
                                          const QString &name)
{
    if (Q3DSDynamicObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSEffectInstance &theItem = static_cast<Q3DSEffectInstance &>(graphObject());
    if (name == QLatin1String("eyeball")) {
        list.append(theItem.setEyeballEnabled(value.getData<bool>()));
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

bool Q3DSEffectTranslator::shaderRequiresRecompilation(Q3DSTranslation &inContext,
                                                       const qt3dsdm::SValue &value,
                                                       const QString &name,
                                                       qt3dsdm::AdditionalMetaDataType::Value type)
{
    Q_UNUSED(inContext);
    Q_UNUSED(value);
    Q_UNUSED(name);
    Q_UNUSED(type);
    return false;
}


Q3DSCustomMaterialTranslator::Q3DSCustomMaterialTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                           Q3DSCustomMaterialInstance &material)
    : Q3DSDynamicObjectTranslator(instance, material)
{

}

void Q3DSCustomMaterialTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSDynamicObjectTranslator::pushTranslation(inContext);
    Q3DSCustomMaterialInstance &theItem = static_cast<Q3DSCustomMaterialInstance &>(graphObject());
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    Q3DSPropertyChangeList list;
    ITERATE_Q3DS_CUSTOM_MATERIAL_PROPERTIES
}

bool Q3DSCustomMaterialTranslator::updateProperty(Q3DSTranslation &inContext,
                                                  qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                  qt3dsdm::Qt3DSDMPropertyHandle property,
                                                  qt3dsdm::SValue &value,
                                                  const QString &name)
{
    if (Q3DSDynamicObjectTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    Q3DSPropertyChangeList list;
    Q3DSCustomMaterialInstance &theItem = static_cast<Q3DSCustomMaterialInstance &>(graphObject());

    if (name == QLatin1String("lightmapindirect")) {
        list.append(theItem.setLightmapIndirectMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("lightmapradiosity")) {
        list.append(theItem.setLightmapRadiosityMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    } else if (name == QLatin1String("lightmapshadow")) {
        list.append(theItem.setLightmapShadowMap(
                          Q3DSValueParser::parseImage(&inContext, value, &graphObject())));
    }
    if (list.count()) {
        theItem.notifyPropertyChanges(list);
        return true;
    }
    return false;
}

void Q3DSCustomMaterialTranslator::copyProperties(Q3DSGraphObject *target, bool ignoreReferenced)
{
    Q3DSDynamicObjectTranslator::copyProperties(target, ignoreReferenced);
    Q3DSPropertyChangeList list;
    Q3DSCustomMaterialInstance *targetMaterial
            = static_cast<Q3DSCustomMaterialInstance *>(target);
    const Q3DSCustomMaterialInstance &theItem = *graphObject<Q3DSCustomMaterialInstance>();
    list.append(targetMaterial->setLightProbe(theItem.lightProbe()));
    list.append(targetMaterial->setLightmapIndirectMap(theItem.lightmapIndirectMap()));
    list.append(targetMaterial->setLightmapRadiosityMap(theItem.lightmapRadiosityMap()));
    list.append(targetMaterial->setLightmapShadowMap(theItem.lightmapShadowMap()));
    list.append(targetMaterial->setLightmapIndirectMap(theItem.lightmapIndirectMap()));
    list.append(targetMaterial->setSourcePath(theItem.sourcePath()));
    list.append(targetMaterial->setLightProbe(theItem.lightProbe()));
    targetMaterial->notifyPropertyChanges(list);
}

bool Q3DSCustomMaterialTranslator::shaderRequiresRecompilation(
        Q3DSTranslation &inContext, const qt3dsdm::SValue &value, const QString &name,
        qt3dsdm::AdditionalMetaDataType::Value type)
{
    Q_UNUSED(inContext);
    Q_UNUSED(value);
    Q_UNUSED(name);
    Q_UNUSED(type);
    return false;
}


Q3DSAliasTranslator::Q3DSAliasTranslator(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                         Q3DSNode &aliasNode)
    : Q3DSNodeTranslator(instance, aliasNode)
{

}

void Q3DSAliasTranslator::pushTranslation(Q3DSTranslation &inContext)
{
    Q3DSTranslatorDataModelParser theParser(inContext, instanceHandle());
    qt3dsdm::SObjectRefType objectReference;
    if (theParser.parseProperty(inContext.objectDefinitions().m_Alias.m_ReferencedNode,
                                objectReference)) {
        qt3dsdm::Qt3DSDMInstanceHandle referenceHandle = qt3dsdm::Qt3DSDMInstanceHandle();
        m_referencedInstance
                = inContext.reader().GetInstanceForObjectRef(instanceHandle(), objectReference);
        if (inContext.reader().IsInstance(m_referencedInstance)) {
            m_referencedTree = inContext.getOrCreateTranslator(m_referencedInstance,
                                                               instanceHandle(), this);
            if (m_referencedTree && !m_referencedTree->graphObject().isNode()) {
                delete m_referencedTree;
                m_referencedTree = nullptr;
                m_referencedInstance = qt3dsdm::Qt3DSDMInstanceHandle();
            } else if (m_referencedTree) {
                createTranslatorsRecursive(inContext, m_referencedInstance, m_referencedTree);
                m_referencedTree->pushTranslation(inContext);
            }
        }
    }
    graphObject().appendChildNode(&m_referencedTree->graphObject());
    Q3DSNodeTranslator::pushTranslation(inContext);

    /* Alias nodes need to add themself to the slide here */
    std::shared_ptr<qt3dsdm::ISlideCore> slideCore = inContext.fullSystem().GetSlideCore();
    qt3dsdm::Qt3DSDMSlideHandle slideHandle
            = inContext.reader().GetAssociatedSlide(m_referencedInstance);
    qt3dsdm::Qt3DSDMInstanceHandle slideInstance = slideCore->GetSlideInstance(slideHandle);
    Q3DSGraphObjectTranslator *slideTranslator = inContext.getOrCreateTranslator(slideInstance);
    addToSlide(slideTranslator->graphObject<Q3DSSlide>());
}

void Q3DSAliasTranslator::createTranslatorsRecursive(Q3DSTranslation &inContext,
                                                     qt3dsdm::Qt3DSDMInstanceHandle instance,
                                                     Q3DSGraphObjectTranslator *translator)
{
    long childCount = inContext.assetGraph().GetChildCount(instance);
    for (long idx = 0; idx < childCount; ++idx) {
        qt3dsdm::Qt3DSDMInstanceHandle theChild = inContext.assetGraph().GetChild(instance, idx);
        Q3DSGraphObjectTranslator *childTranslator
                = inContext.getOrCreateTranslator(theChild, instanceHandle(), this);
        translator->graphObject().appendChildNode(&childTranslator->graphObject());
        childTranslator->pushTranslation(inContext);
        createTranslatorsRecursive(inContext, theChild, childTranslator);
    }
}

bool Q3DSAliasTranslator::updateProperty(Q3DSTranslation &inContext,
                                         qt3dsdm::Qt3DSDMInstanceHandle instance,
                                         qt3dsdm::Qt3DSDMPropertyHandle property,
                                         qt3dsdm::SValue &value,
                                         const QString &name)
{
    if (Q3DSNodeTranslator::updateProperty(inContext, instance, property, value, name))
        return true;

    return false;
}

void Q3DSAliasTranslator::copyProperties(Q3DSGraphObject *, bool)
{
    // copy alias properties does not make sense
    Q_ASSERT_X(false, __FUNCTION__, "Alias node can not be copied");
}

void Q3DSAliasTranslator::addToSlide(Q3DSSlide *slide)
{
    if (m_referencedTree)
        addToSlideRecursive(m_referencedTree, slide);
}

void Q3DSAliasTranslator::addToSlideRecursive(Q3DSGraphObjectTranslator *translator,
                                              Q3DSSlide *slide)
{
    Q3DSGraphObject *obj = &translator->graphObject();
    slide->addObject(obj);
    obj = obj->firstChild();
    while (obj) {
        translator = Q3DSGraphObjectTranslator::translatorForObject(obj);
        addToSlideRecursive(translator, slide);
        obj = obj->nextSibling();
    }
}

}
