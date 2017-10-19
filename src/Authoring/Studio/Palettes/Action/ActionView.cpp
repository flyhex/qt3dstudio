/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "ActionView.h"
#include "ActionContextMenu.h"
#include "ActionModel.h"
#include "CmdDataModelActionSetValue.h"
#include "ClientDataModelBridge.h"
#include "Core.h"
#include "Dispatch.h"
#include "Doc.h"
#include "IDocumentEditor.h"
#include "IDocumentReader.h"
#include "IObjectReferenceHelper.h"
#include "Literals.h"
#include "ObjectListModel.h"
#include "StudioUtils.h"
#include "StudioApp.h"
#include "StudioClipboard.h"
#include "StudioObjectTypes.h"
#include "StudioPreferences.h"
#include "Qt3DSFileTools.h"
#include "Qt3DSDMActionCore.h"
#include "Qt3DSDMDataTypes.h"
#include "Qt3DSDMSlides.h"

#include <QCoreApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTimer>

ActionView::ActionView(QWidget *parent)
    : QQuickWidget(parent)
    , TabNavigable()
    , m_actionsModel(new ActionModel(this))
{
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &ActionView::initialize);

    g_StudioApp.GetCore()->GetDispatch()->AddPresentationChangeListener(this);

    connect(this, &ActionView::actionChanged, this, [this] {
        if (!m_propertyModel)
            m_propertyModel = new PropertyModel(this);

        const auto actionInfo = m_actionsModel->actionInfoAt(m_currentActionIndex);
        if (actionInfo.m_Handler == L"Set Property") {
            m_currentPropertyNameHandle = actionInfo.m_HandlerArgs.at(0);
            m_currentPropertyValueHandle = actionInfo.m_HandlerArgs.at(1);
            m_propertyModel->setAction(m_actionsModel->actionAt(m_currentActionIndex));
            m_propertyModel->setNameHandle(m_currentPropertyNameHandle);
            m_propertyModel->setValueHandle(m_currentPropertyValueHandle);

            Q_EMIT propertyModelChanged();
        }

        });

    m_actionChangedCompressionTimer.setInterval(500);
    m_actionChangedCompressionTimer.setSingleShot(true);
    connect(&m_actionChangedCompressionTimer, &QTimer::timeout, this, [this] {
        updateHandlerArguments();
        updateFiredEvent();
        Q_EMIT actionChanged();
    });
}

ActionView::~ActionView()
{
    m_connections.clear();
}

QSize ActionView::sizeHint() const
{
    return {500, 500};
}

void ActionView::setItem(const qt3dsdm::Qt3DSDMInstanceHandle &handle)
{
    m_objRefHelper = GetDoc()->GetDataModelObjectReferenceHelper();
    m_itemHandle = handle;
    m_actionsModel->setInstanceHandle(handle);
    emitActionChanged();
    Q_EMIT itemChanged();
}

QString ActionView::itemIcon() const
{
    if (!m_itemHandle.Valid())
        return {};

    auto info = m_objRefHelper->GetInfo(m_itemHandle);
    return CStudioObjectTypes::GetNormalIconName(info.m_Type);
}

QString ActionView::itemText() const
{
    if (!m_itemHandle.Valid())
        return tr("No Object Selected");

    const auto data = m_objRefHelper->GetInfo(m_itemHandle);
    return data.m_Name.toQString();
}

QColor ActionView::itemColor() const
{
    if (!m_itemHandle.Valid())
        return Qt::white;

    auto info = m_objRefHelper->GetInfo(m_itemHandle);
    if (info.m_Master)
        return CStudioPreferences::masterColor();
    else
        return CStudioPreferences::textColor();
}

QAbstractItemModel *ActionView::actionsModel() const
{
    return m_actionsModel;
}

QAbstractItemModel *ActionView::propertyModel() const
{
    return m_propertyModel;
}

QString ActionView::targetObjectName() const
{
    if (!GetDoc()->IsValid())
        return {};

    const auto actionInfo = m_actionsModel->actionInfoAt(m_currentActionIndex);

    const auto targetInstance =
        GetBridge()->GetInstance(actionInfo.m_Owner, actionInfo.m_TargetObject);

    QString targetName = targetInstance.Valid()
        ? GetBridge()->GetName(targetInstance).toQString()
        : tr("[Unknown Target]");

    return targetName;
}

QString ActionView::triggerObjectName() const
{
    if (!GetDoc()->IsValid())
        return {};

    const auto actionInfo = m_actionsModel->actionInfoAt(m_currentActionIndex);

    const auto sourceInstance =
        GetBridge()->GetInstance(actionInfo.m_Owner, actionInfo.m_TriggerObject);

    QString sourceName = sourceInstance.Valid()
        ? GetBridge()->GetName(sourceInstance).toQString()
        : tr("[Unknown Source]");

    return sourceName;
}

QString ActionView::eventName() const
{
    if (!GetDoc()->IsValid())
        return {};

    const auto actionInfo = m_actionsModel->actionInfoAt(m_currentActionIndex);
    const auto bridge = GetBridge();
    const auto eventHandle = bridge->ResolveEvent(actionInfo);
    const auto eventInfo = bridge->GetEventInfo(eventHandle);

    const QString formalName = QString::fromWCharArray(eventInfo.m_FormalName.wide_str());
    return formalName.isEmpty() ? tr("[Unknown Event]") : formalName;
}

QString ActionView::handlerName() const
{
    if (!GetDoc()->IsValid())
        return {};

    const auto actionInfo = m_actionsModel->actionInfoAt(m_currentActionIndex);
    const auto bridge = GetBridge();
    const auto handlerHandle = bridge->ResolveHandler(actionInfo);

    if (handlerHandle.Valid()) {
        const auto handlerInfo = bridge->GetHandlerInfo(handlerHandle);
        return QString::fromWCharArray(handlerInfo.m_FormalName.wide_str());
    }

    return tr("[Unknown Handler]");
}

QVariantList ActionView::handlerArguments() const
{
    return m_handlerArguments;
}

PropertyInfo ActionView::property() const
{
    if (!m_propertyModel)
        return {};
    return m_propertyModel->property(m_currentPropertyIndex);
}

void ActionView::setCurrentActionIndex(int index)
{
    if (index == m_currentActionIndex)
        return;

    m_currentActionIndex = index;
    emitActionChanged();
}

void ActionView::setCurrentPropertyIndex(int handle, int index)
{
    if (index == m_currentPropertyIndex)
        return;

    m_currentPropertyValueHandle = 0;
    m_currentPropertyNameHandle = handle;
    for (int i = 0; i < m_handlerArguments.size(); ++i) {
        auto handlerArg = m_handlerArguments[i].value<HandlerArgument>();
        if (handlerArg.m_handle.GetHandleValue() == handle && i < m_handlerArguments.size() - 1) {
            m_currentPropertyValueHandle = m_handlerArguments[i + 1].value<HandlerArgument>().m_handle;
            if (m_propertyModel) {
                m_propertyModel->setNameHandle(m_currentPropertyNameHandle);
                m_propertyModel->setValueHandle(m_currentPropertyValueHandle);
            }
        }
    }

    m_currentPropertyIndex = index;

    // set the property for the handler
    if (m_propertyModel && handle != 0) {
        qt3dsdm::SValue sValue(QVariant(m_propertyModel->property(index).m_nameId));
        qt3dsdm::SValue oldValue;
        GetDoc()->GetStudioSystem()->GetActionCore()->GetHandlerArgumentValue(handle, oldValue);

        if (!Equals(oldValue, sValue)) {
            CCmd *theCmd =
                    new CCmdDataModelActionSetArgumentValue(GetDoc(), handle, sValue);
            g_StudioApp.GetCore()->ExecuteCommand(theCmd);
        }
    }

    Q_EMIT propertyChanged();
}

void ActionView::addAction()
{
    if (m_itemHandle.Valid()) {
        // Query data model bridge to see the applicable events and actions for this instance.
        CClientDataModelBridge *theBridge = GetBridge();

        std::wstring theEventName = theBridge->GetDefaultEvent(m_itemHandle);
        std::wstring theHandlerName = theBridge->GetDefaultHandler(m_itemHandle);

        Q3DStudio::SCOPED_DOCUMENT_EDITOR(*GetDoc(), QObject::tr("Add Action"))
            ->AddAction(GetDoc()->GetActiveSlide(), m_itemHandle, theEventName,
                        theHandlerName);
    }
}

void ActionView::deleteAction(int index)
{
    const auto action = m_actionsModel->actionAt(index);
    if (action.Valid()) {
        Q3DStudio::SCOPED_DOCUMENT_EDITOR(*GetDoc(), QObject::tr("Delete Action"))->DeleteAction(action);
    }
}

QObject *ActionView::showTriggerObjectBrowser(const QPoint &point)
{
    if (!m_objectsModel) {
        m_objectsModel = new ObjectListModel(g_StudioApp.GetCore(),
                                             GetDoc()->GetActiveRootInstance(), this);
    }

    if (!m_triggerObjectBrowser)
        m_triggerObjectBrowser = new ObjectBrowserView(this);

    m_triggerObjectBrowser->setModel(m_objectsModel);

    showBrowser(m_triggerObjectBrowser, point);

    connect(m_triggerObjectBrowser, &ObjectBrowserView::selectionChanged,
            this, [this] {
        auto selectedItem = m_triggerObjectBrowser->selectedHandle();
        setTriggerObject(m_objRefHelper->GetAssetRefValue(selectedItem,
                                                         m_itemHandle,
                                                         (CRelativePathTools::EPathType)(m_triggerObjectBrowser->pathType())));
    });

    return m_triggerObjectBrowser;
}

QObject *ActionView::showTargetObjectBrowser(const QPoint &point)
{
    if (!m_objectsModel) {
        m_objectsModel = new ObjectListModel(g_StudioApp.GetCore(),
                                             GetDoc()->GetActiveRootInstance(), this);
    }

    if (!m_targetObjectBrowser)
        m_targetObjectBrowser = new ObjectBrowserView(this);

    m_targetObjectBrowser->setModel(m_objectsModel);

    showBrowser(m_targetObjectBrowser, point);

    connect(m_targetObjectBrowser, &ObjectBrowserView::selectionChanged,
            this, [this] {
        auto selectedItem = m_targetObjectBrowser->selectedHandle();
        setTargetObject(m_objRefHelper->GetAssetRefValue(selectedItem,
                                                         m_itemHandle,
                                                         (CRelativePathTools::EPathType)(m_targetObjectBrowser->pathType())));
        resetFiredEvent();
    });

    return m_targetObjectBrowser;
}

void ActionView::showContextMenu(int x, int y)
{
    CActionContextMenu contextMenu(this);

    connect(&contextMenu, &CActionContextMenu::copyAction, this, &ActionView::copyAction);
    connect(&contextMenu, &CActionContextMenu::pasteAction, this, &ActionView::pasteAction);
    connect(&contextMenu, &CActionContextMenu::cutAction, this, &ActionView::cutAction);
    connect(&contextMenu, &CActionContextMenu::deleteAction, this, [this] {
        deleteAction(m_currentActionIndex);
    });

    contextMenu.exec(mapToGlobal({x, y}));
}

QObject *ActionView::showEventBrowser(const QPoint &point)
{
    const auto actionInfo = m_actionsModel->actionInfoAt(m_currentActionIndex);
    const auto bridge = GetBridge();
    const auto instanceHandle = bridge->GetInstance(actionInfo.m_Owner, actionInfo.m_TriggerObject);

    if (!instanceHandle.Valid())
        return nullptr;

    if (!m_eventsModel) {
        m_eventsModel = new EventsModel(this);
    }

    qt3dsdm::TEventHandleList eventList;
    bridge->GetEvents(instanceHandle, eventList);
    m_eventsModel->setEventList(eventList);

    if (!m_eventsBrowser)
        m_eventsBrowser = new EventsBrowserView(this);

    m_eventsBrowser->setModel(m_eventsModel);

    showBrowser(m_eventsBrowser, point);

    connect(m_eventsBrowser, &EventsBrowserView::selectionChanged,
            this, [this] {
        setEvent(qt3dsdm::CUICDMEventHandle(m_eventsBrowser->selectedHandle()));
    });

    return m_eventsBrowser;
}

QObject *ActionView::showHandlerBrowser(const QPoint &point)
{
    const auto actionInfo = m_actionsModel->actionInfoAt(m_currentActionIndex);
    const auto bridge = GetBridge();
    const auto instanceHandle = bridge->GetInstance(actionInfo.m_Owner, actionInfo.m_TargetObject);

    if (!instanceHandle.Valid())
        return nullptr;

    if (!m_handlersModel) {
        m_handlersModel = new EventsModel(this);
    }

    qt3dsdm::THandlerHandleList handlerList;
    bridge->GetHandlers(instanceHandle, handlerList);
    m_handlersModel->setHandlerList(handlerList);

    if (!m_handlerBrowser)
        m_handlerBrowser = new EventsBrowserView(this);

    m_handlerBrowser->setModel(m_handlersModel);

    showBrowser(m_handlerBrowser, point);

    connect(m_handlerBrowser, &EventsBrowserView::selectionChanged,
            this, [this] {
        setHandler(qt3dsdm::CUICDMHandlerHandle(m_handlerBrowser->selectedHandle()));
    });

    return m_handlerBrowser;
}

QObject *ActionView::showEventBrowserForArgument(int handle, const QPoint &point)
{
    const auto actionInfo = m_actionsModel->actionInfoAt(m_currentActionIndex);
    const auto bridge = GetBridge();
    const auto instanceHandle = bridge->GetInstance(actionInfo.m_Owner, actionInfo.m_TargetObject);

    if (!instanceHandle.Valid())
        return nullptr;

    if (!m_fireEventsModel) {
        m_fireEventsModel = new EventsModel(this);
    }

    qt3dsdm::TEventHandleList eventList;
    bridge->GetEvents(instanceHandle, eventList);
    m_fireEventsModel->setEventList(eventList);

    if (!m_fireEventsBrowser)
        m_fireEventsBrowser = new EventsBrowserView(this);

    m_fireEventsBrowser->setModel(m_fireEventsModel);

    showBrowser(m_fireEventsBrowser, point);

    connect(m_fireEventsBrowser, &EventsBrowserView::selectionChanged,
            this, [this, handle] {
          setArgumentValue(handle, qt3dsdm::CUICDMEventHandle(m_fireEventsBrowser->selectedHandle()).GetHandleValue());
    });

    return m_fireEventsBrowser;
}

void ActionView::showBrowser(QQuickWidget *browser, const QPoint &point)
{
    QSize popupSize = CStudioPreferences::browserPopupSize();
    browser->disconnect();
    browser->resize(popupSize);
    browser->move(point - QPoint(popupSize.width(), popupSize.height()));

    // Show asynchronously to avoid flashing blank window on first show
    QTimer::singleShot(0, this, [browser] {
        browser->show();
        browser->activateWindow();
        browser->setFocus();
    });
}

void ActionView::updateFiredEvent()
{
    const auto actionInfo = m_actionsModel->actionInfoAt(m_currentActionIndex);
    if (actionInfo.m_Handler != L"Fire Event") {
        m_firedEvent = tr("[Unknown event]");
        return;
    }

    const auto doc = GetDoc();
    if (!doc->IsValid())
        return;

    const auto bridge = GetBridge();
    const auto handlerHandle = bridge->ResolveHandler(actionInfo);
    IActionCore *actionCore = doc->GetStudioSystem()->GetActionCore();

    if (handlerHandle.Valid()) {
        for (const auto &argHandle: actionInfo.m_HandlerArgs) {
            const auto &argumentInfo = actionCore->GetHandlerArgumentInfo(argHandle);
            DataModelDataType::Value theArgType(GetValueType(argumentInfo.m_Value));
            SValue theArgValue(argumentInfo.m_Value);
            if (argumentInfo.m_ArgType == HandlerArgumentType::Event) {
                theArgType = DataModelDataType::String;
                auto theEventHandle = get<qt3ds::QT3DSI32>(argumentInfo.m_Value);
                theArgValue = SValue(std::make_shared<CDataStr>(
                                         bridge->GetEventInfo(theEventHandle).m_Name.wide_str()));
                m_firedEvent = theArgValue.toQVariant().toString();
                Q_EMIT firedEventChanged();
            }
        }
    }
}

void ActionView::updateFiredEventFromHandle(int handle)
{
    m_firedEvent = QString::fromWCharArray(GetBridge()->GetEventInfo(handle).m_FormalName.wide_str());
    Q_EMIT firedEventChanged();
}

void ActionView::resetFiredEvent()
{
    m_firedEvent = tr("[Unknown Event]");
    Q_EMIT firedEventChanged();
}

void ActionView::OnNewPresentation()
{
    // Register callback
    qt3dsdm::IStudioFullSystemSignalProvider *theSignalProvider =
        g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetFullSystemSignalProvider();
    m_connections.push_back(theSignalProvider->ConnectActionCreated(
        std::bind(&ActionView::OnActionAdded, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)));
    m_connections.push_back(theSignalProvider->ConnectActionDeleted(
        std::bind(&ActionView::OnActionDeleted, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)));
    m_connections.push_back(theSignalProvider->ConnectTriggerObjectSet(
        std::bind(&ActionView::OnActionModified, this, std::placeholders::_1)));
    m_connections.push_back(theSignalProvider->ConnectTargetObjectSet(
        std::bind(&ActionView::OnActionModified, this, std::placeholders::_1)));
    m_connections.push_back(theSignalProvider->ConnectEventSet(
        std::bind(&ActionView::OnActionModified, this, std::placeholders::_1)));
    m_connections.push_back(theSignalProvider->ConnectHandlerSet(
        std::bind(&ActionView::OnActionModified, this, std::placeholders::_1)));
    m_connections.push_back(theSignalProvider->ConnectHandlerArgumentValueSet(
        std::bind(&ActionView::OnHandlerArgumentModified, this, std::placeholders::_1)));
    m_connections.push_back(theSignalProvider->ConnectInstancePropertyValue(
        std::bind(&ActionView::OnInstancePropertyValueChanged, this, std::placeholders::_1,
                  std::placeholders::_2)));
    CDispatch *theDispatch = g_StudioApp.GetCore()->GetDispatch();
    m_connections.push_back(theDispatch->ConnectSelectionChange(
                                std::bind(&ActionView::OnSelectionSet, this,
                                          std::placeholders::_1)));
}

void ActionView::OnSelectionSet(Q3DStudio::SSelectedValue inSelectable)
{
    qt3dsdm::Qt3DSDMInstanceHandle theInstance;
    std::vector<qt3dsdm::Qt3DSDMInstanceHandle> instances;

    switch (inSelectable.getType()) {
    case Q3DStudio::SelectedValueTypes::Instance:
        theInstance = inSelectable.getData<qt3dsdm::Qt3DSDMInstanceHandle>();
        break;
    case Q3DStudio::SelectedValueTypes::MultipleInstances:
        instances = inSelectable.getData<std::vector<qt3dsdm::Qt3DSDMInstanceHandle>>();
        // handling only if we have one selected element.
        if (instances.size() == 1)
            theInstance = instances[0];
        break;
    case Q3DStudio::SelectedValueTypes::Slide: {
        qt3dsdm::Qt3DSDMSlideHandle theSlideHandle =
            inSelectable.getData<Q3DStudio::SSlideInstanceWrapper>().m_Slide;
        // Get the owning component instance
        CClientDataModelBridge *theBridge = GetBridge();
        qt3dsdm::SLong4 theComponentGuid = theBridge->GetComponentGuid(theSlideHandle);
        Q_ASSERT(GuidValid(theComponentGuid));
        theInstance = theBridge->GetInstanceByGUID(theComponentGuid);
        Q_ASSERT(theInstance.Valid());
    } break;
    };

    setItem(theInstance);
}

void ActionView::OnActionAdded(qt3dsdm::CUICDMActionHandle inAction, qt3dsdm::Qt3DSDMSlideHandle inSlide, qt3dsdm::Qt3DSDMInstanceHandle inOwner)
{
    CDoc *theDoc = GetDoc();
    qt3dsdm::CStudioSystem *theStudioSystem = theDoc->GetStudioSystem();

    qt3dsdm::Qt3DSDMSlideHandle theCurrentSlide = theDoc->GetActiveSlide();
    qt3dsdm::Qt3DSDMSlideHandle theMasterSlideOfAction =
        theStudioSystem->GetSlideSystem()->GetMasterSlide(inSlide);
    qt3dsdm::Qt3DSDMSlideHandle theMasterOfCurrentSlide =
        theStudioSystem->GetSlideSystem()->GetMasterSlide(theCurrentSlide);

    if (inOwner == m_itemHandle && // the action is added to current viewed instance
        (theCurrentSlide == inSlide || // and is added to the current viewed slide
         (theMasterSlideOfAction == inSlide
          && theMasterOfCurrentSlide == theMasterSlideOfAction))) // or it is added to the master of
                                                                  // the current viewed slide
    {
        m_actionsModel->addAction(inAction);
// KDAB_TODO       SortActions();
    }
}

void ActionView::OnActionDeleted(qt3dsdm::CUICDMActionHandle inAction, qt3dsdm::Qt3DSDMSlideHandle inSlide, qt3dsdm::Qt3DSDMInstanceHandle inOwner)
{
    Q_UNUSED(inSlide);
    Q_UNUSED(inOwner);
    m_actionsModel->removeAction(inAction);
}

void ActionView::OnActionModified(qt3dsdm::CUICDMActionHandle inAction)
{
    if (GetDoc()->GetStudioSystem()->GetActionCore()->HandleValid(inAction)) {
           m_actionsModel->updateAction(inAction);
           emitActionChanged();
    }
}

void ActionView::OnHandlerArgumentModified(qt3dsdm::CUICDMHandlerArgHandle inHandlerArgument)
{
    emitActionChanged();
}

void ActionView::OnInstancePropertyValueChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance, qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    emitActionChanged();
}

void ActionView::copyAction()
{
    auto theTempAPFile =
        GetDoc()->GetDocumentReader().CopyAction(m_actionsModel->actionAt(m_currentActionIndex),
                                                 GetDoc()->GetActiveSlide());
    CUICFile theFile(theTempAPFile);
    CStudioClipboard::CopyActionToClipboard(theFile);
}

void ActionView::cutAction()
{
    copyAction();
    auto action = m_actionsModel->actionAt(m_currentActionIndex);
    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*GetDoc(), QObject::tr("Cut Action"))->DeleteAction(action);
}

void ActionView::pasteAction()
{
    CUICFile theTempAPFile = CStudioClipboard::GetActionFromClipboard();
    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*GetDoc(), QObject::tr("Paste Action"))
        ->PasteAction(theTempAPFile.GetAbsolutePath(), m_itemHandle);
}

void ActionView::setTriggerObject(const qt3dsdm::SObjectRefType &object)
{
    auto action = m_actionsModel->actionAt(m_currentActionIndex);
    if (!action.Valid())
        return;

    auto core = g_StudioApp.GetCore();
    auto theBridge = GetBridge();

    auto theCmd = new CCmdDataModelActionSetTriggerObject(GetDoc(), action, object);
    const SActionInfo &theActionInfo = GetDoc()->GetStudioSystem()->GetActionCore()->GetActionInfo(action);

    Qt3DSDMInstanceHandle theBaseInstance = theActionInfo.m_Owner;
    Qt3DSDMInstanceHandle theObjectInstance = theBridge->GetInstance(theBaseInstance, object);
    Qt3DSDMInstanceHandle theOldInstance = theBridge->GetInstance(theBaseInstance, theActionInfo.m_TargetObject);
    // old instance and object instance could be the same, for example if user changes the type
    // from Absolute to Path. In this case we don't need to reset handler or event.
    if (theOldInstance != theObjectInstance)
        theCmd->ResetEvent(
            theBridge->GetDefaultEvent(theObjectInstance, theActionInfo.m_Event));

    core->ExecuteCommand(theCmd);
    emitActionChanged();
}

void ActionView::setTargetObject(const qt3dsdm::SObjectRefType &object)
{
    auto action = m_actionsModel->actionAt(m_currentActionIndex);
    if (!action.Valid())
        return;

    auto core = g_StudioApp.GetCore();
    auto doc = GetDoc();
    auto theBridge = GetBridge();

    auto theCmd = new CCmdDataModelActionSetTargetObject(doc, action, object);
    const SActionInfo &theActionInfo = doc->GetStudioSystem()->GetActionCore()->GetActionInfo(action);

    Qt3DSDMInstanceHandle theBaseInstance = theActionInfo.m_Owner;
    Qt3DSDMInstanceHandle theObjectInstance = theBridge->GetInstance(theBaseInstance, object);
    Qt3DSDMInstanceHandle theOldInstance = theBridge->GetInstance(theBaseInstance, theActionInfo.m_TargetObject);
    // old instance and object instance could be the same, for example if user changes the type
    // from Absolute to Path. In this case we don't need to reset handler or event.
    if (theOldInstance != theObjectInstance)
        theCmd->ResetHandler(
            theBridge->GetDefaultHandler(theObjectInstance, theActionInfo.m_Handler));

    core->ExecuteCommand(theCmd);
    emitActionChanged();
}

void ActionView::setEvent(const CUICDMEventHandle &event)
{
    if (!event.Valid())
        return;

    auto doc = GetDoc();
    const auto action = m_actionsModel->actionAt(m_currentActionIndex);
    CCmd *theCmd = new CCmdDataModelActionSetEvent(doc, action,
                                                   doc->GetStudioSystem()
                                                      ->GetActionMetaData()
                                                      ->GetEventInfo(event)
                                                      ->m_Name.wide_str());
    g_StudioApp.GetCore()->ExecuteCommand(theCmd);
}

void ActionView::setHandler(const CUICDMHandlerHandle &handler)
{
    if (!handler.Valid())
        return;

    auto doc = GetDoc();
    const auto action = m_actionsModel->actionAt(m_currentActionIndex);
    wstring handlerName(doc->GetStudioSystem()->GetActionMetaData()->GetHandlerInfo(handler)
                                ->m_Name.wide_str());
    CCmdDataModelActionSetHandler *theCmd =
        new CCmdDataModelActionSetHandler(doc, action, handlerName);
    theCmd->ResetHandler(handlerName); // reset the handler args

    g_StudioApp.GetCore()->ExecuteCommand(theCmd);
}

QVariant ActionView::handlerArgumentValue(int handle) const
{
    qt3dsdm::SValue value;
    GetDoc()->GetStudioSystem()->GetActionCore()->GetHandlerArgumentValue(handle, value);
    return value.toQVariant();
}

void ActionView::updateHandlerArguments()
{
    m_currentPropertyValueHandle = 0;
    m_currentPropertyNameHandle = 0;
    m_handlerArguments.clear();
    const auto doc = GetDoc();
    if (!doc->IsValid())
        return;

    const auto actionInfo = m_actionsModel->actionInfoAt(m_currentActionIndex);
    const auto bridge = GetBridge();
    const auto handlerHandle = bridge->ResolveHandler(actionInfo);
    IActionCore *actionCore = doc->GetStudioSystem()->GetActionCore();

    if (handlerHandle.Valid()) {
        auto newMetaData = doc->GetStudioSystem()->GetActionMetaData();

        for (const auto &argHandle: actionInfo.m_HandlerArgs) {
            const auto &argumentInfo = actionCore->GetHandlerArgumentInfo(argHandle);
            Option<SMetaDataHandlerArgumentInfo> argMetaData(
                newMetaData->FindHandlerArgumentByName(handlerHandle, argumentInfo.m_Name));


            HandlerArgument argument;
            argument.m_handle = argHandle;
            argument.m_type = argMetaData->m_ArgType;
            argument.m_name = QString::fromWCharArray(argumentInfo.m_Name.wide_str());
            argument.m_value = argumentInfo.m_Value.toQVariant();
            argument.m_completeType = argMetaData->m_CompleteType;
            m_handlerArguments.append(QVariant::fromValue(argument));
        }
    }
}

void ActionView::emitActionChanged()
{
    m_actionChangedCompressionTimer.start();
}

void ActionView::setArgumentValue(int handle, const QVariant &value)
{
    if (handle == 0)
        return;

    qt3dsdm::SValue sValue(value);
    qt3dsdm::SValue oldValue;
    GetDoc()->GetStudioSystem()->GetActionCore()->GetHandlerArgumentValue(handle, oldValue);

    if (!Equals(oldValue, sValue)) {
        CCmd *theCmd =
            new CCmdDataModelActionSetArgumentValue(GetDoc(), handle, sValue);
        g_StudioApp.GetCore()->ExecuteCommand(theCmd);
    }

    const auto actionInfo = m_actionsModel->actionInfoAt(m_currentActionIndex);
       if (actionInfo.m_Handler == L"Fire Event") {
           if (value.toInt())
               updateFiredEventFromHandle(value.toInt());
       }
}

CDoc *ActionView::GetDoc()
{
    return g_StudioApp.GetCore()->GetDoc();
}

CClientDataModelBridge *ActionView::GetBridge()
{
    return GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
}

void ActionView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty("_actionView"_L1, this);
    rootContext()->setContextProperty("_resDir"_L1, resourceImageUrl());
    rootContext()->setContextProperty("_tabOrderHandler"_L1, tabOrderHandler());
    qmlRegisterUncreatableType<qt3dsdm::HandlerArgumentType>("Qt3DStudio", 1, 0, "HandlerArgumentType",
                                                          "HandlerArgumentType is an enum container"_L1);
    qmlRegisterUncreatableType<qt3dsdm::DataModelDataType>("Qt3DStudio", 1, 0, "DataModelDataType",
                                                          "DataModelDataType is an enum container"_L1);
    qmlRegisterUncreatableType<qt3dsdm::AdditionalMetaDataType>("Qt3DStudio", 1, 0, "AdditionalMetaDataType",
                                                          "AdditionalMetaDataType is an enum container"_L1);
    qmlRegisterUncreatableType<PropertyInfo>("Qt3DStudio", 1, 0, "PropertyInfo",
                                             "PropertyInfo is anot creatable in QML"_L1);
    qmlRegisterUncreatableType<qt3dsdm::CompleteMetaDataType>("Qt3DStudio", 1, 0, "CompleteMetaDataType",
                                                          "CompleteMetaDataType is an enum container"_L1);
    engine()->addImportPath(qmlImportPath());
    setSource(QUrl("qrc:/Studio/Palettes/Action/ActionView.qml"_L1));
}

QStringList ActionView::slideNames()
{
    std::list<Q3DStudio::CString> outSlideNames;
    QStringList slideNames;
    CClientDataModelBridge *theBridge = GetBridge();
    const auto action = m_actionsModel->actionAt(m_currentActionIndex);

    theBridge->GetSlideNamesOfAction(action, outSlideNames);

    for (auto slideName : outSlideNames) {
        slideNames.append(slideName.toQString());
    }

    return slideNames;
}

int ActionView::slideNameToIndex(const QString &name)
{
    const auto slides = slideNames(); // KDAB_TODO cache it
    return slides.indexOf(name);
}
