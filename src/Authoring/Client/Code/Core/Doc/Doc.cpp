/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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

#include "Doc.h"
#include "Core.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMActionCore.h"
#include "ClientDataModelBridge.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMSignals.h"
#include "IKeyframesManager.h"
#include "FileInputStream.h"
#include "FileOutputStream.h"
#include "BufferedInputStream.h"
#include "BufferedOutputStream.h"
#include "DataModelObjectReferenceHelper.h"
#include "MasterP.h"
#include "Dispatch.h"
#include "Exceptions.h"
#include "StudioClipboard.h"
#include "HotKeys.h"
#include "StudioProjectSettings.h"
#include "StudioPreferences.h"
#include "StudioFullSystem.h"
#include "Qt3DSDMDataCore.h"
#include "CmdDataModelDeleteInstance.h"
#include "PlaybackClock.h"
#include "ColorConversion.h"
#include "IDocumentEditor.h"
#include "IDocumentBufferCache.h"
#include "StudioCoreSystem.h"
#include "Qt3DSDMXML.h"
#include "foundation/IOStreams.h"
#include "IComposerSerializer.h"
#include "Qt3DSFileTools.h"
#include "ProjectSettingsSerializer.h"
#include "CmdBatch.h"
#include "IDirectoryWatchingSystem.h"
#include "StandardExtensions.h"
#include "IDocSceneGraph.h"
#include "Qt3DSTextRenderer.h"
#include "SelectedValueImpl.h"
#include "Qt3DSRenderPathManager.h"
#include "StudioApp.h"
#include "Dialogs.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qtimer.h>
#include <QtGui/qvalidator.h>

const long UIP_VERSION = 6; // current version (latest supported)
const long LAST_SUPPORTED_UIP_VERSION = 1;

IMPLEMENT_OBJECT_COUNTER(CDoc)

struct SDocTransactionCommand : public CCmd
{
    QString m_CommandName;
    std::shared_ptr<qt3dsdm::CmdDataModel> m_DataModelTransactions;
    bool m_DoneOnce;
    CDispatch &m_Dispatch;

    SDocTransactionCommand(std::shared_ptr<qt3dsdm::CmdDataModel> dmt,
                           const QString &cmdName, CDispatch &dispatch)
        : m_CommandName(cmdName)
        , m_DataModelTransactions(dmt)
        , m_DoneOnce(false)
        , m_Dispatch(dispatch)
    {
    }

    ECmdType GetType() override { return CCmd::GENERIC; }

    unsigned long Do() override
    {
        if (!m_DoneOnce) {
            m_DataModelTransactions->RunDoNotifications();
        } else {
            if (m_DataModelTransactions) {
                CDispatchDataModelNotificationScope __dispatchScope(m_Dispatch);
                m_DataModelTransactions->DataModelRedo();
            }
        }
        m_DoneOnce = true;

        return 0;
    }
    unsigned long Undo() override
    {
        if (m_DataModelTransactions) {
            CDispatchDataModelNotificationScope __dispatchScope(m_Dispatch);
            m_DataModelTransactions->DataModelUndo();
        }
        return 0;
    }

    QString ToString() override { return m_CommandName; }
};

CDoc::CDoc(CCore *inCore)
    : m_PlayMode(PLAYMODE_STOP)
    , m_CurrentViewTime(0)
    , m_ActiveLayer(0)
    , m_Core(nullptr)
    , m_IsModified(false)
    , m_IsTemporary(true)
    , m_DocumentPath("")
    , m_DataManager(nullptr)
    , m_KeyframesManager(nullptr)
    , m_DataModelObjectRefHelper(nullptr)
    , m_AssetGraph(TAssetGraphPtr())
    , m_TransactionDepth(0)
    , m_nudging(false)
    , m_WindowHandle(nullptr)
{
    ADDTO_OBJECT_COUNTER(CDoc)

    m_DataModelObjectRefHelper = new CObjectReferenceHelper(this);

    m_PlaybackClock = new CPlaybackClock(this);

    m_Core = inCore;
}

CDoc::~CDoc()
{
    REMOVEFROM_OBJECT_COUNTER(CDoc)

    m_SelectedObject = Q3DStudio::SSelectedValue(); // because on shutdown, the selected object ptr
                                                    // cannot be assumed to be valid.
    CleanupData();
    ClosePresentation();

    if (m_SceneInstance.Valid()) {
        m_AssetGraph->RemoveNode(m_SceneInstance);
        m_SceneInstance = 0;
    }

    // Destroy Asset Graph
    m_AssetGraph = TAssetGraphPtr();

    delete m_DataModelObjectRefHelper;
    delete m_PlaybackClock;
}

/**
 * Set the state of this document as being modified or not.
 * This should affect the closing/saving of the core.
 */
void CDoc::SetModifiedFlag(bool inIsModified)
{
    m_IsModified = inIsModified;
}

void CDoc::SetKeyframesManager(IKeyframesManager *inManager)
{
    m_KeyframesManager = inManager;
}

qt3dsdm::IAnimationCore *CDoc::GetAnimationCore() const
{
    return GetStudioSystem()->GetAnimationCore();
}

qt3dsdm::IPropertySystem *CDoc::GetPropertySystem() const
{
    return GetStudioSystem()->GetPropertySystem();
}

void CDoc::SetInstancePropertyValue(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                    const std::wstring &inPropertyName,
                                    const qt3dsdm::SValue &inValue)
{
    qt3dsdm::IPropertySystem *thePropertySystem = GetStudioSystem()->GetPropertySystem();
    qt3dsdm::Qt3DSDMPropertyHandle theProperty =
            thePropertySystem->GetAggregateInstancePropertyByName(inInstance, inPropertyName.c_str());
    if (theProperty.Valid())
        thePropertySystem->SetInstancePropertyValue(inInstance, theProperty, inValue);
}

// Utility that either adds or removes control string for the specified property. Does not check if
// the property already has a controller in the controlledPropStr. Returns true on success.
bool ModifyControlStrForProperty(Q3DStudio::CString &controlledPropStr,
                                 const Q3DStudio::CString &propName,
                                 bool controlled = false,
                                 const Q3DStudio::CString diName = Q3DStudio::CString())
{
    Q3DStudio::CString cpStr = controlledPropStr;

    if (!controlled) {
        // get the length of to-be-deleted controller - property string and
        // delete it from the overall controlledproperty
        long posProp = cpStr.find(" " + propName);

        if (posProp == Q3DStudio::CString::ENDOFSTRING)
            return false;

        long posCtrlr = cpStr.substr(0, posProp - 1).ReverseFind(" ");

        // this is the first controller - property pair in controlledproperty
        if (posCtrlr < 0)
            posCtrlr = 0;

        long deletableStrLen = (posProp + propName.Length()) - posCtrlr + 1;

        cpStr.Delete(posCtrlr, deletableStrLen);

        // clean up the string as we might have extra whitespaces
        cpStr.Replace("  ", " ");

        cpStr.TrimLeft();
        cpStr.TrimRight();
    } else {
        // Insert delimiter if we already have an existing string.
        if (cpStr.size())
            cpStr.append(" ");
        cpStr.append("$" + diName);
        cpStr.append(" ");
        cpStr.append(propName);
    }
    controlledPropStr = cpStr;

    return true;
}

void CDoc::RemoveDatainputBindings(
        const QMultiMap<QString, QPair<qt3dsdm::Qt3DSDMInstanceHandle,
                                       qt3dsdm::Qt3DSDMPropertyHandle>> *map)
{
    // We might be altering several attributes, aggregate them together
    m_SceneEditor->BeginAggregateOperation();
    const auto uniqueKeys = map->uniqueKeys();
    for (const auto &name : uniqueKeys) {
        const auto values = map->values(name);
        for (const auto &pair : values) {
            SetInstancePropertyControlled(pair.first, Q3DStudio::CString(),
                                          pair.second, Q3DStudio::CString::fromQString(name),
                                          false, true);
        }
    }

    m_SceneEditor->EndAggregateOperation();
    // if SetInstancePropertyControlled fails for some reason, we might not have open transaction.
    if (isTransactionOpened())
        closeTransaction();
}

// Set a property in an instance to be controlled by datainput.
// If 'batch' is set to true, we group subsequent transactions together.
// Caller is responsible for calling CloseTransaction() after all transactions
// are finished
void CDoc::SetInstancePropertyControlled(
        qt3dsdm::Qt3DSDMInstanceHandle instance, Q3DStudio::CString instancepath,
        qt3dsdm::Qt3DSDMPropertyHandle propName, Q3DStudio::CString newCtrl,
        bool controlled, bool batch)
{
    Q_UNUSED(instancepath)

    qt3dsdm::IPropertySystem *thePropertySystem = GetStudioSystem()->GetPropertySystem();
    // We might have invalid controller (not found from the global list of datainputs)
    bool newCtrlrValid = false;
    qt3dsdm::SValue controlledProperty;
    Q3DStudio::CString currCtrldPropsStr = Q3DStudio::CString();
    qt3dsdm::SValue currentCtrldProps;
    qt3dsdm::Qt3DSDMMetaDataPropertyHandle metadataHandle;
    qt3dsdm::Option<qt3dsdm::SMetaDataPropertyInfo> metadata;

    // Get current controller - property string for this element
    qt3dsdm::Qt3DSDMPropertyHandle ctrldElemPropHandle
            = thePropertySystem->GetAggregateInstancePropertyByName(instance,
                                                                    L"controlledproperty");

    // Get the controlledproperty tag for the target element
    if (ctrldElemPropHandle.Valid()) {
        thePropertySystem->GetInstancePropertyValue(
                    instance, ctrldElemPropHandle, currentCtrldProps);
        if (!currentCtrldProps.empty())
            currCtrldPropsStr = qt3dsdm::get<qt3dsdm::TDataStrPtr>(currentCtrldProps)->GetData();
    }

    // Get the name of controlled property if valid
    if (propName.Valid()) {
        metadataHandle
                = GetStudioSystem()->GetActionMetaData()->GetMetaDataProperty(instance, propName);
        metadata = GetStudioSystem()->GetActionMetaData()->GetMetaDataPropertyInfo(metadataHandle);
    }

    // Check for new controller validity
    if (g_StudioApp.m_dataInputDialogItems.contains(newCtrl.toQString()))
        newCtrlrValid = true;
    else if (controlled)
        return; // trying to set control on but new controller is not valid, abort

    // @slide and @timeline are not valid named properties thus incoming propName handle
    // will be invalid.
    // Check if the caller wants to remove control from either pseudo-property
    // and handle them with a special case. Datamodel notifications will cause
    // slide and timeline views to update datainput control status, no need for additional
    // signaling.
    // Note that slide and timeline controls are never set using this interface,
    // as @slide and @timeline are not using actual properties; we implement control
    // removal here just for the purposes of enabling recursive removal of control throughout
    // the graph
    if (!propName.Valid() && !controlled) {
        if (currCtrldPropsStr.find(newCtrl + " @slide") != Q3DStudio::CString::ENDOFSTRING) {
            ModifyControlStrForProperty(currCtrldPropsStr, Q3DStudio::CString("@slide"),
                                        false, newCtrl);
        }
        if (currCtrldPropsStr.find(newCtrl + " @timeline") != Q3DStudio::CString::ENDOFSTRING) {
            ModifyControlStrForProperty(currCtrldPropsStr, Q3DStudio::CString("@timeline"),
                                        false, newCtrl);
        }
        // in any case, write out the controlledproperty string even if unmodified
        controlledProperty = std::make_shared<qt3dsdm::CDataStr>(currCtrldPropsStr);
    } else {
        // We are going to set or change the controller for this property. Remove the
        // old controller - property pair from the controlledproperty string first if it exists.
        if (currCtrldPropsStr.find(metadata->m_Name.c_str()) != Q3DStudio::CString::ENDOFSTRING)
            ModifyControlStrForProperty(currCtrldPropsStr, metadata->m_Name.c_str(), false);

        // Modify the controlledproperty tag for the target element
        if (controlled) {
            ModifyControlStrForProperty(currCtrldPropsStr, metadata->m_Name.c_str(), true, newCtrl);
            controlledProperty = std::make_shared<qt3dsdm::CDataStr>(currCtrldPropsStr);
        } else if (!controlled) {
            if (currCtrldPropsStr.size()) {
                // Current controller - property string was already removed, just store
                // what is left.
                controlledProperty = std::make_shared<qt3dsdm::CDataStr>(currCtrldPropsStr);
            } else {
                // All control is off. We cannot remove the entire property because undoing
                // datainput control changes after removal would cause a crash in the
                // undo/redo system.
                // Consequently we cannot remove controlledproperty completely from the UIP file
                // either so store empty string instead.
                controlledProperty = std::make_shared<qt3dsdm::CDataStr>(Q3DStudio::CString());
            }
        }
    }

    // Set the controlledproperty string in the controlled element
    // Use same transaction if "batch" is true
    if (!batch) {
        Q3DStudio::ScopedDocumentEditor(*this, QObject::tr("Set controlled"), __FILE__, __LINE__)
                ->SetInstancePropertyValue(instance, ctrldElemPropHandle, controlledProperty);
    } else {
        if (!isTransactionOpened())
            OpenTransaction(QObject::tr("Set multiple controlled"), __FILE__, __LINE__);
        SetInstancePropertyValue(instance, L"controlledproperty", controlledProperty);
    }
}

Q3DStudio::IDocumentBufferCache &CDoc::GetBufferCache()
{
    if (!m_DocumentBufferCache)
        m_DocumentBufferCache = Q3DStudio::IDocumentBufferCache::CreateBufferCache(*this);
    return *m_DocumentBufferCache.get();
}

Q3DStudio::IDocumentReader &CDoc::GetDocumentReader()
{
    if (!m_SceneEditor)
        m_SceneEditor = Q3DStudio::IInternalDocumentEditor::CreateEditor(*this);
    return *m_SceneEditor;
}

Q3DStudio::IDocumentEditor &CDoc::OpenTransaction(const QString &inCmdName, const char *inFile,
                                                  int inLine)
{
    ++m_TransactionDepth;
    if (m_TransactionDepth == 1) {
        assert(!m_OpenTransaction);
        m_OpenTransaction = std::make_shared<qt3dsdm::CmdDataModel>(*this);
        m_OpenTransaction->SetName(inCmdName);
        m_OpenTransaction->SetConsumer();
        m_Core->SetCommandStackModifier(this);
        qCInfo(qt3ds::TRACE_INFO) << inFile << "(" << inLine << "): Transaction opened: "
                                  << inCmdName;
        m_OpenTransaction->m_File = inFile;
        m_OpenTransaction->m_Line = inLine;
        CCmdStack *theCommandStack = m_Core->GetCmdStack();
        if (theCommandStack)
            theCommandStack->EmptyRedoStack();
    } else
        qCInfo(qt3ds::TRACE_INFO) << inFile << "(" << inLine << "): Open Transaction: "
                                  << inCmdName;

    if (!m_SceneEditor)
        m_SceneEditor = Q3DStudio::IInternalDocumentEditor::CreateEditor(*this);

    return *m_SceneEditor;
}

Q3DStudio::IDocumentEditor &CDoc::maybeOpenTransaction(const QString &cmdName,
                                                       const char *inFile, int inLine)
{
    if (!m_OpenTransaction)
        return OpenTransaction(cmdName, inFile, inLine);
    return *m_SceneEditor;
}

bool CDoc::isTransactionOpened() const
{
    return m_OpenTransaction != nullptr;
}

void CDoc::rollbackTransaction()
{
    if (m_OpenTransaction)
        m_OpenTransaction->DataModelRollback();
}

void CDoc::closeTransaction()
{
    if (m_TransactionDepth) {
        --m_TransactionDepth;
        if (m_TransactionDepth == 0) {
            assert(m_OpenTransaction);
            forceCloseTransaction();
        }
    }
}

void CDoc::forceCloseTransaction()
{
    if (m_OpenTransaction) {
        qCInfo(qt3ds::TRACE_INFO) << "Closing transaction";
        // Ensure hasTransaction will return false right at this second.
        std::shared_ptr<qt3dsdm::CmdDataModel> theTransaction(m_OpenTransaction);
        m_OpenTransaction.reset();

        m_Core->SetCommandStackModifier(nullptr);
        // Release the consumer without running notifications because our command will run
        // the notifications when it first gets executed.
        theTransaction->ReleaseConsumer(false);
        if (theTransaction->HasTransactions()) {
            SDocTransactionCommand *newCommand = new SDocTransactionCommand(
                        theTransaction, theTransaction->GetName(), *m_Core->GetDispatch());
            // Execute the command synchronously.  If you are getting crashes due to UI refreshes
            // then
            // you need to run your entire change system in a postmessage of some sort.
            m_Core->ExecuteCommand(newCommand);
        }
        qCInfo(qt3ds::TRACE_INFO) << theTransaction->m_File.GetCharStar() << "("
                                  << theTransaction->m_Line << "): Transaction closed";
    }
    m_TransactionDepth = 0;
}

bool CDoc::canUndo()
{
    return m_OpenTransaction != nullptr;
}

bool CDoc::preUndo()
{
    if (m_OpenTransaction && m_OpenTransaction->HasTransactions()) {
        qCInfo(qt3ds::TRACE_INFO) << "PreUndo begin";
        // In this case we want the command to absolutely immediately commit; we don't want it
        // to wait until a further post message else the previous command is the one that will get
        // undone.
        forceCloseTransaction();
        qCInfo(qt3ds::TRACE_INFO) << "PreUndo end";
        return true;
    }
    return false;
}


// Is document modified since last save?
bool CDoc::isModified() const
{
    return m_IsModified;
}

bool CDoc::isValid() const
{
    return !m_DocumentPath.isEmpty();
}

/**
 * Get the Asset from inSelectedItem, if exists
 */
qt3dsdm::Qt3DSDMInstanceHandle
CDoc::GetInstanceFromSelectable(Q3DStudio::SSelectedValue inSelectedItem) const
{
    if (inSelectedItem.getType() == Q3DStudio::SelectedValueTypes::Instance) {
        // This is DataModel asset. Find corresponding CAsset if there is any
        return inSelectedItem.getData<qt3dsdm::Qt3DSDMInstanceHandle>();
    } else if (inSelectedItem.getType() == Q3DStudio::SelectedValueTypes::MultipleInstances) {
        const qt3dsdm::TInstanceHandleList &theData =
                *inSelectedItem.getDataPtr<qt3dsdm::TInstanceHandleList>();
        if (theData.empty() == false)
            return theData[0];
    }

    return 0;
}

int CDoc::getSelectedInstancesCount() const
{
    return int(m_SelectedValue.GetSelectedInstances().size());
}

qt3dsdm::Qt3DSDMInstanceHandle CDoc::GetSelectedInstance() const
{
    return GetInstanceFromSelectable(m_SelectedObject);
}

CCore *CDoc::GetCore() const
{
    return m_Core;
}

/**
 *	Calls NotifyActiveSlideChanged( qt3dsdm::Qt3DSDMSlideHandle inNewActiveSlide, bool
 *inForceRefresh )
 *	Could not make this on optional param because someone is calling from CGenericCmd that
 *	seems to only allow 1 param.
 */
void CDoc::NotifyActiveSlideChanged(qt3dsdm::Qt3DSDMSlideHandle inNewActiveSlide)
{
    NotifyActiveSlideChanged(inNewActiveSlide, false);
}

void CDoc::SetActiveSlideChange(qt3dsdm::Qt3DSDMSlideHandle inNewActiveSlide)
{
    if (!inNewActiveSlide.Valid())
        inNewActiveSlide = GetActiveSlide();

    if (m_ActiveSlide != inNewActiveSlide) {
        // Ensure that events are batched if they aren't already
        CDispatchDataModelNotificationScope __dispatchScope(*m_Core->GetDispatch());
        ASSERT(inNewActiveSlide.Valid());

        m_ActiveSlide = inNewActiveSlide;

        DeselectAllItems(false);
        m_ActiveLayer = 0;

        qt3dsdm::ISlideSystem *theSlideSystem = m_StudioSystem->GetSlideSystem();
        qt3dsdm::Qt3DSDMSlideHandle theMasterSlide = theSlideSystem->GetMasterSlide(inNewActiveSlide);

        long theNewIndex = theSlideSystem->GetSlideIndex(inNewActiveSlide);
        theSlideSystem->SetActiveSlide(theMasterSlide, theNewIndex);

        m_PlaybackClock->Reset();

        // Disable the active layer and set it lazily.

        // Set active slide to DataModel
    }
}

/**
 * Sets the active slide for the document.
 * The active slide is used for when something other than the Scene is
 * acting as the root of the presentation, as when delving into a component
 * or such.
 *
 * When changing time contexts (master slides):
 * Go to the last-edited/currently displayed slide of that context (default: Slide1)
 * Select whatever was last selected in that context (default: the context element itself)
 *
 * When clicking on a slide:
 * If the clicked-on slide is the current slide, select and inspect the time context itself
 * If the clicked-on slide is different:
 *   If the was-selected item was non-master, select and inspect the time context itself
 *   If the was-selected item was master, continue to leave that item selected and inspected.
 *
 * @param inNewActiveSlide the new slide handle to be the active root of the doc.
 * @param inForceRefresh forces a refresh of the active time context no matter what, if the
 * inNewRoot is NULL and this is true, then it will refresh the active time context
 * @param inIgnoreLastDisplayTime false to always restore the last displayed time when this context
 * was active. E.g. when this would be true is when we are playing through, time should always start
 * from 0 then.
 */
void CDoc::NotifyActiveSlideChanged(qt3dsdm::Qt3DSDMSlideHandle inNewActiveSlide, bool inForceRefresh,
                                    bool inIgnoreLastDisplayTime /*= false */)
{
    using namespace qt3dsdm;

    Qt3DSDMSlideHandle theLastActiveSlide = m_ActiveSlide;

    // Record the last selected object in that slide
    ISlideSystem *theSlideSystem = GetStudioSystem()->GetSlideSystem();
    theSlideSystem->SetSlideSelectedInstance(theLastActiveSlide, GetSelectedInstance());
    Qt3DSDMInstanceHandle theNextSelectedInstance = GetSelectedInstance();

    // If we are forcing a refresh. and the incoming root is not valid, use the existing active time
    // context
    if (!inNewActiveSlide.Valid())
        inNewActiveSlide = GetActiveSlide();

    // We have a new root context
    if (inForceRefresh || theLastActiveSlide != inNewActiveSlide) {
        ASSERT(inNewActiveSlide.Valid());

        // Notify the system of a slide change. Do it.
        if (theLastActiveSlide != inNewActiveSlide) {
            SetActiveSlideChange(inNewActiveSlide);
            if (inIgnoreLastDisplayTime)
                m_StudioSystem->GetSlideSystem()->SetComponentSeconds(
                            m_StudioSystem->GetSlideSystem()->GetMasterSlide(inNewActiveSlide), 0);
        }

        Qt3DSDMSlideHandle theLastMasterSlide =
                m_StudioSystem->GetSlideSystem()->GetMasterSlide(theLastActiveSlide);
        Qt3DSDMSlideHandle theNewMasterSlide =
                m_StudioSystem->GetSlideSystem()->GetMasterSlide(inNewActiveSlide);

        int theIndex = m_StudioSystem->GetSlideSystem()->GetSlideIndex(inNewActiveSlide);
        m_StudioSystem->GetFullSystem()->GetSignalSender()->SendActiveSlide(
                    theNewMasterSlide, theIndex, inNewActiveSlide);

        // If the slide *did* change then we have some somewhat complex logic
        // If we didn't change time contexts...
        if (theLastMasterSlide == theNewMasterSlide) {
            // If we are reselecting the current slide, select the component.
            if (theLastActiveSlide == inNewActiveSlide)
                theNextSelectedInstance = 0;

            if (theNextSelectedInstance.Valid()) {
                // Discard selected instance if nonmaster
                if (theNewMasterSlide
                        != GetDocumentReader().GetAssociatedSlide(theNextSelectedInstance))
                    theNextSelectedInstance = 0;
            }
        }
        // If we did change time contexts
        else {
            theNextSelectedInstance = theSlideSystem->GetSlideSelectedInstance(inNewActiveSlide);
            // If nothing was selected, then select the component.
        }

        if (!theNextSelectedInstance.Valid())
            theNextSelectedInstance = GetDocumentReader().GetComponentForSlide(inNewActiveSlide);

        if (theNextSelectedInstance != GetSelectedInstance())
            SelectDataModelObject(theNextSelectedInstance);
    }
}

/**
 * @returns the first enabled layer in the active time context
 */
qt3dsdm::Qt3DSDMInstanceHandle CDoc::getFirstSelectableLayer()
{
    CClientDataModelBridge *theBridge = m_StudioSystem->GetClientDataModelBridge();

    Q3DStudio::CGraphIterator theLayers;
    GetAssetChildrenInSlide(this, m_SceneInstance,
                            theBridge->GetComponentActiveSlide(m_SceneInstance), theLayers,
                            OBJTYPE_LAYER);

    for (; !theLayers.IsDone(); ++theLayers) {
        if (!theBridge->IsLockedAtAll(theLayers.GetCurrent())
                && m_StudioSystem->IsInstance(theLayers.GetCurrent())) {
            return theLayers.GetCurrent();
        }
    }

    return 0;
}

// returns all instances that has a 'variants' property
QVector<int> CDoc::getVariantInstances(int instance)
{
    auto bridge = m_StudioSystem->GetClientDataModelBridge();
    if (!instance)
        instance = int(m_SceneInstance);
    Q3DStudio::CGraphIterator it;
    GetAssetChildren(this, instance, it);

    QVector<int> list;

    for (; !it.IsDone(); ++it) {
        if (m_StudioSystem->IsInstance(it.GetCurrent())) {
            if (bridge->GetObjectType(it.GetCurrent()) & OBJTYPE_IS_VARIANT)
                list.append(it.GetCurrent());

            list.append(getVariantInstances(it.GetCurrent()));
        }
    }

    return list;
}

qt3dsdm::Qt3DSDMInstanceHandle CDoc::GetActiveRootInstance()
{
    if (m_ActiveSlide.Valid()) {
        return m_StudioSystem->GetClientDataModelBridge()->GetOwningComponentInstance(
                    m_ActiveSlide);
    }

    return m_SceneInstance;
}

/**
 *	Returns the current active slide
 *	The timeline is always displaying the active root.
 */
qt3dsdm::Qt3DSDMSlideHandle CDoc::GetActiveSlide()
{
    if (m_ActiveSlide.Valid())
        return m_ActiveSlide;

    if (m_SceneInstance.Valid())
        return m_StudioSystem->GetClientDataModelBridge()->GetComponentSlide(m_SceneInstance, 0);

    return 0;
}

/**
 * Get the currently active layer.
 * The active layer is the layer of the last selected asset or the first enabled layer after
 * the slide is switched and is held onto mainly for determining which layer
 * to drop an object onto when dropping on the scene.
 */
qt3dsdm::Qt3DSDMInstanceHandle CDoc::GetActiveLayer()
{
    if (!m_ActiveLayer.Valid())
        m_ActiveLayer = getFirstSelectableLayer();

    return m_ActiveLayer;
}

/**
 * Set the currently active layer.
 * The active layer is the layer of the last selected asset or the first enabled layer after
 * the slide is switched and is held onto mainly for determining which layer
 * to drop an object onto when dropping on the scene.
 */
void CDoc::SetActiveLayer(qt3dsdm::Qt3DSDMInstanceHandle inLayerInstance)
{
    m_ActiveLayer = inLayerInstance;
}

/**
 * Deselects all the items and keyframes that are currently selected.
 */
void CDoc::DeselectAllItems(bool inSendEvent)
{
    if (inSendEvent)
        NotifySelectionChanged();
    else
        m_unnotifiedSelectionChange = SetSelection();

    // Remove selection on keyframes.
    DeselectAllKeyframes();
}

/**
 * Cuts the selected object
 */
void CDoc::CutSelectedObject()
{
    CutObject(m_SelectedValue.GetSelectedInstances());
}

void CDoc::CutObject(qt3dsdm::TInstanceHandleList inInstances)
{
    if (CHotKeys::isFocusOnControlThatWantsKeys())
        return;
    if (inInstances.empty())
        return;
    bool theContinueCutFlag = true;
    for (size_t idx = 0, end = inInstances.size(); idx < end && theContinueCutFlag; ++idx) {
        qt3dsdm::Qt3DSDMInstanceHandle inInstance(inInstances[idx]);
        if (!GetDocumentReader().IsInstance(inInstance))
            return;

        // Build the list of targets
        Q3DStudio::CString theListOfTargets;
        GetActionDependencies(inInstance, theListOfTargets);

        if (!theListOfTargets.IsEmpty()) {
            if (m_DeletingReferencedObjectHandler)
                m_DeletingReferencedObjectHandler->DisplayMessageBox(theListOfTargets.toQString());
            // theContinueCutFlag = false;
        }
    }
    using namespace Q3DStudio;

    if (theContinueCutFlag) {
        CFilePath thePath(GetDocumentReader().CopySceneGraphObjects(inInstances));
        Qt3DSFile theFile(thePath);
        CStudioClipboard::CopyObjectToClipboard(
                    theFile, false, false,
                    m_StudioSystem->GetClientDataModelBridge()->GetObjectType(inInstances[0]));
        SCOPED_DOCUMENT_EDITOR(*this, QObject::tr("Cut Object"))->DeleteInstances(inInstances);
    }
}

void CDoc::CopyObject(qt3dsdm::TInstanceHandleList inInstances)
{
    if (CHotKeys::isFocusOnControlThatWantsKeys())
        return;
    if (inInstances.empty())
        return;
    using namespace Q3DStudio;
    CFilePath thePath(GetDocumentReader().CopySceneGraphObjects(inInstances));
    Qt3DSFile theFile(thePath);
    CStudioClipboard::CopyObjectToClipboard(
                theFile, false, false,
                m_StudioSystem->GetClientDataModelBridge()->GetObjectType(inInstances[0]));
}

void CDoc::PasteObject(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    using namespace Q3DStudio;
    if (inInstance.Valid()) {
        qt3dsdm::Qt3DSDMInstanceHandle theInstance(inInstance);
        qint64 dummy = 0;
        Qt3DSFile theTempAPFile = CStudioClipboard::GetObjectFromClipboard(false, dummy);
        SCOPED_DOCUMENT_EDITOR(*this, QObject::tr("Paste Object"))
                ->PasteSceneGraphObject(theTempAPFile.GetAbsolutePath(), theInstance, true,
                                        DocumentEditorInsertType::LastChild, CPt());
    }
}

void CDoc::PasteObjectMaster(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    using namespace Q3DStudio;
    if (inInstance.Valid()) {
        qt3dsdm::Qt3DSDMInstanceHandle theInstance(inInstance);
        qint64 dummy = 0;
        Qt3DSFile theTempAPFile = CStudioClipboard::GetObjectFromClipboard(false, dummy);
        SCOPED_DOCUMENT_EDITOR(*this, QObject::tr("Paste Object"))
                ->PasteSceneGraphObjectMaster(theTempAPFile.GetAbsolutePath(), theInstance, true,
                                              DocumentEditorInsertType::LastChild, CPt());
    }
}

/**
 * Deletes the selected object
 */
void CDoc::deleteSelectedObject(bool slide)
{
    if (!slide) {
        qt3dsdm::TInstanceHandleList theSelectedHandles = m_SelectedValue.GetSelectedInstances();
        if (!theSelectedHandles.empty())
            DeleteObject(theSelectedHandles);
    } else {
        // Check if the slide is the last one or the master, and prevent deleting if it is
        qt3dsdm::Qt3DSDMSlideHandle slideHandle = GetActiveSlide();
        qt3dsdm::ISlideSystem *slideSys = m_StudioSystem->GetSlideSystem();
        qt3dsdm::Qt3DSDMSlideHandle masterSlideHandle = slideSys->GetMasterSlide(slideHandle);
        size_t slideCount = slideSys->GetSlideCount(masterSlideHandle);
        if (slideHandle != masterSlideHandle && slideCount > 2) {
            Q3DStudio::SCOPED_DOCUMENT_EDITOR(*this, QObject::tr("Delete Slide"))->DeleteSlide(
                        slideHandle);
        }
    }
}

void CDoc::DeleteObject(const qt3dsdm::TInstanceHandleList &inInstances)
{
    // We don't deselect all items because that will happen automagically because we are listening
    // to the events coming out of the system around delete.
    DeselectAllKeyframes();

    CClientDataModelBridge *theClientBridge = GetStudioSystem()->GetClientDataModelBridge();
    qt3dsdm::TInstanceHandleList deletableInstances;
    for (size_t idx = 0, end = inInstances.size(); idx < end; ++idx) {

        // find all the custom items created for it and remove it all
        if (theClientBridge->CanDelete(inInstances[idx])) {
            Q3DStudio::CString theListOfTargets;
            GetActionDependencies(inInstances[idx], theListOfTargets);

            if (!theListOfTargets.IsEmpty() && m_DeletingReferencedObjectHandler)
                m_DeletingReferencedObjectHandler->DisplayMessageBox(theListOfTargets.toQString());

            deletableInstances.push_back(inInstances[idx]);
        }
    }

    if (!deletableInstances.empty()) {
        NotifySelectionChanged();
        Q3DStudio::SCOPED_DOCUMENT_EDITOR(*this, QObject::tr("Delete"))
                                         ->DeleteInstances(deletableInstances);
    }
}

/**
 *	Checks and return a string of objects that have actions referencing inAsset.
 *	@inAsset				The asset to check for dependencies.
 *	@ioActionDependencies	String representation of objects that have actions referencing it.
 */
void CDoc::GetActionDependencies(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                 Q3DStudio::CString &ioActionDependencies)
{
    // Step 1 : Get all actions affecting myself and all my descendents
    qt3dsdm::TActionHandleList theAffectedActions;
    GetActionDependencies(inInstance, theAffectedActions);

    // Set of unique owner names
    typedef std::set<Q3DStudio::CString> TActionOwners;
    TActionOwners theActionOwners;

    qt3dsdm::IActionCore *theActionCore = m_StudioSystem->GetActionCore();
    qt3dsdm::TActionHandleList::iterator thePos = theAffectedActions.begin();
    qt3dsdm::TActionHandleList::iterator theEnd = theAffectedActions.end();

    // Filter away those actions that is owned by myself or my descendants, which anyway the
    // actions are going away too.
    CClientDataModelBridge *theBridge = m_StudioSystem->GetClientDataModelBridge();
    for (; thePos != theEnd; ++thePos) {
        const qt3dsdm::SActionInfo &theActionInfo = theActionCore->GetActionInfo(*thePos);
        qt3dsdm::Qt3DSDMInstanceHandle theOwnerHandle = theActionInfo.m_Owner;

        if (theOwnerHandle != inInstance && theOwnerHandle.Valid()
                && !IsAscendant(theOwnerHandle, inInstance, m_AssetGraph))
            theActionOwners.insert(theBridge->GetName(theOwnerHandle));
    }

    // Iterate the set and form the output string
    TActionOwners::iterator theActionOwnersPos = theActionOwners.begin();
    TActionOwners::iterator theActionOwnersEnd = theActionOwners.end();
    for (; theActionOwnersPos != theActionOwnersEnd; ++theActionOwnersPos) {
        if (ioActionDependencies.IsEmpty())
            ioActionDependencies += *theActionOwnersPos;
        else
            ioActionDependencies += ", " + *theActionOwnersPos;
    }
}

/**
 *	Checks and return a string of objects that have actions referencing inAsset.
 *	@inAsset				The asset to check for dependencies.
 *	@ioActionList			List of actions.
 */
void CDoc::GetActionDependencies(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                 qt3dsdm::TActionHandleList &ioActionList)
{
    CClientDataModelBridge *theBridge = m_StudioSystem->GetClientDataModelBridge();
    theBridge->GetReferencedActions(
                inInstance, qt3dsdm::REFERENCED_AS_TRIGGER | qt3dsdm::REFERENCED_AS_TARGET, ioActionList);

    Q3DStudio::CGraphIterator theChildren;
    GetAssetChildren(this, inInstance, theChildren);
    for (; !theChildren.IsDone(); ++theChildren) {
        GetActionDependencies(theChildren.GetCurrent(), ioActionList);
    }
}

/**
 * Deletes the selected items, this is the primary handler of the delete key.
 */
void CDoc::DeleteSelectedItems(bool slide)
{
    // If there are keyframes selected, delete them, else delete whatever asset is selected
    if (!deleteSelectedKeyframes())
        deleteSelectedObject(slide);
}

bool CDoc::deleteSelectedKeyframes()
{
    if (m_KeyframesManager)
        return m_KeyframesManager->RemoveKeyframes(false);

    return false;
}

/**
 * Sets keyframes on all the changed properties of the selected object.
 */
void CDoc::SetChangedKeyframes()
{
    if (m_KeyframesManager)
        m_KeyframesManager->SetChangedKeyframes();
}

bool CDoc::SetSelection(Q3DStudio::SSelectedValue inNewSelection)
{
    if (inNewSelection != m_SelectedObject) {
        qt3dsdm::Qt3DSDMInstanceHandle theNewSelectedInstance =
                GetInstanceFromSelectable(inNewSelection);

        if (theNewSelectedInstance.Valid()) {
            // Do not allow selection if asset is locked.
            if (m_StudioSystem->GetClientDataModelBridge()->IsLockedAtAll(theNewSelectedInstance))
                return false; // bail!
        }
        m_SelectedObject = inNewSelection;

        CClientDataModelBridge *theBridge = m_StudioSystem->GetClientDataModelBridge();

        // Handle CAsset's specific code
        if (theNewSelectedInstance.Valid()) {
            // Check if we do not select a SlideInspectable. Refer to
            // CStudioApp::GetInspectableFromSelectable.
            qt3dsdm::Qt3DSDMSlideHandle theCurrentActiveSlide = GetActiveSlide();
            if (theNewSelectedInstance
                    != theBridge->GetOwningComponentInstance(theCurrentActiveSlide)) {
                // If the newly selected object is in the scene then make the layer it belongs to
                // the 'active' layer.
                qt3dsdm::Qt3DSDMInstanceHandle theActiveLayer = 0;

                qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance = GetSelectedInstance();
                theActiveLayer = theBridge->GetResidingLayer(theSelectedInstance);

                if (theActiveLayer.Valid() && theBridge->IsLockedAtAll(theActiveLayer))
                    theActiveLayer = 0;

                if (!theActiveLayer.Valid())
                    theActiveLayer = getFirstSelectableLayer();

                if (theActiveLayer.Valid() && theBridge->IsLockedAtAll(theActiveLayer))
                    theActiveLayer = 0;

                SetActiveLayer(theActiveLayer);
            }
        }

        return true;
    }

    return false;
}

void CDoc::NotifySelectionChanged(Q3DStudio::SSelectedValue inNewSelection)
{
    m_SelectedValue = inNewSelection;
    if (SetSelection(inNewSelection))
        m_Core->GetDispatch()->FireSelectionChange(inNewSelection);
    else if (m_unnotifiedSelectionChange)
        m_Core->GetDispatch()->FireSelectionChange(m_SelectedObject);
    m_unnotifiedSelectionChange = false;
}

template <typename TDataType>
struct SReferenceTransaction : public qt3dsdm::ITransaction
{
    TDataType &m_TargetData;
    TDataType m_OldData;
    TDataType m_NewData;
    SReferenceTransaction(const char *inFile, int inLine, TDataType &inTarget, TDataType inOldData,
                          TDataType inNewData)
        : ITransaction(inFile, inLine)
        , m_TargetData(inTarget)
        , m_OldData(inOldData)
        , m_NewData(inNewData)
    {
    }
    void Do() override { m_TargetData = m_NewData; }
    void Undo() override { m_TargetData = m_OldData; }
};

void CDoc::SetActiveSlideWithTransaction(qt3dsdm::Qt3DSDMSlideHandle inNewActiveSlide)
{
    using namespace qt3dsdm;
    Qt3DSDMSlideHandle theActiveSlide = m_ActiveSlide;
    m_ActiveSlide = inNewActiveSlide;
    TTransactionConsumerPtr theConsumer = m_StudioSystem->GetFullSystem()->GetConsumer();
    if (theConsumer) {
        theConsumer->OnTransaction(std::make_shared<SReferenceTransaction<Qt3DSDMSlideHandle>>(
                                       __FILE__, __LINE__, std::ref(m_ActiveSlide), theActiveSlide,
                                       inNewActiveSlide));
    }
}

void CDoc::SetSceneGraph(std::shared_ptr<Q3DStudio::IDocSceneGraph> inGraph)
{
    m_SceneGraph = inGraph;
}

void CDoc::GetProjectFonts(std::vector<std::pair<QString, QString>> &outFontNameFileList)
{
    outFontNameFileList.clear();
    qt3ds::render::ITextRenderer *theRenderer = m_SceneGraph->GetTextRenderer();
    if (theRenderer) {
        auto theProjectFonts = theRenderer->GetProjectFontList();
        for (uint32_t idx = 0, end = theProjectFonts.size(); idx < end; ++idx) {
            outFontNameFileList.push_back({theProjectFonts[idx].m_FontName,
                                           theProjectFonts[idx].m_FontFile});
        }
    }
}

void CDoc::GetProjectFonts(std::vector<QString> &outFonts)
{
    outFonts.clear();
    qt3ds::render::ITextRenderer *theRenderer = m_SceneGraph->GetTextRenderer();
    if (theRenderer) {
        auto theProjectFonts = theRenderer->GetProjectFontList();
        for (uint32_t idx = 0, end = theProjectFonts.size(); idx < end; ++idx)
            outFonts.push_back(theProjectFonts[idx].m_FontName);
    }
}

// Given a font path, return the font name
QString CDoc::GetProjectFontName(const Q3DStudio::CFilePath &inFullPathToFontFile)
{
    qt3ds::render::ITextRenderer *theRenderer = m_SceneGraph->GetTextRenderer();
    QString theFont;
    if (theRenderer) {
        auto theProjectFonts = theRenderer->GetProjectFontList();
        qCInfo(qt3ds::TRACE_INFO) << "Attempting to find font: "
                                  << inFullPathToFontFile.filePath();
        for (uint32_t idx = 0, end = theProjectFonts.size(); idx < end; ++idx) {
            // Using a CFilePath here instead of a CString means the file path object will normalize
            // the data coming from fontconfig.  For example, they always use forward slashes
            // instead of
            // back slashes.
            Q3DStudio::CFilePath theFontFile(theProjectFonts[idx].m_FontFile);
            if (inFullPathToFontFile == theFontFile) {
                qCInfo(qt3ds::TRACE_INFO) << "Matching against: " << theFontFile.filePath()
                                          << " SUCCEEDED";
                theFont = theProjectFonts[idx].m_FontName;
                break;
            } else {
                qCInfo(qt3ds::TRACE_INFO) << "Matching against: " << theFontFile.filePath()
                                          << " FAILED";
            }
        }
    }
    return theFont;
}

void CDoc::OnSlideDeleted(qt3dsdm::Qt3DSDMSlideHandle inSlide)
{
    using namespace qt3dsdm;
    if (inSlide == m_ActiveSlide) {
        ISlideSystem &theSlideSystem = *m_StudioSystem->GetFullSystem()->GetSlideSystem();
        Qt3DSDMSlideHandle theMaster = theSlideSystem.GetMasterSlide(inSlide);
        Qt3DSDMSlideHandle theFirstSlide = theSlideSystem.GetSlideByIndex(theMaster, 1);
        Qt3DSDMSlideHandle theNewSlide;
        if (theFirstSlide == inSlide)
            theNewSlide = theSlideSystem.GetSlideByIndex(theMaster, 2);
        else
            theNewSlide = theFirstSlide;
        SetActiveSlideWithTransaction(theNewSlide);
    }
}

void CDoc::OnInstanceDeleted(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (GetSelectedInstance() == inInstance)
        DeselectAllItems();

    if (m_ActiveLayer == inInstance)
        m_ActiveLayer = 0;

    using namespace qt3dsdm;
    CClientDataModelBridge &theBridge(*m_StudioSystem->GetClientDataModelBridge());
    SComposerObjectDefinitions &theDefinitions(theBridge.GetObjectDefinitions());
    IDataCore &theCore(*m_StudioSystem->GetFullSystem()->GetCoreSystem()->GetDataCore());
    if (theCore.IsInstanceOrDerivedFrom(inInstance, theDefinitions.m_SlideOwner.m_Instance)) {
        Qt3DSDMSlideHandle theSlide = theBridge.GetComponentActiveSlide(inInstance);
        if (theSlide == m_ActiveSlide) {
            // empty loop intentional, finding the next component parent.
            Qt3DSDMInstanceHandle theParent;
            for (theParent = m_AssetGraph->GetParent(inInstance); theParent.Valid()
                 && !theCore.IsInstanceOrDerivedFrom(theParent,
                                                     theDefinitions.m_SlideOwner.m_Instance);
                 theParent = m_AssetGraph->GetParent(theParent))
                ;

            if (theParent.Valid()) {
                QT3DS_ASSERT(theCore.IsInstanceOrDerivedFrom(theParent,
                                                             theDefinitions.m_SlideOwner.m_Instance));
                m_ActiveSlide = theBridge.GetComponentActiveSlide(theParent);
            } else {
                QT3DS_ASSERT(false);
                // This is fine because setting the active slide to zero will force it to default
                // to the scene's first slide.
                m_ActiveSlide = 0;
            }
        }
    }
    // Remove this instance from datainputs controlled element list
    for (auto &it : qAsConst(g_StudioApp.m_dataInputDialogItems)) {
        if (it->countOfInstance(inInstance))
            it->removeControlFromInstance(inInstance);
    }
}

void CDoc::onPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                             qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    using namespace qt3dsdm;
    const auto bridge = m_StudioSystem->GetClientDataModelBridge();
    // Save the material definition upon undo and redo
    if (m_Core->GetCmdStack()->isUndoingOrRedoing() &&
            bridge->isInsideMaterialContainer(inInstance)) {
        getSceneEditor()->saveIfMaterial(inInstance);
    }

    // If a material inside the material container is renamed, the file has to be renamed too
    // and the referenced materials that refer to that renamed material
    if (inProperty == bridge->GetNameProperty() && bridge->isInsideMaterialContainer(inInstance)) {
        const auto sceneEditor = getSceneEditor();
        const QString dirPath = GetDocumentDirectory();

        const auto renameMaterial = [&](const QPair<QString, QString> &materialRename) {
            const QString oldFile
                    = sceneEditor->getFilePathFromMaterialName(materialRename.first);
            const QString newFile
                    = sceneEditor->getFilePathFromMaterialName(materialRename.second);
            // If the newfile already exists ie. file was renamed manually by the user,
            // rename the referenced materials regardless
            if (QFileInfo(oldFile).exists())
                QFile::rename(oldFile, newFile);

            if (QFileInfo(newFile).exists()) {
                const QString newRelPath = QDir(dirPath).relativeFilePath(newFile);

                QVector<qt3dsdm::Qt3DSDMInstanceHandle> refMats;
                getSceneReferencedMaterials(GetSceneInstance(), refMats);
                for (auto &refMat : qAsConst(refMats)) {
                    const auto origMat = bridge->getMaterialReference(refMat);
                    if (origMat.Valid() && long(origMat) == inInstance) {
                        sceneEditor->setMaterialSourcePath(refMat,
                                    Q3DStudio::CString::fromQString(newRelPath));
                        sceneEditor->SetName(refMat, bridge->GetName(inInstance, true));
                        m_StudioSystem->GetFullSystemSignalSender()
                                ->SendInstancePropertyValue(refMat, bridge->GetNameProperty());
                    }
                }
            }
            GetCore()->getProjectFile().renameMaterial(materialRename.first,
                                                       materialRename.second);
            m_materialUndoRenames.append(QPair<QString, QString>(materialRename.second,
                                                                 materialRename.first));
        };

        // After a rename, the opposite rename is saved to an undo list
        // While doing an undo or redo, this list is checked for renames
        if (m_Core->GetCmdStack()->isUndoingOrRedoing()) {
            // Make a copy of the undo list since renameMaterial adds new renames to the same list
            auto materialUndoRenamesCopy = m_materialUndoRenames;
            for (int i = 0; i < materialUndoRenamesCopy.size();) {
                const auto &materialRename = materialUndoRenamesCopy[i];
                if (sceneEditor->GetName(inInstance).toQString() == materialRename.second) {
                    renameMaterial(materialRename);
                    m_materialUndoRenames.remove(i);
                    materialUndoRenamesCopy.remove(i);
                } else {
                    ++i;
                }
            }
        }

        // These renames are queued by the user by either renaming the material object
        // or the .materialdef file.
        for (int i = 0; i < m_materialRenames.size();) {
            const auto &materialRename = m_materialRenames[i];
            if (sceneEditor->GetName(inInstance).toQString() == materialRename.second) {
                renameMaterial(materialRename);
                m_materialRenames.remove(i);
            } else {
                ++i;
            }
        }
    }
    // check if we changed datainput bindings
    if (inProperty == m_StudioSystem->GetPropertySystem()
            ->GetAggregateInstancePropertyByName(inInstance, L"controlledproperty")) {
        // we need to rebuild the datainput map as we do not know what exactly
        // happened with controlledproperty property
        UpdateDatainputMapForInstance(inInstance);
    }
}

void CDoc::queueMaterialRename(const QString &oldName, const QString &newName) {
    m_materialRenames.append(QPair<QString, QString>(oldName, newName));
}

Q3DStudio::SSelectedValue CDoc::SetupInstanceSelection(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (m_StudioSystem->IsInstance(inInstance)
            && !m_StudioSystem->GetClientDataModelBridge()->IsLockedAtAll(inInstance)) {
        return Q3DStudio::SSelectedValue(inInstance);
    }
    return Q3DStudio::SSelectedValue();
}

/**
 *	Select DataModel Object given its instance handle.
 *	@param inInstanceHandle The instance handle of the DataModel Object to be selected
 */
void CDoc::SelectDataModelObject(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle)
{
    qt3dsdm::TInstanceHandleList theObjects = m_SelectedValue.GetSelectedInstances();
    if (std::find(theObjects.begin(), theObjects.end(), inInstanceHandle) == theObjects.end())
        NotifySelectionChanged(SetupInstanceSelection(inInstanceHandle));
    else
        NotifySelectionChanged(theObjects);
}

void CDoc::ToggleDataModelObjectToSelection(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (m_StudioSystem->GetClientDataModelBridge()->IsMultiSelectable(inInstance)) {
        qt3dsdm::TInstanceHandleList theNewHandles;
        if (m_SelectedValue.getType() != Q3DStudio::SelectedValueTypes::MultipleInstances) {
            // Attempt conversion if possible.
            if (m_SelectedValue.getType() == Q3DStudio::SelectedValueTypes::Instance) {
                qt3dsdm::Qt3DSDMInstanceHandle theCurrentlySelectedInstance =
                        m_SelectedValue.getData<qt3dsdm::Qt3DSDMInstanceHandle>();
                if (m_StudioSystem->GetClientDataModelBridge()->IsMultiSelectable(
                            theCurrentlySelectedInstance))
                    theNewHandles.push_back(theCurrentlySelectedInstance);
            }
        } else {
            theNewHandles = m_SelectedValue.getData<qt3dsdm::TInstanceHandleList>();
        }

        auto iter = std::find(theNewHandles.begin(), theNewHandles.end(), inInstance);
        if (iter == theNewHandles.end())
            theNewHandles.push_back(inInstance);
        else
            theNewHandles.erase(iter);
        NotifySelectionChanged(theNewHandles);
    }
}

void CDoc::SelectAndNavigateToDataModelObject(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle)
{
    if (!inInstanceHandle.Valid()) {
        QT3DS_ASSERT(false);
        return;
    }
    Q3DStudio::IDocumentReader &theReader(GetDocumentReader());
    qt3dsdm::Qt3DSDMSlideHandle theAssociatedSlide = theReader.GetAssociatedSlide(inInstanceHandle);
    qt3dsdm::Qt3DSDMInstanceHandle theNewComponent =
            theReader.GetComponentForSlide(theAssociatedSlide);
    qt3dsdm::Qt3DSDMInstanceHandle theOldComponent = theReader.GetComponentForSlide(GetActiveSlide());
    if (theNewComponent.Valid() && theNewComponent != theOldComponent) {
        // Get the currently active slide for the new component.
        qt3dsdm::Qt3DSDMSlideHandle theActiveSlide =
                theReader.GetComponentActiveSlide(theNewComponent);
        NotifyActiveSlideChanged(theActiveSlide, true);
    }
    NotifySelectionChanged(SetupInstanceSelection(inInstanceHandle));
}

/**
 *	Get the latest end time of the children of the Active Root.
 */
long CDoc::GetLatestEndTime()
{
    long theTime = 0;
    Q3DStudio::CGraphIterator theChildren;
    GetAssetChildrenInTimeParent(GetActiveRootInstance(), this, true, theChildren,
                                 GetActiveSlide());
    for (; !theChildren.IsDone(); ++theChildren) {
        qt3dsdm::Qt3DSDMInstanceHandle theChildAsset = theChildren.GetCurrent();
        long theChildEnd = GetDocumentReader().GetTimeRange(theChildAsset).second;
        if ((theChildEnd > theTime))
            theTime = theChildEnd;
    }
    return theTime;
}

bool CDoc::isPlayHeadAtEnd()
{
    return m_CurrentViewTime >= GetLatestEndTime();
}

void CDoc::OnComponentSeconds()
{
    long theTime = GetCurrentClientTime();

    m_CurrentViewTime = theTime;
    QT3DS_PROFILE(NotifyTimeChanged_UpdateAllViews);

    m_Core->GetDispatch()->FireOnTimeChanged(m_CurrentViewTime);
}

/**
 *	Tell Client that the time has changed and update views accordingly.
 *	Set the current time on the current time context to be inNewTime.
 *	@param inNewTime The time to set Client to (in milliseconds)
 */
void CDoc::NotifyTimeChanged(long inNewTime)
{
    if (m_PlaybackClock)
        m_PlaybackClock->OnTimeChange(inNewTime);
    DoNotifyTimeChanged(inNewTime);
}

void CDoc::DoNotifyTimeChanged(long inNewTime)
{
    QT3DS_PROFILE(NotifyTimeChanged);

    // Make sure time is within valid range
    long theMinTime = 0; // min time is always 0
    if (inNewTime < theMinTime)
        inNewTime = theMinTime;
    else {
        long theLatestEndTime = GetLatestEndTime();
        if (inNewTime > theLatestEndTime)
            inNewTime = theLatestEndTime;
    }

    // Update DataModel
    qt3dsdm::Qt3DSDMSlideHandle theMasterSlide =
            m_StudioSystem->GetSlideSystem()->GetMasterSlide(GetActiveSlide());
    // TODO: fix precision issue from converting to/from float & long. choose 1 type
    m_StudioSystem->GetSlideSystem()->SetComponentSeconds(theMasterSlide, (float)inNewTime / 1000);
}

/**
 * Sets the timebar time range of the currently selected object in the timeline
 * @param inSetStart true to set the Start time, false sets the End time
 * @param inAffectsChildren true if the children should have their start/end times in sync
 */
void CDoc::TruncateTimebar(bool inSetStart, bool inAffectsChildren)
{
    Q_UNUSED(inAffectsChildren);

    qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance = GetSelectedInstance();
    // Cannot change the time bars for a material
    if (theSelectedInstance.Valid()) {
        Q3DStudio::ScopedDocumentEditor(*this, QObject::tr("Truncate Time Range"),
                                        __FILE__, __LINE__)
                ->TruncateTimeRange(theSelectedInstance, inSetStart, GetCurrentViewTime());
    }
}

/**
 * Tell Client to be in either PLAY or PAUSE mode.
 * This potentially changes the current mode of the Client, whether it should
 * Play or Pause.
 * @param inPlayMode Either PLAYMODE_PLAY or PLAYMODE_STOP
 * @param inRestoreTime whether there is a time to restore for time context
 */
void CDoc::SetPlayMode(EPlayMode inPlayMode, long inRestoreTime /*= -1*/)
{
    if (m_PlayMode != inPlayMode)
    {
        m_PlayMode = inPlayMode;

        if (inPlayMode == PLAYMODE_PLAY) {
            // Set Client to PLAY
            m_PlaybackClock->StartPlayback();
            m_Core->GetDispatch()->FireOnPlayStart();
        } else {
            // Set Client to STOP
            if (inRestoreTime >= 0)
                NotifyTimeChanged(inRestoreTime); // update views as indicated by client
            m_PlaybackClock->StopPlayback();
            m_Core->GetDispatch()->FireOnPlayStop();
        }
    }
}

/**
 * Determine if Studio is playing the presentation.
 * @return true if Studio is currently in play mode
 */
bool CDoc::IsPlaying()
{
    return m_PlayMode == PLAYMODE_PLAY;
}

/**
 * Returns the time Client believes it to be.
 * This requests the current time from Client. This allows in-place previewing
 * of the presentation in Studio.
 * @return The current time of Client in milliseconds.
 */
long CDoc::GetCurrentClientTime()
{
    if (m_ActiveSlide.Valid())
        return m_StudioSystem->GetSlideSystem()->GetComponentSecondsLong(m_ActiveSlide);
    return 0;
}

/**
 * Get the current visible time.
 * This differs by the current client time in that you can move the playhead
 * without Client updating, causing the view time to change but not client time.
 * This usually happens when control-dragging the playhead.
 * @return the current view time.
 */
long CDoc::GetCurrentViewTime() const
{
    return m_CurrentViewTime;
}

void CDoc::SetDirectoryWatchingSystem(
        std::shared_ptr<Q3DStudio::IDirectoryWatchingSystem> inSystem)
{
    m_DirectoryWatchingSystem = inSystem;
}

Q3DStudio::IDirectoryWatchingSystem *CDoc::GetDirectoryWatchingSystem() const
{
    return m_DirectoryWatchingSystem ? m_DirectoryWatchingSystem.get() : nullptr;
}

bool CDoc::SetDocumentPath(const QString &inDocumentPath)
{
    // We always need to have a document path.
    if (inDocumentPath.isEmpty()) {
        ASSERT(false); // User should have specified which file.
        m_DocumentPath = CreateUntitledDocument();
    } else {
        m_DocumentPath = inDocumentPath;
        QFile f(m_DocumentPath);
        if (!f.exists()) // If the file doesn't exist, create it.
            f.open(QIODevice::ReadWrite);
    }

    // Document path should always be absolute path and it should exist
    QFileInfo info(m_DocumentPath);
    if (info.isRelative() || !info.exists())
        return false;

    m_Core->GetDispatch()->FireOnDocumentPathChanged(m_DocumentPath);
    return true;
}

/**
 * Create Untitled document in user directory
 */
QString CDoc::CreateUntitledDocument() const
{
    QString dirPath = QDir::cleanPath(Q3DStudio::CFilePath::GetUserApplicationDirectory()
                                              + QStringLiteral("/Qt3DStudio/Untitled"));
    QDir dir(dirPath);
    dir.mkpath(QStringLiteral("."));
    QString filePath = dirPath + QStringLiteral("/Untitled.uip");

     // create the file if it doesn't exist
    if (!QFileInfo(filePath).exists()) {
        QFile f(filePath);
        f.open(QIODevice::ReadWrite);
    }
    return filePath;
}

void CDoc::SetImportFailedHandler(std::shared_ptr<Q3DStudio::IImportFailedHandler> inHandler)
{
    m_ImportFailedHandler = inHandler;
}

std::shared_ptr<Q3DStudio::IImportFailedHandler> CDoc::GetImportFailedHandler()
{
    return m_ImportFailedHandler;
}

void CDoc::SetDocMessageBoxHandler(
        std::shared_ptr<Q3DStudio::IDeletingReferencedObjectHandler> inHandler)
{
    m_DeletingReferencedObjectHandler = inHandler;
}

void CDoc::setMoveRenameHandler(std::shared_ptr<Q3DStudio::IMoveRenameHandler> inHandler)
{
    m_moveRenameHandler = inHandler;
}

std::shared_ptr<Q3DStudio::IMoveRenameHandler> CDoc::getMoveRenameHandler()
{
    return m_moveRenameHandler;
}

// absolute document path
QString CDoc::GetDocumentPath() const
{
    return m_DocumentPath;
}

/**
 * Gets the document path relative to the project root
 */
QString CDoc::getRelativePath() const
{
    return QDir(GetCore()->getProjectFile().getProjectPath())
            .relativeFilePath(m_DocumentPath);
}

void CDoc::setPresentationId(const QString &id)
{
    m_presentationId = id;
}

QString CDoc::getPresentationId() const
{
    return m_presentationId;
}

QString CDoc::GetDocumentDirectory() const
{
    return QFileInfo(m_DocumentPath).path();
}

/**
 * Given an absolute path, return the relative path to doc.
 * This is used when we drag image / behavior / other files to scene.
 * In future we may want to return path to handle importing files from $CommonAssets.
 */
QString CDoc::GetRelativePathToDoc(const Q3DStudio::CFilePath &inPath)
{
    return QDir(GetDocumentDirectory()).relativeFilePath(inPath.toQString());
}

/**
 * Given a path (may be relative or absolute), return an absolute path.
 * If the path is relative, it will be resolved based on document path.
 * In future we may want to resolve path based on $CommonAssets.
 */
QString CDoc::GetResolvedPathToDoc(const Q3DStudio::CFilePath &inPath)
{
    // If it is a relative path, resolve it.
    if (!inPath.IsAbsolute()) {
        // Sanity check that document path has been set properly.
        ASSERT(QFileInfo(m_DocumentPath).exists());

        return QDir::cleanPath(QFileInfo(m_DocumentPath).dir()
                               .absoluteFilePath(inPath.toQString()));
    }
    return inPath.toQString();
}

/**
 * Close the current document.
 * This will remove all the items from the scene, perform all the cleanup.
 * The state of the Doc is invalid after this and a CreateNewDocument or LoadDocument must be called
 * following this, for a new presentation.
 */
void CDoc::CloseDocument()
{
    // selection would be invalid from this point onwards
    DeselectAllItems();

    m_SceneEditor.reset();
    if (m_DocumentBufferCache) // Ensure old buffers aren't picked up for the same relative path.
        m_DocumentBufferCache->Clear();

    CDispatchDataModelNotificationScope __dispatchScope(*GetCore()->GetDispatch());
    SetPlayMode(PLAYMODE_STOP);

    m_Core->GetDispatch()->FireOnClosingPresentation();

    CleanupData(); // clean up all studio data
    ClosePresentation();

    if (m_SceneInstance.Valid()) {
        m_AssetGraph->RemoveNode(m_SceneInstance);
        m_SceneInstance = 0;

        // Destroy Asset Graph
        m_AssetGraph = TAssetGraphPtr();
    }

    // Clear the modified flag - no data loaded
    SetModifiedFlag(false);

    // Invalidate document path, so isValid() returns false
    m_DocumentPath.clear();
}

/**
 * Called when the core opens a UIP file.
 */
void CDoc::LoadDocument(const QString &inDocument)
{
    ResetDataCore();

    CFileInputStream theFileStream(inDocument);
    CBufferedInputStream theBufferedStream(&theFileStream, QFileInfo(inDocument).size());
    SetDocumentPath(inDocument); // SetDocumentPath before LoadPresentation because we need
    // DocumentPath to load relative resources such as images
    LoadPresentationFile(&theBufferedStream);
}

void CDoc::SaveDocument(const QString &inDocument)
{
    // Remove unused materials from the container during saving so that the .uip is not cluttered
    Q3DStudio::CUpdateableDocumentEditor updatableEditor(*this);
    updatableEditor.EnsureEditor(QString(), __FILE__, __LINE__)
            .removeUnusedFromMaterialContainer();

    CFileOutputStream theFileStream(inDocument);
    // Exceptions here get propagated to the crash dialog.
    CBufferedOutputStream theBufferStream(&theFileStream);
    SavePresentationFile(&theBufferStream);
    theBufferStream.Close();

    // Rollback material container changes so that undos work (material property changes etc.)
    updatableEditor.RollbackEditor();
}

/**
 * This will create and load a new document with all the default resources.
 * This should only be called on an empty document (after CloseDocument) and
 * will do all the creation of the resources (default Layer, Light, Camera etc)
 */
void CDoc::CreateNewDocument()
{
    using namespace qt3dsdm;
    using namespace Q3DStudio;

    CDispatchDataModelNotificationScope __dispatchScope(*m_Core->GetDispatch());
    ResetDataCore();

    CreatePresentation();

    // Create the default objects in the scene
    Qt3DSDMSlideHandle theSlide =
            m_StudioSystem->GetClientDataModelBridge()->GetOrCreateGraphRoot(m_SceneInstance);
    IDataCore &theDataCore(
                *m_StudioSystem->GetFullSystem()->GetCoreSystem()->GetTransactionlessDataCore());
    ISlideSystem &theSlideSystem(*m_StudioSystem->GetFullSystem()->GetSlideSystem());
    SComposerObjectDefinitions &theObjectDefinitions(
                m_StudioSystem->GetClientDataModelBridge()->GetObjectDefinitions());
    IMetaData &theMetaData(*m_StudioSystem->GetActionMetaData());

    Qt3DSDMInstanceHandle theLayer = IDocumentEditor::CreateSceneGraphInstance(
                ComposerObjectTypes::Convert(ComposerObjectTypes::Layer), m_SceneInstance, theSlide,
                theDataCore, theSlideSystem, theObjectDefinitions, *m_AssetGraph, theMetaData);

    IDocumentEditor::CreateSceneGraphInstance(
                ComposerObjectTypes::Convert(ComposerObjectTypes::Camera), theLayer, theSlide, theDataCore,
                theSlideSystem, theObjectDefinitions, *m_AssetGraph, theMetaData);
    IDocumentEditor::CreateSceneGraphInstance(
                ComposerObjectTypes::Convert(ComposerObjectTypes::Light), theLayer, theSlide, theDataCore,
                theSlideSystem, theObjectDefinitions, *m_AssetGraph, theMetaData);

    OnNewPresentation();

    // Override the modified flag that's set when we create the default resources.
    SetModifiedFlag(false);
}

/**
 * Create a new presentation within Client.
 */
void CDoc::CreatePresentation()
{
    // m_SceneInstance should be invalid
    ASSERT(!m_SceneInstance.Valid());

    // TODO: Move these to ComponentFactory

    // Create the top-level scene object
    Q3DStudio::CId theSceneId(SCENE_GUID);

    CClientDataModelBridge *theBridge = m_StudioSystem->GetClientDataModelBridge();
    m_SceneInstance = theBridge->CreateAssetInstance(theSceneId, OBJTYPE_SCENE);

    // Initialize Asset Graph
    m_AssetGraph = TAssetGraph::CreateGraph();
    m_AssetGraph->AddRoot(m_SceneInstance);
}

void CDoc::ClosePresentation()
{
    OnPresentationDeactivated();
}

/**
 *	Step the client (this is really overridding update in client)
 */
void CDoc::ClientStep()
{
    m_PlaybackClock->UpdateTime();
}

/**
 * Removes all StudioObjects from the Map
 * This cleans up all of the CStudioObjects that Studio owns.
 * This is called when shutdown and prior to loading or creating a new presentation.
 * Nothing should be instantiated in this call, since this is called in the destructor.
 */
void CDoc::CleanupData()
{
    // Make sure selection is cleared
    DeselectAllItems();

    // Clear references that are now invalid
    m_ActiveSlide = 0;
    SetActiveLayer(0);
}

/**
 * Process a copy command.
 * This will copy Actions, Keyframes or objects, depending on what is selected.
 * If the Action Palette is visible, then this will do an Action copy.
 * If there are any keyframes selected then this will do a keyframe copy
 */
void CDoc::HandleCopy()
{
    if (canCopySelectedActions()) {
        ASSERT(0); // Dispatch ... and/or, what?
        // m_StudioApp->GetViews( )->GetActionControl( )->OnCopyAction( );
    } else if (m_KeyframesManager && m_KeyframesManager->HasSelectedKeyframes()) {
        if (canCopySelectedKeyframes())
            m_KeyframesManager->CopyKeyframes();
    } else if (canCopySelectedObjects()) {
        CopyObject(m_SelectedValue.GetSelectedInstances());
    }
}

/**
 * Process a paste command.
 * If there is an action on the clipboard, and the action palette is visible,
 * this will paste the action.
 * Otherwise, this will paste an object if the clipboard data has an object
 * data type in it, else it will attempt to paste keyframes.
 */
void CDoc::HandlePaste()
{
    if (canPasteActions()) {
        ASSERT(0); // dispatch
        // m_StudioApp->GetViews( )->GetActionControl( )->OnPasteAction( );
        // m_StudioApp->GetViews( )->OnShowAction( );
    } else if (canPasteObjects()) {
        PasteObject(getPasteTarget(GetSelectedInstance()));
    } else {
        if (m_KeyframesManager)
            m_KeyframesManager->PasteKeyframes();
    }
}

/**
 * Process a paste command for Master Slide.
 * If there is an action on the clipboard, and the action palette is visible,
 * this will paste the action.
 * Otherwise, this will paste an object if the clipboard data has an object
 * data type in it, else it will attempt to paste keyframes.
 */
void CDoc::HandleMasterPaste()
{
    using namespace Q3DStudio;

    if (canPasteActions()) {
        ASSERT(0); // dispatch
        // m_StudioApp->GetViews( )->GetActionControl( )->OnPasteAction( );
        // m_StudioApp->GetViews( )->OnShowAction( );
    } else if (canPasteObjects()) {
        qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance = getPasteTarget(GetSelectedInstance());
        long theTargetObjectType =
                GetStudioSystem()->GetClientDataModelBridge()->GetObjectType(theSelectedInstance);
        qt3dsdm::ISlideSystem *theSlideSystem = GetStudioSystem()->GetSlideSystem();
        qt3dsdm::Qt3DSDMSlideHandle theTargetSlide =
                theSlideSystem->GetAssociatedSlide(theSelectedInstance);

        if (theTargetObjectType != OBJTYPE_SCENE && theTargetObjectType != OBJTYPE_COMPONENT) {
            if (theTargetSlide && theSlideSystem->IsMasterSlide(theTargetSlide)) {
                qt3dsdm::Qt3DSDMSlideHandle theMasterSlideHandle =
                        theSlideSystem->GetMasterSlide(theTargetSlide);
                if (theMasterSlideHandle.Valid())
                    theTargetSlide = theMasterSlideHandle;
                PasteObjectMaster(theSelectedInstance);
            } else {
                PasteObject(theSelectedInstance);
            }
        } else {
            qt3dsdm::Qt3DSDMSlideHandle theMasterSlideHandle =
                    theSlideSystem->GetMasterSlide(theTargetSlide);
            if (theMasterSlideHandle.Valid())
                theTargetSlide = theMasterSlideHandle;
            PasteObjectMaster(theSelectedInstance);
        }
    } else {
        if (m_KeyframesManager)
            m_KeyframesManager->PasteKeyframes();
    }
}

/**
 * Process a cut command.
 * This will copy/cut Action, if the Action Palette is visible, and an action
 * is selected.  Otherwise the following holds true.
 * This will copy Keyframes or objects, depending on what is selected.
 * If there are any keyframes selected then this will do a keyframe copy
 * The object/keyframe(s) will then be deleted.
 */
void CDoc::HandleCut()
{
    // we only need to check if it can be copied. If it can be copied, then it can be cut.
    if (canCopySelectedActions()) {
        ASSERT(0); // dispatch
        // m_StudioApp->GetViews( )->GetActionControl( )->OnCutAction( );
    } else if (m_KeyframesManager && m_KeyframesManager->HasSelectedKeyframes()) {
        m_KeyframesManager->CopyKeyframes();
        m_KeyframesManager->RemoveKeyframes(true);
    } else if (canCopySelectedObjects()) {
        CutSelectedObject();
    }
}

bool CDoc::canCopyObjects(const qt3dsdm::TInstanceHandleList &inInstances) const
{
    if (inInstances.empty())
        return false;

    bool retval = true;
    const qt3dsdm::TInstanceHandleList &theInstances = inInstances;
    for (size_t idx = 0, end = theInstances.size(); idx < end && retval; ++idx)
        retval &= m_StudioSystem->GetClientDataModelBridge()->IsDuplicateable(theInstances[idx]);

    return retval;
}

/**
 * Check to see if selected object can be copied into the clipboard
 */
bool CDoc::canCopySelectedObjects() const
{
    return canCopyObjects(m_SelectedValue.GetSelectedInstances());
}

/**
 * Check to see if a keyframe can be copied into the clipboard
 */
bool CDoc::canCopySelectedKeyframes() const
{
    return m_KeyframesManager ? m_KeyframesManager->CanPerformKeyframeCopy() : false;
}

/**
 * Check to see if an Action can be copied onto the clipboard.
 * @return true if an Action can be copied onto the clipboard.
 */
bool CDoc::canCopySelectedActions() const
{
    bool theCanCopy = false;

    // TODO: Refactor selection
    // Only if views are already created;
    // Otherwise we can neither obtain the ActionControl,
    // nor can we check if the ActionControl can perform a copy.
    // if ( m_StudioApp->GetViews( )->AreViewsCreated( ) )
    //{
    //	theCanCopy = m_StudioApp->GetViews( )->GetActionControl( )->CanCopyAction( );
    //}

    return theCanCopy;
}

/**
 * Check to see if an object can be pasted into the Scene.
 */
bool CDoc::canPasteObjects() const
{
    return getPasteTarget(GetSelectedInstance()).Valid();
}

/**
 * Check to see if a keyframe can be pasted from the clipboard
 */
bool CDoc::canPasteKeyframes() const
{
    return m_KeyframesManager ? m_KeyframesManager->CanPerformKeyframePaste() : false;
}

/**
 * Check to see if an action can be pasted from the clipboard.
 * Paste action is only allowed if there is an Action on the clipboard, an asset
 * is selected, and the Timeline is in focus.
 * @return true if we can paste an action.
 */
bool CDoc::canPasteActions() const
{
    bool theCanPaste = false;

    // TODO: Refactor selection
    // CViews* theViews = m_StudioApp->GetViews( );

    //// Only if views are already created;
    //// Otherwise we can neither obtain the ActionControl,
    //// nor can we check if the ActionControl can perform a paste.
    // if ( m_StudioApp->GetViews( )->AreViewsCreated( ) )
    //{
    //	theCanPaste = theViews->GetActionControl( )->CanPasteAction( ) && (
    //theViews->GetTimelineControl( )->IsInFocus( ) || theViews->GetActionControl( )->IsInFocus( )
    //);
    //}

    return theCanPaste;
}

/**
 * Is there valid selected keyframe(s), object(s), or action(s) to be copied?
 */
bool CDoc::canCopy() const
{
    return canCopySelectedKeyframes() || canCopySelectedObjects() || canCopySelectedActions();
}

/**
 * Check to see if an object or keyframe(s) can be pasted into the Scene.
 */
bool CDoc::canPaste() const
{
    return canPasteKeyframes() || canPasteObjects() || canPasteActions();
}

/**
 * Check to see if an object or keyframe(s) can be cut.
 */
bool CDoc::canCut()
{
    // copyable keyframes and actions are cuttable (i.e. can be deleted).
    if (canCopySelectedActions() || canCopySelectedKeyframes())
        return true;

    if (canCopySelectedObjects()) {
        // Check the Object is not the scene. The Object cannot be the last layer as well
        qt3dsdm::TInstanceHandleList theInstances = m_SelectedValue.GetSelectedInstances();
        bool canDelete = !theInstances.empty();
        for (size_t idx = 0, end = theInstances.size() && canDelete; idx < end; ++idx) {
            canDelete &= GetDocumentReader().IsInstance(theInstances[idx])
                    && GetStudioSystem()->GetClientDataModelBridge()->CanDelete(theInstances[idx]);
        }
        return canDelete;
    }

    return false;
}

/**
 * Handles the duplicate command passed by mainframe.
 * Makes a copy of the currently selected item (if there is one) and attaches
 * it to the same parent as the original.
 */
void CDoc::HandleDuplicateCommand(bool slide)
{
    using namespace Q3DStudio;
    qt3dsdm::Qt3DSDMInstanceHandle theSelectedInstance = GetSelectedInstance();
    CClientDataModelBridge *bridge = m_StudioSystem->GetClientDataModelBridge();
    qt3dsdm::Qt3DSDMSlideHandle slideHandle = GetActiveSlide();

    // If we have a non-master slide or a valid object to duplicate
    if (slide && slideHandle != m_StudioSystem->GetSlideSystem()->GetMasterSlide(slideHandle)) {
        SCOPED_DOCUMENT_EDITOR(*this, QObject::tr("Duplicate Slide"))->DuplicateSlide(slideHandle);
    } else if (bridge->IsDuplicateable(theSelectedInstance)) {
        SCOPED_DOCUMENT_EDITOR(*this, QObject::tr("Duplicate Object"))
                ->DuplicateInstance(theSelectedInstance);
    }
}

/**
 * Verify that an asset can be renamed. Only actions are affected by rename at this point, so prompt
 * user to proceed if the actions will be hosed.
 */
bool CDoc::VerifyCanRename(qt3dsdm::Qt3DSDMInstanceHandle inAsset)
{
    bool theResult = true;
    std::set<Q3DStudio::CString> theAffectedList;
    GetActionsAffectedByRename(inAsset, theAffectedList);
    if (!theAffectedList.empty()) {
        Q3DStudio::CString theFormulatedString;
        std::set<Q3DStudio::CString>::iterator thePos = theAffectedList.begin();
        for (; thePos != theAffectedList.end(); ++thePos) {
            if (theFormulatedString.IsEmpty())
                theFormulatedString += (*thePos);
            else
                theFormulatedString += ", " + (*thePos);
        }

        ASSERT(0); // Dialogs by dispatch
        //QString theTitle(tr("Confirm Rename Object"));
        //QString theMessage(tr("The following objects have actions that reference this object "
        //                      "and/or its descendants:\n%1\nAre you sure that you want to "
        //                      "rename?").arg(theFormulatedString.toQString()));
        //
        //if (m_StudioApp->GetDialogs( )->DisplayMessageBox(
        //            theTitle, theMessage,
        //            Qt3DSMessageBox::ICON_WARNING, true) == Qt3DSMessageBox::MSGBX_CANCEL) {
        //    theResult = false;
        //}
    }
    return theResult;
}

/**
 * Load a stream of a UIP file.
 */
void CDoc::LoadPresentationFile(CBufferedInputStream *inInputStream)
{
    // Let any interested parties know that a presentation is going to be loaded
    m_Core->GetDispatch()->FireOnLoadingPresentation();

    int uipVersion = LoadStudioData(inInputStream);

    // We have a new presentation and a new active time context (scene)
    OnNewPresentation();
    qt3dsdm::Qt3DSDMSlideHandle theMasterSlide =
            m_StudioSystem->GetClientDataModelBridge()->GetComponentSlide(m_SceneInstance, 0);
    qt3dsdm::Qt3DSDMSlideHandle theChildSlide =
            m_StudioSystem->GetClientDataModelBridge()->GetComponentSlide(m_SceneInstance, 1);
    m_StudioSystem->GetFullSystem()->GetSignalSender()->SendActiveSlide(theMasterSlide, 1,
                                                                        theChildSlide);

    if (uipVersion < 6) {
        QTimer::singleShot(0, [=](){
            // uipVersion 6 introduced vec4 colors
            g_StudioApp.GetDialogs()->DisplayMessageBox(
                        tr("Old Presentation Version"),
                        tr("Some custom materials, effects, and behaviors may not work correctly."),
                        Qt3DSMessageBox::ICON_WARNING, false);
        });
    }

    if (uipVersion == 3) {
        bool cleaned = m_SceneEditor->CleanUpMeshes();
        QTimer::singleShot(0, [=](){
            // Update UIP version
            g_StudioApp.OnSave();
            if (cleaned) {
                // Show message box only if some meshes were cleaned
                g_StudioApp.GetDialogs()->DisplayMessageBox(
                            tr("Old Presentation Version"),
                            tr("Presentation was in old format and had unoptimized meshes.\n"
                               "They were optimized, and presentation version was updated."),
                            Qt3DSMessageBox::ICON_INFO, false);
            }
        });
    }
}

/**
 * Loads the Studio object data from an archive
 * Loads Studio object data from a presentation file archive.
 * @param inArchive CArchive from which to load the data objects.
 */
int CDoc::LoadStudioData(CBufferedInputStream *inInputStream)
{
    using namespace std;
    using namespace qt3dsdm;
    using namespace Q3DStudio;
    qt3ds::QT3DSI32 theVersion = 0;

    QT3DS_PROFILE(LoadStudioData);
    bool theModifiedFlag = false;

    try {
        // Disable all property change signals until the load is complete.
        // This cuts down on a lot of redraw calls.
        CDispatchDataModelNotificationScope __dispatchScope(*GetCore()->GetDispatch());

        {
            std::shared_ptr<IDOMReader> theReaderPtr =
                    CreateDOMReader(*inInputStream, theVersion);
            if (!theReaderPtr)
                throw CInvalidFileFormatException();

            IDOMReader &theReader(*theReaderPtr);

            theReader.Att("version", theVersion);

            CProjectSettingsSerializer theProjectSettingsSerializer(
                        m_Core->GetStudioProjectSettings());
            theReader.Serialize(L"ProjectSettings", theProjectSettingsSerializer);

            if (m_AssetGraph)
                m_AssetGraph->Clear();
            else
                m_AssetGraph = TAssetGraph::CreateGraph();

            // We definitely don't want a million events firing off during this deserialization.
            std::shared_ptr<IComposerSerializer> theSerializer(CreateTransactionlessSerializer());
            theSerializer->SerializeScene(theReader, GetDocumentDirectory(), int(theVersion));
        }

        auto bridge = GetStudioSystem()->GetClientDataModelBridge();

        // Setup the Presentation and Scene
        // Asset Graph has only one root and that's the scene
        m_SceneInstance = m_AssetGraph->GetRoot(0);
        m_ActiveSlide = bridge->GetComponentSlide(m_SceneInstance, 1);

        // Make sure material container has no duration
        const auto matCont = bridge->getMaterialContainer();
        const auto slideCore = GetStudioSystem()->GetFullSystem()->GetCoreSystem()->GetSlideCore();
        if (matCont.Valid()) {
            slideCore->forceSetInstancePropertyValueOnAllSlides(
                        matCont, bridge->GetSceneAsset().m_EndTime, 0);
        }
    } catch (...) {
        CleanupData();
        throw; // pass the error along to the caller, so the appropriate error message can be
        // feedback
    }
    SetModifiedFlag(theModifiedFlag);

    return theVersion;
}

/**
 *	Create new data core. Typically in construction or loading a new presentation
 */
void CDoc::ResetDataCore()
{
    m_Connections.clear();
    if (m_StudioSystem)
        m_StudioSystem->ResetDatabase();
    else
        m_StudioSystem = std::make_shared<qt3dsdm::CStudioSystem>(this);

    // Setup defaults
    qt3dsdm::IStudioAnimationSystem *theAnimationSystem = GetStudioSystem()->GetAnimationSystem();
    theAnimationSystem->SetInterpolation(CStudioPreferences::GetInterpolation());
    theAnimationSystem->SetAutoKeyframe(CStudioPreferences::IsAutosetKeyframesOn());
}

void CDoc::SetupDataCoreSignals()
{
    // Setup the cascading of instance delete notifications so that we can reset our selection.
    std::shared_ptr<qt3dsdm::IDataCore> theDataCore =
            m_StudioSystem->GetFullSystem()->GetCoreSystem()->GetDataCore();
    std::shared_ptr<qt3dsdm::IDataCoreSignalProvider> theProvider =
            std::dynamic_pointer_cast<qt3dsdm::IDataCoreSignalProvider>(theDataCore);
    std::shared_ptr<qt3dsdm::ISlideCoreSignalProvider> theSlideProvider =
            std::dynamic_pointer_cast<qt3dsdm::ISlideCoreSignalProvider>(
                m_StudioSystem->GetFullSystem()->GetCoreSystem()->GetSlideCore());

    if (theProvider)
        m_Connections.push_back(theProvider->ConnectBeforeInstanceDeleted(
                                std::bind(&CDoc::OnInstanceDeleted, this, std::placeholders::_1)));

    if (theSlideProvider)
        m_Connections.push_back(theSlideProvider->ConnectBeforeSlideDeleted(
                                    std::bind(&CDoc::OnSlideDeleted, this, std::placeholders::_1)));
    m_Connections.push_back(
                m_StudioSystem->GetFullSystem()->GetSignalProvider()->ConnectComponentSeconds(
                    std::bind(&CDoc::OnComponentSeconds, this)));
    m_Connections.push_back(
                m_StudioSystem->GetFullSystem()->GetSignalProvider()->ConnectActiveSlide(
                    std::bind(&CDoc::OnComponentSeconds, this)));

    // listener to keep track of datainput bindings
    m_Connections.push_back(
                m_StudioSystem->GetFullSystem()->GetSignalProvider()->ConnectInstancePropertyValue(
                    std::bind(&CDoc::onPropertyChanged, this,
                              std::placeholders::_1, std::placeholders::_2)));
}

std::shared_ptr<Q3DStudio::IComposerSerializer> CDoc::CreateSerializer()
{
    using namespace qt3dsdm;
    using namespace Q3DStudio;
    CStudioFullSystem &theFullSystem(*m_StudioSystem->GetFullSystem());
    CStudioCoreSystem &theCoreSystem(*theFullSystem.GetCoreSystem());
    CClientDataModelBridge &theClientBridge(*GetStudioSystem()->GetClientDataModelBridge());
    return IComposerSerializer::CreateGraphSlideSerializer(
                *theCoreSystem.GetDataCore(), *theCoreSystem.GetNewMetaData(),
                *theCoreSystem.GetSlideCore(), *theCoreSystem.GetAnimationCore(),
                *theCoreSystem.GetActionCore(), *m_AssetGraph, *theFullSystem.GetSlideSystem(),
                *theFullSystem.GetActionSystem(), *theCoreSystem.GetSlideGraphCore(),
                theClientBridge.GetObjectDefinitions(), m_ImportFailedHandler,
                *theCoreSystem.GetGuideSystem(), *GetSceneGraph()->GetPathManager(),
                *theFullSystem.GetPropertySystem());
}

std::shared_ptr<Q3DStudio::IComposerSerializer> CDoc::CreateTransactionlessSerializer()
{
    using namespace qt3dsdm;
    using namespace Q3DStudio;
    CStudioFullSystem &theFullSystem(*m_StudioSystem->GetFullSystem());
    CStudioCoreSystem &theCoreSystem(*theFullSystem.GetCoreSystem());
    CClientDataModelBridge &theClientBridge(*GetStudioSystem()->GetClientDataModelBridge());
    return IComposerSerializer::CreateGraphSlideSerializer(
                *theCoreSystem.GetTransactionlessDataCore(), *theCoreSystem.GetNewMetaData(),
                *theCoreSystem.GetTransactionlessSlideCore(),
                *theCoreSystem.GetTransactionlessAnimationCore(),
                *theCoreSystem.GetTransactionlessActionCore(), *m_AssetGraph,
                *theFullSystem.GetSlideSystem(), *theFullSystem.GetActionSystem(),
                *theCoreSystem.GetTransactionlessSlideGraphCore(),
                theClientBridge.GetObjectDefinitions(), m_ImportFailedHandler,
                *theCoreSystem.GetGuideSystem(), *GetSceneGraph()->GetPathManager(),
                *theFullSystem.GetPropertySystem());
}

std::shared_ptr<qt3dsdm::IDOMWriter> CDoc::CreateDOMWriter()
{
    using namespace qt3dsdm;
    qt3ds::QT3DSI32 theFileVersion = UIP_VERSION;
    TStringTablePtr theStringTable(
                m_StudioSystem->GetFullSystem()->GetCoreSystem()->GetDataCore()->GetStringTablePtr());
    std::shared_ptr<IDOMWriter> theWriterPtr(
                IDOMWriter::CreateDOMWriter(L"UIP", theStringTable).first);
    IDOMWriter &theWriter(*theWriterPtr);
    theWriter.Att(L"version", theFileVersion);
    theWriter.Begin(L"Project");
    return theWriterPtr;
}

using namespace qt3ds;
using namespace qt3ds::foundation;

struct SBufferedInputStreamInStream : public qt3ds::foundation::IInStream
{
    CBufferedInputStream &m_Stream;

    SBufferedInputStreamInStream(CBufferedInputStream &str)
        : m_Stream(str)
    {
    }

    QT3DSU32 Read(NVDataRef<QT3DSU8> data) override
    {
        long amountRead = m_Stream.Read(data.begin(), data.size());
        if (amountRead < 0)
            amountRead = 0;
        return static_cast<QT3DSU32>(amountRead);
    }
};

struct SBufferedOutputStreamOutStream : public qt3dsdm::IOutStream
{
    CBufferedOutputStream &m_Stream;

    SBufferedOutputStreamOutStream(CBufferedOutputStream &str)
        : m_Stream(str)
    {
    }

    bool Write(NVConstDataRef<QT3DSU8> data) override
    {
        m_Stream.Write(data.begin(), data.size());
        return true;
    }
};

inline std::shared_ptr<qt3dsdm::IDOMReader>
DoCreateDOMReader(qt3ds::foundation::IInStream &inStream,
                  std::shared_ptr<qt3dsdm::IStringTable> theStringTable,
                  qt3ds::QT3DSI32 &outVersion)
{
    using namespace qt3dsdm;
    std::shared_ptr<IDOMFactory> theFactory(IDOMFactory::CreateDOMFactory(theStringTable));
    SDOMElement *theElement = CDOMSerializer::Read(*theFactory, inStream);
    outVersion = 0;
    if (theElement) {
        std::shared_ptr<IDOMReader> retval =
                IDOMReader::CreateDOMReader(*theElement, theStringTable, theFactory);

        IDOMReader &theReader(*retval);
        theReader.Att("version", outVersion);
        if (outVersion > UIP_VERSION || outVersion < LAST_SUPPORTED_UIP_VERSION)
            return std::shared_ptr<qt3dsdm::IDOMReader>();
        if (!theReader.MoveToFirstChild(L"Project"))
            return std::shared_ptr<qt3dsdm::IDOMReader>();

        return retval;
    }
    return std::shared_ptr<qt3dsdm::IDOMReader>();
}

std::shared_ptr<qt3dsdm::IDOMReader> CDoc::CreateDOMReader(const Q3DStudio::CString &inFilePath,
                                                           qt3ds::QT3DSI32 &outVersion)
{
    using namespace qt3dsdm;

    TStringTablePtr theStringTable(
                m_StudioSystem->GetFullSystem()->GetCoreSystem()->GetDataCore()->GetStringTablePtr());
    CFileSeekableIOStream theStream(inFilePath.GetCharStar(), FileReadFlags());
    if (!theStream.IsOpen())
        return std::shared_ptr<qt3dsdm::IDOMReader>();
    return DoCreateDOMReader(theStream, theStringTable, outVersion);
}

std::shared_ptr<qt3dsdm::IDOMReader> CDoc::CreateDOMReader(CBufferedInputStream &inStream,
                                                           qt3ds::QT3DSI32 &outVersion)
{
    using namespace qt3dsdm;
    TStringTablePtr theStringTable(
                m_StudioSystem->GetFullSystem()->GetCoreSystem()->GetDataCore()->GetStringTablePtr());
    SBufferedInputStreamInStream theStream(inStream);
    return DoCreateDOMReader(theStream, theStringTable, outVersion);
}
using std::pair;
using std::make_pair;
using qt3ds::render::SImageTextureData;

static bool SourcePathImageBufferLessThan(const pair<Q3DStudio::CString, SImageTextureData> &lhs,
                                          const pair<Q3DStudio::CString, SImageTextureData> &rhs)
{
    return lhs.first < rhs.first;
}

struct SBufferFilter
{
    Q3DStudio::TCharPtrToSlideInstanceMap &m_Map;
    qt3dsdm::IStringTable &m_StringTable;
    SBufferFilter(Q3DStudio::TCharPtrToSlideInstanceMap &inMap, qt3dsdm::IStringTable &inStrTable)
        : m_Map(inMap)
        , m_StringTable(inStrTable)
    {
    }

    // We want to filter out items that aren't in the map, so we need to return true
    // if the item is not in the map.
    bool operator()(const std::pair<Q3DStudio::CString, SImageTextureData> &inItem) const
    {
        return m_Map.find(m_StringTable.GetWideStr(inItem.first.c_str())) == m_Map.end();
    }
};

/**
 *	SavePresentationFile: Saves the presentation file.
 *	@param	inArchive	CArchive for saving data
 *	@return true if saved successfully
 */
void CDoc::SavePresentationFile(CBufferedOutputStream *inOutputStream)
{
    using namespace std;
    using namespace qt3dsdm;
    using namespace Q3DStudio;

    std::shared_ptr<IDOMWriter> theWriterPtr(CreateDOMWriter());

    IDOMWriter &theWriter(*theWriterPtr);

    CProjectSettingsSerializer theProjectSettingsSerializer(m_Core->GetStudioProjectSettings());
    theWriter.Serialize(L"ProjectSettings", theProjectSettingsSerializer);
    // Ensure we have a buffer cache.
    GetBufferCache();
    // Ensure all images that can be referenced in any way are loaded into the document buffer
    // cache.
    if (m_SceneEditor && m_DocumentBufferCache) {
        CClientDataModelBridge &theBridge = *(m_StudioSystem->GetClientDataModelBridge());
        IPropertySystem &thePropertySystem = *(m_StudioSystem->GetPropertySystem());
        TCharPtrToSlideInstanceMap sourcePathToInstanceMap;
        m_SceneEditor->GetSourcePathToInstanceMap(sourcePathToInstanceMap, false, false);
        // Ensure the image is loaded in the image buffer system
        // because this scans the image for alpha bits.
        for (TCharPtrToSlideInstanceMap::iterator theIter = sourcePathToInstanceMap.begin(),
             end = sourcePathToInstanceMap.end();
             theIter != end; ++theIter) {
            const TSlideInstanceList &theList = theIter->second;
            if (theList.empty())
                continue;
            bool isImage = false;
            bool isInstance = false;
            for (size_t slideInstanceIdx = 0, slideInstanceEnd = theList.size();
                 slideInstanceIdx < slideInstanceEnd && !isInstance; ++slideInstanceIdx) {
                Qt3DSDMInstanceHandle theInstance = theList[slideInstanceIdx].second;
                isInstance = m_StudioSystem->IsInstance(theInstance);
                if (isInstance)
                    isImage = thePropertySystem.IsInstanceOrDerivedFrom(
                                theInstance, theBridge.GetObjectDefinitions().m_Image.m_Instance);
            }
            if (isImage)
                m_DocumentBufferCache->GetOrCreateImageBuffer(CFilePath(theIter->first));
        }

        auto textureList = theBridge.GetDynamicObjectTextureList();
        for (auto texture : textureList)
            m_DocumentBufferCache->GetOrCreateImageBuffer(CFilePath(texture));

        std::vector<pair<Q3DStudio::CString, SImageTextureData>> theImageBuffers;
        m_DocumentBufferCache->GetImageBuffers(theImageBuffers);

        if (!theImageBuffers.empty()) {
            // Ensure the source paths are always written out in the same order to keep source
            // control reasonable
            std::sort(theImageBuffers.begin(), theImageBuffers.end(),
                      SourcePathImageBufferLessThan);
            IDOMWriter::Scope __BufferData(theWriter, L"BufferData");
            for (size_t idx = 0, end = theImageBuffers.size(); idx < end; ++idx) {
                SImageTextureData theBuffer = theImageBuffers[idx].second;
                if (theBuffer.m_TextureFlags.HasTransparency()) {
                    IDOMWriter::Scope __ImageScope(theWriter, L"ImageBuffer");
                    theWriter.Att(L"sourcepath", theImageBuffers[idx].first.c_str());
                    // Writing boolean in wide text results just in "T" or "F" on Linux
                    theWriter.Att("hasTransparency", true);
                }
            }
        }
    }

    std::shared_ptr<IComposerSerializer> theSerializer(CreateSerializer());
    theSerializer->SerializeScene(theWriter);

    SBufferedOutputStreamOutStream theStream(*inOutputStream);
    CDOMSerializer::WriteXMLHeader(theStream);
    CDOMSerializer::Write(*theWriter.GetTopElement(), theStream);
}

bool CDoc::canSetKeyframeInterpolation() const
{
    if (m_KeyframesManager)
        return m_KeyframesManager->canSetKeyframeInterpolation();

    return false;
}

void CDoc::SetKeyframeInterpolation()
{
    if (m_KeyframesManager)
        m_KeyframesManager->SetKeyframeInterpolation();
}

void CDoc::DeselectAllKeyframes()
{
    if (m_KeyframesManager)
        m_KeyframesManager->DeselectAllKeyframes();
}

/**
 *	Recursive function to get all actions affected by renaming the asset with inObjectId.
 *	I think the logic behind this is that once the parent will affect all children's path
 *reference, and hence all its descendants' action must be checked as well.
 */
void CDoc::GetActionsAffectedByRename(qt3dsdm::Qt3DSDMInstanceHandle inAsset,
                                      std::set<Q3DStudio::CString> &ioActionsAffected)
{
    if (!inAsset)
        return;
    ASSERT(0);
    Q_UNUSED(ioActionsAffected);
    // TODO : UICDMActionCore needs to support relative/absolute reference, and needs to implement
    // something similar to GetAllAffectedByRename
    // refer to CSAction::AffectedByRename on how this was implemented

    // CActionManager::TActionList theActions = m_ActionManager->GetAllAffectedByRename(
    // inAsset->GetAssetID( ) );
    // CActionManager::TActionList::iterator thePos = theActions.begin( );
    // for ( ; thePos != theActions.end( ); ++thePos )
    //	ioActionsAffected.insert( ( *thePos )->GetOwningState( )->GetAsset( )->GetName( ) );
    //
    //// process the children
    // Q3DStudio::CGraphIterator theChildren;
    // GetAssetChildren( this, inAsset, theChildren );
    // for ( ; !theChildren.IsDone( ); ++theChildren )
    //{
    //	CAsset* theChild = static_cast<CAsset*>( theChildren.GetCurrent( ) );
    //	GetActionsAffectedByRename( theChild, ioActionsAffected );
    //}
}

qt3dsdm::Qt3DSDMInstanceHandle CDoc::getPasteTarget(qt3dsdm::Qt3DSDMInstanceHandle selected) const
{
    // Logic for object pasting is:
    // 1) If you can paste the object as a sibling of the selected object -> do that
    //  Except: If selected object is the currently active component, never paste under its parent,
    //          as that would be outside the current time context.
    // 2) If you can paste the object as a child of the selected object -> do that
    //  Except: If the selected object is a component that is not active, never paste under it.
    //          Components can have children only in time context of their own.

    qt3dsdm::Qt3DSDMInstanceHandle pasteTarget;
    if (selected.Valid()) {
        auto bridge = m_StudioSystem->GetClientDataModelBridge();
        qt3dsdm::Qt3DSDMInstanceHandle selectedParent = bridge->GetParentInstance(selected);
        bool componentSelected = bridge->IsComponentInstance(selected);
        bool selectedComponentActive = false;
        if (componentSelected)
            selectedComponentActive = bridge->IsActiveComponent(selected);

        if (!selectedComponentActive && selectedParent.Valid()
                && CStudioClipboard::CanPasteObject(bridge->GetObjectType(selectedParent))) {
            pasteTarget = selectedParent;
        } else if (CStudioClipboard::CanPasteObject(bridge->GetObjectType(selected))
                   && (!componentSelected || selectedComponentActive)) {
            pasteTarget = selected;
        }
    }
    return pasteTarget;
}

/**
 * Image is a property of the material.
 * Recursively iterate the images involved by the object and do either of following:
 * a. schedule remove instance if ( outBatch != NULL )
 * b. list the source GUID of the image if ( outImageIdList != NULL )
 * This is so that ScheduleRemoveImageInstances & GetImageInstancesSourceGUID can share code
 */
void CDoc::IterateImageInstances(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                 std::vector<Q3DStudio::CId> *outImageIdList)
{
    CClientDataModelBridge *theClientBridge = GetStudioSystem()->GetClientDataModelBridge();

    if (theClientBridge->IsSceneInstance(inInstance)
            || theClientBridge->IsComponentInstance(inInstance)) // If this is a Component, return.
        // Iterating Component has been handled
        // by other function such as
        // ScheduleRemoveComponentInstances
        return;

    qt3dsdm::IPropertySystem *thePropertySystem = GetStudioSystem()->GetPropertySystem();
    qt3dsdm::ISlideSystem *theSlideSystem = GetStudioSystem()->GetSlideSystem();

    Q3DStudio::CGraphIterator theSourceChildIterator;
    GetAssetChildren(this, inInstance, theSourceChildIterator);

    for (; !theSourceChildIterator.IsDone(); ++theSourceChildIterator) {
        qt3dsdm::Qt3DSDMInstanceHandle theChildInstance = theSourceChildIterator.GetCurrent();
        if (theClientBridge->IsMaterialInstance(theChildInstance)) {
            qt3dsdm::TPropertyHandleList theProperties;
            thePropertySystem->GetAggregateInstanceProperties(theChildInstance, theProperties);
            size_t thePropertyCount = theProperties.size();
            for (size_t thePropertyIndex = 0; thePropertyIndex < thePropertyCount;
                 ++thePropertyIndex) {
                qt3dsdm::Qt3DSDMPropertyHandle theProperty = theProperties[thePropertyIndex];
                qt3dsdm::AdditionalMetaDataType::Value theAdditionalMetaDataType =
                        thePropertySystem->GetAdditionalMetaDataType(theChildInstance, theProperty);

                if (theAdditionalMetaDataType == qt3dsdm::AdditionalMetaDataType::Image) {
                    qt3dsdm::Qt3DSDMSlideHandle theSlide =
                            theSlideSystem->GetAssociatedSlide(theChildInstance);
                    bool theIsMaster = theSlideSystem->IsMasterSlide(theSlide);
                    if (!theIsMaster
                            || theSlideSystem->IsPropertyLinked(theChildInstance, theProperty)) {
                        qt3dsdm::SValue theValue;
                        thePropertySystem->GetInstancePropertyValue(theChildInstance, theProperty,
                                                                    theValue);
                        qt3dsdm::SLong4 theDataModelImageGuid = qt3dsdm::get<qt3dsdm::SLong4>(theValue);

                        qt3dsdm::Qt3DSDMInstanceHandle theImageInstance =
                                theClientBridge->GetImageInstanceByGUID(theDataModelImageGuid);
                        if (theImageInstance.Valid()) {
                            // Get the image source guid
                            if (outImageIdList) {
                                outImageIdList->push_back(
                                            Q3DStudio::CId(theDataModelImageGuid.m_Longs[0],
                                            theDataModelImageGuid.m_Longs[1],
                                        theDataModelImageGuid.m_Longs[2],
                                        theDataModelImageGuid.m_Longs[3]));
                            }
                        }
                    } else {
                        qt3dsdm::SValue theValue;
                        qt3dsdm::Qt3DSDMSlideHandle theMasterSlide =
                                theClientBridge->GetOrCreateGraphRoot(theChildInstance);

                        size_t theNumSlides = theSlideSystem->GetSlideCount(theMasterSlide);
                        for (size_t theSlideIndex = 0; theSlideIndex < theNumSlides;
                             ++theSlideIndex) {
                            if (theSlideSystem->GetSlidePropertyValue(
                                        theSlideIndex, theChildInstance, theProperty, theValue)) {
                                qt3dsdm::SLong4 theDataModelImageGuid =
                                        qt3dsdm::get<qt3dsdm::SLong4>(theValue);
                                qt3dsdm::Qt3DSDMInstanceHandle theImageInstance =
                                        theClientBridge->GetImageInstanceByGUID(theDataModelImageGuid);
                                if (theImageInstance.Valid()) {
                                    // Get the image source guid
                                    if (outImageIdList) {
                                        outImageIdList->push_back(
                                                    Q3DStudio::CId(theDataModelImageGuid.m_Longs[0],
                                                    theDataModelImageGuid.m_Longs[1],
                                                theDataModelImageGuid.m_Longs[2],
                                                theDataModelImageGuid.m_Longs[3]));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            IterateImageInstances(theChildInstance, outImageIdList);
        }
    }
}

/**
 * @returns the object based on the selection mode
 */
qt3dsdm::Qt3DSDMInstanceHandle CDoc::GetObjectbySelectMode(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                                           bool inGroupMode)
{
    qt3dsdm::Qt3DSDMInstanceHandle theResult = inInstance;

    CClientDataModelBridge *theBridge = m_StudioSystem->GetClientDataModelBridge();
    if (!theBridge->IsInActiveComponent(inInstance)) {
        qt3dsdm::Qt3DSDMInstanceHandle theParent = theBridge->GetParentInstance(inInstance);
        if (theParent.Valid())
            theResult = GetObjectbySelectMode(theParent, inGroupMode);
        else
            theResult = 0;
    } else {
        if (inGroupMode) {
            qt3dsdm::Qt3DSDMInstanceHandle theGroup = GetTopmostGroup(inInstance);
            if (theGroup.Valid())
                theResult = theGroup;
        }
    }
    return theResult;
}

/**
 * @returns the topmost group object that owns this instance
 */
qt3dsdm::Qt3DSDMInstanceHandle CDoc::GetTopmostGroup(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    CClientDataModelBridge *theBridge = m_StudioSystem->GetClientDataModelBridge();

    if (theBridge->IsGroupInstance(inInstance)) {
        qt3dsdm::Qt3DSDMInstanceHandle theGroup = inInstance;

        // If this group has a parent
        qt3dsdm::Qt3DSDMInstanceHandle theParent = theBridge->GetParentInstance(inInstance);
        if (theParent.Valid()) {
            qt3dsdm::Qt3DSDMInstanceHandle theParentGroup;
            // Get the group object of the parent
            if (GetActiveRootInstance()
                    == inInstance) { // check if this component is the active time context now
                theGroup = 0;
            } else // all other cases, try to get parent's topmost group, if any
            {
                theParentGroup = GetTopmostGroup(theParent);
            }

            // If another group was found higher up the chain, then it is the topmost group
            if (theParentGroup)
                theGroup = theParentGroup;
        }

        return theGroup;
    } else {
        qt3dsdm::Qt3DSDMInstanceHandle theParent = theBridge->GetParentInstance(inInstance);
        if (theParent.Valid())
            return GetTopmostGroup(theParent);
        else
            return 0;
    }
}

/**
 * Image is a property of the material, find out all images involved by the delete
 * object and scehedule a remove instances.
 */
void CDoc::ScheduleRemoveImageInstances(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                        CCmdBatch * /*outBatch*/)
{
    IterateImageInstances(inInstance, nullptr);
}

/**
 * Remove all DataModel instances and schedule a delete
 */
void CDoc::ScheduleRemoveDataModelInstances(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                            CCmdBatch *outBatch)
{
    // remove my children
    Q3DStudio::CGraphIterator theSourceChildIterator;
    GetAssetChildren(this, inInstance, theSourceChildIterator);
    for (; !theSourceChildIterator.IsDone(); ++theSourceChildIterator)
        ScheduleRemoveDataModelInstances(theSourceChildIterator.GetCurrent(), outBatch);

    // Remove myself
    CCmd *theCmd = new CCmdDataModelDeleteInstance(this, inInstance);
    outBatch->AddCommand(theCmd);
}

/**
 * inObject may be a Component or it may have children Component, find out all Components involved
 * by the delete
 * object and scehedule a remove instances.
 */
void CDoc::ScheduleRemoveComponentInstances(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                            CCmdBatch *outBatch)
{
    CClientDataModelBridge *theClientBridge = GetStudioSystem()->GetClientDataModelBridge();
    if (theClientBridge->IsComponentInstance(inInstance)) {
        Q3DStudio::CId theInstanceId = theClientBridge->GetGUID(inInstance);
        qt3dsdm::Qt3DSDMInstanceHandle theInstance =
                theClientBridge->GetComponentInstanceByGUID(theInstanceId);
        CCmd *theCmd = new CCmdDataModelDeleteComponentInstance(this, theInstance);
        outBatch->AddCommand(theCmd);
    }
    // Recursively find the Component
    Q3DStudio::CGraphIterator theSourceChildIterator;
    GetAssetChildren(this, inInstance, theSourceChildIterator);
    for (; !theSourceChildIterator.IsDone(); ++theSourceChildIterator)
        ScheduleRemoveComponentInstances(theSourceChildIterator.GetCurrent(), outBatch);
}

void CDoc::SetDefaultKeyframeInterpolation(bool inSmooth)
{
    CStudioPreferences::SetInterpolation(inSmooth);
    GetStudioSystem()->GetAnimationSystem()->SetInterpolation(inSmooth);
}

void CDoc::AddToGraph(qt3dsdm::Qt3DSDMInstanceHandle inParentInstance,
                      qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (!inInstance.Valid())
        return;

    ASSERT(!m_AssetGraph->IsExist(inInstance));
    m_AssetGraph->AddChild(inParentInstance, inInstance);
}

void CDoc::OnNewPresentation()
{
    m_materialRenames.clear();
    m_materialUndoRenames.clear();

    m_PlaybackClock->Reset();
    m_Core->GetDispatch()->FireOnNewPresentation();

    // true to refresh
    NotifyActiveSlideChanged(
                m_StudioSystem->GetClientDataModelBridge()->GetComponentActiveSlide(
                    m_SceneInstance), true);

    // Boot up the document editor so we can listen to file system changes
    // and get master behaviors updated.
    GetDocumentReader();
    SetupDataCoreSignals();
}

void CDoc::OnPresentationDeactivated()
{
}

/**
 * Gets the list of standard and custom materials used in the scene.
 *
 * @param inParent search root
 * @param outMats list of scene materials
 */
void CDoc::getSceneMaterials(qt3dsdm::Qt3DSDMInstanceHandle inParent,
                             QVector<qt3dsdm::Qt3DSDMInstanceHandle> &outMats) const
{
    const CClientDataModelBridge *bridge = m_StudioSystem->GetClientDataModelBridge();
    for (long i = 0, count = m_AssetGraph->GetChildCount(inParent); i < count; ++i) {
        qt3dsdm::Qt3DSDMInstanceHandle theChild(m_AssetGraph->GetChild(inParent, i));
        if (!bridge->isMaterialContainer(theChild) && !bridge->isInsideMaterialContainer(theChild)
                && (bridge->IsMaterialInstance(theChild)
                    || bridge->IsCustomMaterialInstance(theChild))) {
            outMats.push_back(theChild);
        }

        getSceneMaterials(theChild, outMats);
    }
}

void CDoc::getSceneReferencedMaterials(qt3dsdm::Qt3DSDMInstanceHandle inParent,
                                       QVector<qt3dsdm::Qt3DSDMInstanceHandle> &outMats) const
{
    const CClientDataModelBridge *bridge = m_StudioSystem->GetClientDataModelBridge();
    for (long i = 0, count = m_AssetGraph->GetChildCount(inParent); i < count; ++i) {
        qt3dsdm::Qt3DSDMInstanceHandle theChild(m_AssetGraph->GetChild(inParent, i));
        if (bridge->IsReferencedMaterialInstance(theChild))
            outMats.push_back(theChild);

        getSceneReferencedMaterials(theChild, outMats);
    }
}

void CDoc::getUsedSharedMaterials(QVector<qt3dsdm::Qt3DSDMInstanceHandle> &outMats) const
{
    QVector<qt3dsdm::Qt3DSDMInstanceHandle> refMats;
    getSceneReferencedMaterials(GetSceneInstance(), refMats);

    CClientDataModelBridge *bridge = m_StudioSystem->GetClientDataModelBridge();
    for (auto &refMat : qAsConst(refMats)) {
        qt3dsdm::Qt3DSDMInstanceHandle original = bridge->getMaterialReference(refMat);
        if (original.Valid())
            outMats.append(original);
    }
}

void CDoc::CheckActionDependencies(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    Q3DStudio::CString theListOfTargets;
    GetActionDependencies(inInstance, theListOfTargets);

    if (!theListOfTargets.IsEmpty()) {
        if (m_DeletingReferencedObjectHandler)
            m_DeletingReferencedObjectHandler->DisplayMessageBox(theListOfTargets.toQString());
    }
}

// In outMap, returns datainput names found from element control
// bindings but which are missing from (UIP) datainput list.
void CDoc::UpdateDatainputMap(QMultiMap<QString,
                              QPair<qt3dsdm::Qt3DSDMInstanceHandle,
                                    qt3dsdm::Qt3DSDMPropertyHandle>> *outMap)
{
    for (auto &it : qAsConst(g_StudioApp.m_dataInputDialogItems))
        it->ctrldElems.clear();

    UpdateDatainputMapRecursive(GetSceneInstance(), outMap);
}

// Update global datainput map for all datainput bindings for a single instance.
void CDoc::UpdateDatainputMapForInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    auto propSystem = GetPropertySystem();

    qt3dsdm::Qt3DSDMPropertyHandle ctrldPropHandle
            = propSystem->GetAggregateInstancePropertyByName(inInstance, L"controlledproperty");

    if (propSystem->HasAggregateInstanceProperty(inInstance, ctrldPropHandle)) {
        qt3dsdm::SValue ctrldPropVal;
        propSystem->GetInstancePropertyValue(inInstance, ctrldPropHandle, ctrldPropVal);
        Q3DStudio::CString currCtrldPropsStr
                = qt3dsdm::get<qt3dsdm::TDataStrPtr>(ctrldPropVal)->GetData();
        QStringList splitStr = currCtrldPropsStr.toQString().split(QLatin1Char(' '));

        // There is no way to detect a case where control is removed i.e. a controller-property
        // pair simply disappears from controlledproperty string. We need to do a complete
        // rebuild for inInstance references in the global map, but still avoid doing
        // a scene-wide datainput map update.
        for (auto &it : qAsConst(g_StudioApp.m_dataInputDialogItems))
            it->removeControlFromInstance(inInstance);

        // Rebuild controlled element items and append them to global datainput map.
        for (int i = 0; i < splitStr.size() - 1; i += 2) {
            // Check for '$' because Qt3DS v1.1 did not differentiate datainput
            // names with it
            QString diName = splitStr[i].startsWith(QLatin1Char('$'))
                    ? splitStr[i].remove(0, 1) : splitStr[i];
            QString propName = splitStr[i+1];
            auto propHandle = propSystem->GetAggregateInstancePropertyByName(
                        inInstance, propName.toStdWString().c_str());
            auto propType = propSystem->GetDataType(propHandle);
            CDataInputDialogItem::ControlledItem item(inInstance, propHandle);
            if (propType)
                item.dataType = {propType, false};
            else if (propName == QLatin1String("@slide"))
                item.dataType = {qt3dsdm::DataModelDataType::Value::String, true};
            else if (propName == QLatin1String("@timeline"))
                item.dataType = {qt3dsdm::DataModelDataType::Value::RangedNumber, true};

            // Check for DI name validity because we might have broken
            // presentations with property control bindings set to non-existent
            // datainputs
            if (g_StudioApp.m_dataInputDialogItems.contains(diName))
                g_StudioApp.m_dataInputDialogItems[diName]->ctrldElems.append(item);
        }
    }
}

void CDoc::UpdateDatainputMapRecursive(
        const qt3dsdm::Qt3DSDMInstanceHandle inInstance,
        QMultiMap<QString,
                  QPair<qt3dsdm::Qt3DSDMInstanceHandle, qt3dsdm::Qt3DSDMPropertyHandle>> *outMap)
{
    auto propSystem = GetPropertySystem();

    qt3dsdm::Qt3DSDMPropertyHandle ctrldPropHandle
        = propSystem->GetAggregateInstancePropertyByName(inInstance, L"controlledproperty");

    if (propSystem->HasAggregateInstanceProperty(inInstance, ctrldPropHandle)) {
        qt3dsdm::SValue ctrldPropVal;
        propSystem->GetInstancePropertyValue(inInstance, ctrldPropHandle, ctrldPropVal);
        Q3DStudio::CString currCtrldPropsStr
            = qt3dsdm::get<qt3dsdm::TDataStrPtr>(ctrldPropVal)->GetData();
        QStringList splitStr = currCtrldPropsStr.toQString().split(' ');
        for (int i = 0; i < splitStr.size() - 1; i += 2) {
            QString diName = splitStr[i].startsWith('$') ? splitStr[i].remove(0, 1) : splitStr[i];
            QString propName = splitStr[i+1];

            auto propHandle = propSystem->GetAggregateInstancePropertyByName(
                        inInstance, propName.toStdWString().c_str());
            auto propType = propSystem->GetDataType(propHandle);
            // Update the controlled elements and property types for
            // verified, existing datainputs. Note that for @timeline and
            // @slide controllers the property type is not found as these
            // are pseudo-properties, so we handle them here.
            // For slide control, type is strictly set to String.
            // For timeline the datainput is strictly Ranged Number only.
            if (g_StudioApp.m_dataInputDialogItems.contains(diName)) {
                CDataInputDialogItem::ControlledItem item(inInstance, propHandle);
                if (propType)
                    item.dataType = {propType, false};
                else if (propName == QLatin1String("@slide"))
                    item.dataType = {qt3dsdm::DataModelDataType::Value::String, true};
                else if (propName == QLatin1String("@timeline"))
                    item.dataType = {qt3dsdm::DataModelDataType::Value::RangedNumber, true};

                g_StudioApp.m_dataInputDialogItems[diName]->ctrldElems.append(item);
            } else if (outMap != nullptr) {
                // Do multi insert as single datainput name can
                // be found in several elements.
                qt3dsdm::Qt3DSDMPropertyHandle prop
                        = propSystem->GetAggregateInstancePropertyByName(
                            inInstance, splitStr[i+1].toStdWString().c_str());
                QPair<qt3dsdm::Qt3DSDMInstanceHandle,
                      qt3dsdm::Qt3DSDMPropertyHandle> valuepair(inInstance, prop);
                outMap->insertMulti(diName, valuepair);
            }
        }
    }

    // recurse through the tree
    Q3DStudio::CGraphIterator iter;
    GetAssetChildren(this, inInstance, iter);
    for (; !iter.IsDone(); ++iter)
        UpdateDatainputMapRecursive(iter.GetCurrent(), outMap);
}

// Sanity checks controlledproperty strings to see if controller names
// and target properties are valid. Removes invalid controller - property
// pairs from controlledproperty string. Transaction/undo points are not created
// for invalid pair deletions, so caller is responsible for dispatching datamodel
// notifications.
// Recurses through the entire tree and returns true only if all strings were valid.
bool CDoc::VerifyControlledProperties(const qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    auto propSystem = GetPropertySystem();
    bool ret = true;

    qt3dsdm::Qt3DSDMPropertyHandle ctrldPropHandle
        = propSystem->GetAggregateInstancePropertyByName(inInstance, L"controlledproperty");
    // Split controlledproperty string to parts and check each for validity.
    if (ctrldPropHandle) {
        qt3dsdm::SValue ctrldPropVal;
        propSystem->GetInstancePropertyValue(inInstance, ctrldPropHandle, ctrldPropVal);
        Q3DStudio::CString currCtrldPropsStr
                = qt3dsdm::get<qt3dsdm::TDataStrPtr>(ctrldPropVal)->GetData();
        QStringList splitStr = currCtrldPropsStr.toQString().split(' ');

        Q3DStudio::CString validatedStr;

        QRegExpValidator rxp(QRegExp("[A-Za-z0-9_$]+"));

        for (int i = 0; i < splitStr.size() - 1; i += 2) {
            // Not much to do to validate datainput name except to check that it has no illegal
            // characters. We will check elsewhere that datainput names correspond to ones
            // that are defined in the global UIA file.
            int pos;
            auto diValidationRes = rxp.validate(splitStr[i], pos);

            bool targetValid = false;
            // check that target property exists or the target is @slide or @timeline
            qt3dsdm::Qt3DSDMPropertyHandle targetPropHandle
                    = propSystem->GetAggregateInstancePropertyByName(
                        inInstance, splitStr[i+1].toStdWString().c_str());
            if (targetPropHandle
                || splitStr[i+1] == QLatin1String("@timeline")
                || splitStr[i+1] == QLatin1String("@slide")) {
                targetValid = true;
            }

            // if either controller or property is invalid or an empty string,
            // do not insert pair into validated string
            if (diValidationRes != QValidator::Invalid && targetValid
                && splitStr[i].size() && splitStr[i] != QLatin1String("$")) {
                // only add spacer if not at the first entry
                if (validatedStr.size())
                    validatedStr.append(" ");
                validatedStr.append(Q3DStudio::CString::fromQString(splitStr[i]) + " "
                                    + Q3DStudio::CString::fromQString(splitStr[i+1]));
            } else {
                ret = false;
            }
        }

        // If we had to remove invalid entries for this object, write out the
        // changed controlproperties string
        if (!ret) {
            Q3DStudio::SValue controlledProperty
                    = std::make_shared<qt3dsdm::CDataStr>(validatedStr);
            // Set changed controlledproperty properties directly without creating
            // transaction and undo points
            SetInstancePropertyValue(inInstance,  L"controlledproperty", controlledProperty);
        }
    }

    // Recurse through the tree.
    // If one or more objects fails to validate, return false for the entire recursive
    // call stack.
    Q3DStudio::CGraphIterator iter;
    GetAssetChildren(this, inInstance, iter);
    for (; !iter.IsDone(); ++iter) {
        auto res = VerifyControlledProperties(iter.GetCurrent());
        ret = res && ret;
    }

    return ret;
}

// Replaces datainput bindings in list given by "instances".
// Opens up a single transaction that batches all binding changes.
// NOTE: calling function is responsible for closing the transaction.
void CDoc::ReplaceDatainput(const QString &oldName, const QString &newName,
                            const QList<qt3dsdm::Qt3DSDMInstanceHandle> &instances)
{
    auto propSystem = GetPropertySystem();

    // Open a single transaction for all datainput replaces, so that all binding replacements
    // done from within f.ex datainput management dialog can be undone with a single undo.
    if (!isTransactionOpened())
        OpenTransaction(QObject::tr("Replace datainput bindings"), __FILE__, __LINE__);

    for (auto it : instances) {
        qt3dsdm::Qt3DSDMPropertyHandle ctrldPropHandle
                = propSystem->GetAggregateInstancePropertyByName(it.GetHandleValue(),
                                                                 L"controlledproperty");
        QString newStr;
        bool renamed = false;
        if (ctrldPropHandle && it.Valid()) {
            qt3dsdm::SValue ctrldPropVal;
            propSystem->GetInstancePropertyValue(it.GetHandleValue(), ctrldPropHandle,
                                                 ctrldPropVal);
            Q3DStudio::CString currCtrldPropsStr
                    = qt3dsdm::get<qt3dsdm::TDataStrPtr>(ctrldPropVal)->GetData();
            QStringList splitStr = currCtrldPropsStr.toQString().split(QLatin1Char(' '));

            for (int i = 0; i < splitStr.size() - 1; i += 2) {
                if (splitStr[i].contains(oldName)) {
                    splitStr[i] = QLatin1Char('$') + newName;
                    renamed = true;
                }
                newStr.append(QStringLiteral(" ") + splitStr[i] + QStringLiteral(" ")
                              + splitStr[i+1]);
            }
        }
        // Make changes to property.
        if (renamed) {
            newStr = newStr.trimmed();
            Q3DStudio::SValue controlledProperty
                    = std::make_shared<qt3dsdm::CDataStr>(Q3DStudio::CString::fromQString(newStr));
            SetInstancePropertyValue(it.GetHandleValue(), L"controlledproperty",
                                     controlledProperty);
        }
    }
}

QString CDoc::GetCurrentController(qt3dsdm::Qt3DSDMInstanceHandle instHandle,
                                   qt3dsdm::Qt3DSDMPropertyHandle propHandle)
{
    auto propSys = GetPropertySystem();
    qt3dsdm::SValue currPropVal;
    propSys->GetInstancePropertyValue(
                instHandle,
                propSys->GetAggregateInstancePropertyByName(
                    instHandle, qt3dsdm::TCharStr(L"controlledproperty")),
                currPropVal);
    if (!currPropVal.empty()) {
        Q3DStudio::CString currPropValStr
                = qt3dsdm::get<qt3dsdm::TDataStrPtr>(currPropVal)->GetData();

        Q3DStudio::CString propName = propSys->GetName(propHandle).c_str();

        // Datainput controller name is always prepended with "$". Differentiate
        // between datainput and property that has the same name by searching specifically
        // for whitespace followed by property name.
        long propNamePos = currPropValStr.find(" " + propName);
        if ((propNamePos != currPropValStr.ENDOFSTRING) && (propNamePos != 0)) {
            long posCtrlr = currPropValStr.substr(0, propNamePos).ReverseFind("$");

            // adjust pos if this is the first controller - property pair
            // in controlledproperty
            if (posCtrlr < 0)
                posCtrlr = 0;

            // remove $
            posCtrlr++;
            return currPropValStr.substr(posCtrlr, propNamePos - posCtrlr).toQString();
        } else {
            return {};
        }
    }

    return {};
}

QDebug operator<<(QDebug dbg, const SubPresentationRecord &r)
{
    QDebugStateSaver stateSaver(dbg);
    dbg.nospace() << r.m_type << " " << r.m_id << " " << r.m_argsOrSrc;
    return dbg;
}

void CDoc::setPlayBackPreviewState(bool state)
{
    m_playbackPreviewOn = state;
}

bool CDoc::isPlayBackPreviewOn() const
{
    return m_playbackPreviewOn;
}

int CDataInputDialogItem::countOfInstance(const qt3dsdm::Qt3DSDMInstanceHandle handle) const
{
    int count = 0;
    for (auto &it : qAsConst(ctrldElems)) {
        if (it.instHandle == handle)
            count++;
    }
    return count;
}

void CDataInputDialogItem::getBoundTypes(
        QVector<QPair<qt3dsdm::DataModelDataType::Value, bool>> &outVec) const
{
    for (auto &it : qAsConst(ctrldElems))
        outVec.append(it.dataType);
}

void CDataInputDialogItem::getInstCtrldItems(const qt3dsdm::Qt3DSDMInstanceHandle handle,
                                             QVector<ControlledItem> &outVec) const
{
    for (auto &it : qAsConst(ctrldElems)) {
        if (it.instHandle == handle)
            outVec.append(it);
    }
}

void CDataInputDialogItem::removeControlFromInstance(const qt3dsdm::Qt3DSDMInstanceHandle handle)
{
    for (auto it = ctrldElems.begin(); it != ctrldElems.end();) {
      if (it->instHandle == handle)
        it = ctrldElems.erase(it);
      else
        ++it;
    }
}
