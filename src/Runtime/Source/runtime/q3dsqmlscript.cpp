/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "q3dsqmlscript.h"

#include "Qt3DSQmlEngine.h"
#include "Qt3DSElementSystem.h"
#include "Qt3DSApplication.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSInputEngine.h"
#include "Qt3DSInputFrame.h"
#include "Qt3DSQmlElementHelper.h"
#include "Qt3DSHash.h"
#include "Qt3DSEulerAngles.h"

using namespace Q3DStudio;

QJSValue argToQJSValue(qt3ds::foundation::IStringTable &strTable,
                       Q3DStudio::UINT8 type, const UVariant &value) {
    switch (type) {
    case ATTRIBUTETYPE_INT32:
    case ATTRIBUTETYPE_HASH:
        return QJSValue(value.m_INT32);
    case ATTRIBUTETYPE_FLOAT:
        return QJSValue(value.m_FLOAT);
    case ATTRIBUTETYPE_BOOL:
        return QJSValue(value.m_INT32 != 0);
    case ATTRIBUTETYPE_STRING:
        return QJSValue(strTable.HandleToStr(value.m_StringHandle));
    case ATTRIBUTETYPE_NONE:
        return QJSValue();
    default:
        qCCritical(qt3ds::INVALID_OPERATION)
                << "Pushed event argument value of unknown type: "<< type;
    }
    return QJSValue();
}

void eventCallback(void *contextData, SEventCommand &event)
{
    auto data = static_cast<Q3DSQmlScript::EventData *>(contextData);
    qt3ds::foundation::IStringTable &strTable(
                event.m_Target->GetBelongedPresentation()->GetStringTable());
    QJSValue arg1 = argToQJSValue(strTable, event.m_Arg1Type, event.m_Arg1);
    QJSValue arg2 = argToQJSValue(strTable, event.m_Arg2Type, event.m_Arg2);
    data->function.call({arg1, arg2});
}

Q3DSQmlScript::Q3DSQmlScript(CQmlEngine &api, Q3DSQmlBehavior &object,
                             TElement &behavior, TElement &owner)
    : QObject(nullptr)
    , m_api(api)
    , m_object(object)
    , m_behavior(behavior)
    , m_owner(owner)
    , m_initialized(false)
    , m_lastActivationState(false)
    , m_deltaTime(0.0f)
    , m_lastTime(0)
{
    updateProperties();
}

Q3DSQmlScript::~Q3DSQmlScript()
{
    for (const EventCallbackInfo &callback : m_eventCallbacks)
        delete callback.data;
}

void Q3DSQmlScript::update()
{
    updateProperties();

    bool active = m_behavior.GetActive();

    if (active && !m_initialized) {
        m_initialized = true;
        Q_EMIT m_object.initialize();
    }

    TTimeUnit time = static_cast<CPresentation *>(m_behavior.GetBelongedPresentation())->GetTime();
    m_deltaTime = (time - m_lastTime) / 1000.0f;
    m_lastTime = time;

    if (m_lastActivationState != active) {
        if (active)
            Q_EMIT m_object.activate();
        else
            Q_EMIT m_object.deactivate();
    }
    m_lastActivationState = active;
    if (active)
        Q_EMIT m_object.update();
}

void Q3DSQmlScript::call(const QString &function)
{
    const QMetaObject *meta = m_object.metaObject();
    const QString normalized = function + "()";
    if (meta->indexOfMethod(normalized.toUtf8().constData()) != -1)
        QMetaObject::invokeMethod(&m_object, function.toUtf8().constData());
}

void Q3DSQmlScript::updateProperties()
{
    using namespace qt3ds::foundation;
    using namespace qt3ds::runtime::element;

    if (!m_behavior.GetActive())
        return;

    unsigned int numProperties = m_behavior.GetNumProperties();
    for (unsigned int i = 0; i < numProperties; ++i) {
        Option<TPropertyDescAndValuePtr> property = m_behavior.GetPropertyByIndex(i);
        if (!property.hasValue())
            break;

        TPropertyDescAndValuePtr value = property.getValue();
        const char *name = value.first.m_Name.c_str();

        UVariant *valuePtr = value.second;
        switch (value.first.m_Type) {
        case ATTRIBUTETYPE_INT32:
        case ATTRIBUTETYPE_HASH:
            m_object.setProperty(name, valuePtr->m_INT32);
            break;
        case ATTRIBUTETYPE_FLOAT:
            m_object.setProperty(name, valuePtr->m_FLOAT);
            break;
        case ATTRIBUTETYPE_BOOL:
            m_object.setProperty(name, valuePtr->m_INT32 != 0);
            break;
        case ATTRIBUTETYPE_STRING:
            m_object.setProperty(
                name,
                m_behavior.GetBelongedPresentation()->GetStringTable()
                .HandleToStr(valuePtr->m_StringHandle)
                .c_str());
            break;
        default:
            QT3DS_ASSERT(false);
        }
    }
}

bool Q3DSQmlScript::hasBehavior(const TElement *behavior)
{
    return &m_behavior == behavior;
}

float Q3DSQmlScript::getDeltaTime()
{
    return m_deltaTime;
}

float Q3DSQmlScript::getAttribute(const QString &attribute)
{
    if (!m_behavior.GetActive())
        return 0;

    float floatValue = 0;
    m_api.GetAttribute(m_owner.m_Path,
                       attribute.toUtf8().constData(),
                       (char *)&floatValue);
    return floatValue;
}

void Q3DSQmlScript::setAttribute(const QString &attribute, const QVariant &value)
{
    setAttribute("", attribute, value);
}

void Q3DSQmlScript::setAttribute(const QString &handle, const QString &attribute,
                                 const QVariant &value)
{
    if (!m_behavior.GetActive())
        return;

    TElement *element = &m_owner;
    if (!handle.isEmpty())
        element = getElementByPath(handle);
    if (!element)
        return;

    QByteArray valueStr;
    float valueFloat;

    const char *valuePtr = nullptr;
    switch (static_cast<QMetaType::Type>(value.type())) {
    case QMetaType::Bool:
    case QMetaType::Int:
    case QMetaType::Double:
    case QMetaType::Float:
        valueFloat = value.toFloat();
        valuePtr = (const char *)&valueFloat;
        break;
    case QMetaType::QString:
    default:
        valueStr = value.toString().toUtf8();
        valuePtr = valueStr.constData();
        break;
    }

    m_api.SetAttribute(element,
                       attribute.toUtf8().constData(),
                       valuePtr);
}

void Q3DSQmlScript::fireEvent(const QString &event)
{
    if (!m_behavior.GetActive())
        return;
    m_api.FireEvent(m_behavior.m_Path, event.toUtf8().constData());
}

void Q3DSQmlScript::registerForEvent(const QString &event, const QJSValue &function)
{
    registerForEvent("", event, function);
}

void Q3DSQmlScript::registerForEvent(const QString &handle, const QString &event,
                                     const QJSValue &function)
{
    if (!m_behavior.GetActive())
        return;

    TElement *element = &m_owner;
    if (!handle.isEmpty())
        element = getElementByPath(handle);
    if (!element)
        return;

    if (!function.isCallable())
        return;

    CPresentation *presentation
            = static_cast<CPresentation *>(m_behavior.GetBelongedPresentation());
    TEventCommandHash eventHash = CHash::HashEventCommand(event.toUtf8().constData());

    for (auto &&callback : m_eventCallbacks) {
        if (callback.element == element && callback.eventHash == eventHash)
            return;
    }

    EventData *data = new EventData();
    data->function = function;

    m_eventCallbacks.push_back({element, eventHash, data});

    presentation->RegisterEventCallback(element, eventHash, &eventCallback, data);
}

void Q3DSQmlScript::unregisterForEvent(const QString &event)
{
    unregisterForEvent("", event);
}

void Q3DSQmlScript::unregisterForEvent(const QString &handle, const QString &event)
{
    if (!m_behavior.GetActive())
        return;

    TElement *element = &m_owner;
    if (!handle.isEmpty())
        element = getElementByPath(handle);
    if (!element)
        return;

    CPresentation *presentation
            = static_cast<CPresentation *>(m_behavior.GetBelongedPresentation());
    TEventCommandHash eventHash = CHash::HashEventCommand(event.toUtf8().constData());
    EventData *data = nullptr;

    for (int i = 0; i < m_eventCallbacks.size(); ++i) {
        if (m_eventCallbacks[i].element == element && m_eventCallbacks[i].eventHash == eventHash) {
            data = m_eventCallbacks[i].data;
            m_eventCallbacks.erase(m_eventCallbacks.begin() + i);
            break;
        }
    }

    if (data) {
        presentation->UnregisterEventCallback(element, eventHash, &eventCallback, data);
        delete data;
    }
}

QVector2D Q3DSQmlScript::getMousePosition()
{
    CPresentation *presentation
            = static_cast<CPresentation *>(m_behavior.GetBelongedPresentation());
    qt3ds::runtime::IApplication &app = presentation->GetApplication();

    SInputFrame input = app.GetInputEngine().GetInputFrame();
    if (app.GetPrimaryPresentation()) {
        Q3DStudio::SMousePosition position =
            app.GetPrimaryPresentation()->GetScene()->WindowToPresentation(
                Q3DStudio::SMousePosition(static_cast<INT32>(input.m_PickX),
                                          static_cast<INT32>(input.m_PickY)));

        return QVector2D(position.m_X, position.m_Y);
    }
    return QVector2D(0, 0);
}

QMatrix4x4 Q3DSQmlScript::calculateGlobalTransform(const QString &handle)
{
    TElement *element = &m_owner;
    if (!handle.isEmpty())
        element = getElementByPath(handle);
    if (!element)
        return QMatrix4x4();

    RuntimeMatrix transform;
    IScene *scene = element->GetBelongedPresentation()->GetScene();
    scene->CalculateGlobalTransform(element, transform);
    transform.FlipCoordinateSystem();

    return QMatrix4x4(&transform.m_Data[0][0]);
}

QVector3D Q3DSQmlScript::lookAt(const QVector3D &target)
{
    RuntimeVector3 rotation;

    FLOAT theMag = ::sqrtf(target.x() * target.x() + target.z() * target.z());
    FLOAT thePitch = -::atan2f(target.y(), theMag);
    FLOAT theYaw = ::atan2f(target.x(), target.z());

    rotation.Set(thePitch, theYaw, 0.0f);

    return QVector3D(rotation.m_X, rotation.m_Y, rotation.m_Z);
}

QVector3D Q3DSQmlScript::matrixToEuler(const QMatrix4x4 &matrix)
{
    CEulerAngleConverter converter;
    const float *qMatrix = matrix.constData();
    HMatrix hHatrix;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j)
            hHatrix[i][j] = qMatrix[j * 4 + i];
    }
    EulerAngles eulerAngles = converter.Eul_FromHMatrix(hHatrix, EulOrdYXZs);
    return QVector3D(-eulerAngles.y, -eulerAngles.x, -eulerAngles.z);
}

QString Q3DSQmlScript::getParent(const QString &handle)
{
    TElement *element = &m_owner;
    if (!handle.isEmpty())
        element = getElementByPath(handle);
    if (!element)
        return "";

    TElement *parent = element->GetParent();
    if (!parent)
        return "";
    return parent->m_Path.c_str();
}

void Q3DSQmlScript::setDataInputValue(const QString &name, const QVariant &value,
                                      Q3DSDataInput::ValueRole valueRole)
{
    m_api.SetDataInputValue(name, value, valueRole);
}

TElement *Q3DSQmlScript::getElementByPath(const QString &path)
{
    if (!m_api.GetApplication())
        return nullptr;

    TElement *element = CQmlElementHelper::GetElement(
                        *m_api.GetApplication(),
                        m_api.GetApplication()->GetPrimaryPresentation(),
                        path.toUtf8().constData(), &m_owner);
    return element;
}
