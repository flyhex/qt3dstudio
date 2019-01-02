/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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

#include "Qt3DSCommonPrecompile.h"
#include "Dispatch.h"
#include "DispatchListeners.h"
#include "Cmd.h"
#include "SelectedValueImpl.h"

class SDispatchSignalSystem : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void selectionChanged(Q3DStudio::SSelectedValue);
};

//=============================================================================
/**
 * Constructor for a CDispatch object.
 */
CDispatch::CDispatch()
    : m_DataModelBeginDepth(0)
    , m_SignalSystem(std::make_shared<SDispatchSignalSystem>())
{
}

//==============================================================================
/**
 *	Releases the object.
 */
CDispatch::~CDispatch()
{
}

void CDispatch::AddDataModelListener(IDataModelListener *inListener)
{
    m_DataModelListeners.AddListener(inListener);
}
void CDispatch::RemoveDataModelListener(IDataModelListener *inListener)
{
    m_DataModelListeners.RemoveListener(inListener);
}
// These are counted, so you can begin,begin,end,end and you will
// get overall just one begin and one end event sent.
// This is specifically to allow batching commands wrap their inner commands
// with the begin/end notification set.
void CDispatch::FireBeginDataModelNotifications()
{
    ++m_DataModelBeginDepth;
    if (m_DataModelBeginDepth == 1)
        m_DataModelListeners.FireEvent(&IDataModelListener::OnBeginDataModelNotifications);
}

bool CDispatch::AreDataModificationsHappening() const
{
    return m_DataModelBeginDepth > 0;
}

void CDispatch::FireEndDataModelNotifications()
{
    if (m_DataModelBeginDepth)
        --m_DataModelBeginDepth;
    if (!m_DataModelBeginDepth) {
        // if there is already an exception in the stack, we should not call the below function,
        // as there is a high chance that there is going to be another exception
        if (!std::uncaught_exception()) {
            m_DataModelListeners.FireEvent(&IDataModelListener::OnEndDataModelNotifications);
        }
    }
}

void CDispatch::FireImmediateRefreshInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    m_DataModelListeners.FireEvent(&IDataModelListener::OnImmediateRefreshInstanceSingle,
                                   inInstance);
}

void CDispatch::FireImmediateRefreshInstance(qt3dsdm::Qt3DSDMInstanceHandle *inInstances,
                                             long inInstanceCount)
{
    m_DataModelListeners.FireEvent(&IDataModelListener::OnImmediateRefreshInstanceMultiple,
                                   inInstances, inInstanceCount);
}

void CDispatch::AddReloadListener(IReloadListener *inListener)
{
    m_ReloadListeners.AddListener(inListener);
}

void CDispatch::RemoveReloadListener(IReloadListener *inListener)
{
    m_ReloadListeners.RemoveListener(inListener);
}

void CDispatch::FireReloadEffectInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    m_ReloadListeners.FireEvent(&IReloadListener::OnReloadEffectInstance, inInstance);
}

void CDispatch::AddDocumentBufferCacheListener(IDocumentBufferCacheListener *inListener)
{
    m_DocumentBufferCacheListeners.AddListener(inListener);
}
void CDispatch::RemoveDocumentBufferCacheListener(IDocumentBufferCacheListener *inListener)
{
    m_DocumentBufferCacheListeners.RemoveListener(inListener);
}
void CDispatch::FireDocumentBufferCacheInvalidated(const QString &inCache)
{
    m_DocumentBufferCacheListeners.FireEvent(&IDocumentBufferCacheListener::OnBufferInvalidated,
                                             inCache);
}

//==============================================================================
/**
 *	Registers a listener for drag operations on the scene.  This function removes
 *	the listener from the list if it is already there before adding it again.
 *	This prevents multiple instances of the same listener from registering for
 *	the event.
 *	@param inListener The listener to be registered.
 */
void CDispatch::AddSceneDragListener(CSceneDragListener *inListener)
{
    m_SceneDragListeners.AddListener(inListener);
}

//==============================================================================
/**
 *	Removes all instances of the specified listener from the registration list.
 *	@param inListener The listener to be unregistered
 */
void CDispatch::RemoveSceneDragListener(CSceneDragListener *inListener)
{
    m_SceneDragListeners.RemoveListener(inListener);
}

//==============================================================================
/**
 *	Fires a scene mouse down event to interested listeners.
 *	@param inPoint The mouse postion at the time of the event, in client coordinates.
 */
void CDispatch::FireSceneMouseDown(SceneDragSenderType::Enum inSender, const QPoint &inPoint, int inToolMode)
{
    m_SceneDragListeners.FireEvent(&CSceneDragListener::OnSceneMouseDown, inSender, inPoint,
                                   inToolMode);
}

void CDispatch::FireSceneMouseUp(SceneDragSenderType::Enum inSender)
{
    m_SceneDragListeners.FireEvent(&CSceneDragListener::OnSceneMouseUp, inSender);
}

//==============================================================================
/**
 *	Fires a scene mouse move event to interested listeners.
 *	@param inPoint The mouse's current position in client coordinates.
 *	@param inToolMode The current studio toolmode, minus the select mode.
 *	@param inFlags The mouse flags (see MSDN WM_MOUSEMOVE).
 */
void CDispatch::FireSceneMouseDrag(SceneDragSenderType::Enum inSender, const QPoint &inPoint, int inToolMode,
                                   int inFlags)
{
    m_SceneDragListeners.FireEvent(&CSceneDragListener::OnSceneMouseDrag, inSender, inPoint,
                                   inToolMode, inFlags);
}

void CDispatch::FireSceneMouseMove(SceneDragSenderType::Enum inSender, const QPoint &inPoint)
{
    m_SceneDragListeners.FireEvent(&CSceneDragListener::OnSceneMouseMove, inSender, inPoint);
}

void CDispatch::FireSceneMouseDblClick(SceneDragSenderType::Enum inSender, const QPoint &inPoint)
{
    m_SceneDragListeners.FireEvent(&CSceneDragListener::OnSceneMouseDblClick, inSender, inPoint);
}

//==============================================================================
/**
 *	Fires a scene mouse wheel event to interested listeners.
 *	@param inDelta the distance rotated expressed in multiples or divisions of WHEEL_DELTA
 *	@param inToolMode The current studio toolmode.
 */
void CDispatch::FireSceneMouseWheel(SceneDragSenderType::Enum inSender, short inDelta,
                                    int inToolMode)
{
    m_SceneDragListeners.FireEvent(&CSceneDragListener::OnSceneMouseWheel, inSender, inDelta,
                                   inToolMode);
}

qt3dsdm::TSignalConnectionPtr
CDispatch::ConnectSelectionChange(std::function<void(Q3DStudio::SSelectedValue)> inCallback)
{
    return std::make_shared<qt3dsdm::QtSignalConnection>(
                QObject::connect(m_SignalSystem.get(),
                                 &SDispatchSignalSystem::selectionChanged,inCallback));
}

void CDispatch::FireSelectionChange(Q3DStudio::SSelectedValue inValue)
{
    Q_EMIT m_SignalSystem->selectionChanged(inValue);
}

//=============================================================================
/**
 * Add a listener to be notified of presentation changes.
 * @param inListener the listener to be added.
 */
void CDispatch::AddPresentationChangeListener(CPresentationChangeListener *inListener)
{
    m_PresentationChangeListeners.AddListener(inListener);
}

//=============================================================================
/**
 * Remove a listener from the list of listeners to be notified of presentation changes.
 * @param inListener the listener to be removed.
 */
void CDispatch::RemovePresentationChangeListener(CPresentationChangeListener *inListener)
{
    m_PresentationChangeListeners.RemoveListener(inListener);
}

//=============================================================================
/**
 * Notify all the presentation change listeners that a presentation or a subpresentation is loading
 * To be exact, this is fired just before the presentation is to be loaded
 */
void CDispatch::FireOnLoadingPresentation(bool inIsSubPresentation /*= false */)
{
    m_PresentationChangeListeners.FireEvent(&CPresentationChangeListener::OnLoadingPresentation,
                                            inIsSubPresentation);
}

/**
 * Notify all the presentation change listeners that a new presentation was loaded.
 */
void CDispatch::FireOnNewPresentation()
{
    m_PresentationChangeListeners.FireEvent(&CPresentationChangeListener::OnNewPresentation);
}

//=============================================================================
/**
 * Notify all the presentation change listeners that the current presentation is closing.
 */
void CDispatch::FireOnClosingPresentation()
{
    m_PresentationChangeListeners.FireEvent(&CPresentationChangeListener::OnClosingPresentation);
}

void CDispatch::FireOnSavingPresentation(const QString &inNewPresentationFile)
{
    m_PresentationChangeListeners.FireEvent(&CPresentationChangeListener::OnSavingPresentation,
                                            inNewPresentationFile);
}

void CDispatch::FireOnExportingAsset(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    m_PresentationChangeListeners.FireEvent(&CPresentationChangeListener::OnExportingAsset,
                                            inInstance);
}

void CDispatch::FireOnPresentationModifiedExternally()
{
    m_PresentationChangeListeners.FireEvent(
        &CPresentationChangeListener::OnPresentationModifiedExternally);
}

void CDispatch::AddClientPlayChangeListener(CClientPlayChangeListener *inListener)
{
    m_ClientPlayChangeListeners.AddListener(inListener);
}

void CDispatch::RemoveClientPlayChangeListener(CClientPlayChangeListener *inListener)
{
    m_ClientPlayChangeListeners.RemoveListener(inListener);
}

void CDispatch::FireOnPlayStart()
{
    m_ClientPlayChangeListeners.FireEvent(&CClientPlayChangeListener::OnPlayStart);
}

void CDispatch::FireOnPlayStop()
{
    m_ClientPlayChangeListeners.FireEvent(&CClientPlayChangeListener::OnPlayStop);
}

void CDispatch::FireOnTimeChanged(long inTime)
{
    try {
        m_ClientPlayChangeListeners.FireEvent(&CClientPlayChangeListener::OnTimeChanged, inTime);
    }
    // Temporary fix for the stuck playhead problem on a crash
    // this should be fixed by addressing the actual crash
    catch (...) {
    }
}

//=============================================================================
/**
 * Add a navigation listener to this.
 */
void CDispatch::AddNavigationListener(CNavigationListener *inListener)
{
    m_NavigationListeners.AddListener(inListener);
}

//=============================================================================
/**
 * Remove a navigation listener from this.
 */
void CDispatch::RemoveNavigationListener(CNavigationListener *inListener)
{
    m_NavigationListeners.RemoveListener(inListener);
}

//=============================================================================
/**
 * Fire an event to the navigation listeners to navigate to the URL.
 * This is used to update the assistant palette to the specified URL.
 */
void CDispatch::FireOnNavigate(const QString &inURL, bool inForceDisplay)
{
    m_NavigationListeners.FireEvent(&CNavigationListener::OnNavigate, inURL, inForceDisplay);
}

//=============================================================================
/**
 * Add a file open listener to this.
 */
void CDispatch::AddFileOpenListener(CFileOpenListener *inListener)
{
    m_FileOpenListeners.AddListener(inListener);
}

//=============================================================================
/**
 * Remove a file open listener from this.
 */
void CDispatch::RemoveFileOpenListener(CFileOpenListener *inListener)
{
    m_FileOpenListeners.RemoveListener(inListener);
}

//=============================================================================
/**
 * Fire an event to all the file open listeners that a has been opened.
 */
void CDispatch::FireOnOpenDocument(const QString &inFile, bool inSucceeded)
{
    m_FileOpenListeners.FireEvent(&CFileOpenListener::OnOpenDocument, inFile, inSucceeded);
}

//=============================================================================
/**
 * Fire an event to all the file open listeners that a has been saved.
 */
void CDispatch::FireOnSaveDocument(const QString &inFile, bool inSucceeded, bool inSaveCopy)
{
    m_FileOpenListeners.FireEvent(&CFileOpenListener::OnSaveDocument, inFile, inSucceeded,
                                  inSaveCopy);
}

//=============================================================================
/**
 * Fire an event to all the file open listeners that a has been saved.
 */
void CDispatch::FireOnDocumentPathChanged(const QString &inNewPath)
{
    m_FileOpenListeners.FireEvent(&CFileOpenListener::OnDocumentPathChanged, inNewPath);
}

//=============================================================================
/**
 * Adds a toolbar change listener
 */
void CDispatch::AddToolbarChangeListener(CToolbarChangeListener *inListener)
{
    m_ToolbarChangeListeners.AddListener(inListener);
}

//=============================================================================
/**
 * Removes a toolbar change listener
 */
void CDispatch::RemoveToolbarChangeListener(CToolbarChangeListener *inListener)
{
    m_ToolbarChangeListeners.RemoveListener(inListener);
}

//=============================================================================
/**
 * Fire an event to all the toolbar change listeners
 */
void CDispatch::FireOnToolbarChange() // long inToolType, bool inEnabled )
{
    m_ToolbarChangeListeners.FireEvent(&CToolbarChangeListener::OnToolbarChange);
}

//=============================================================================
/**
 * Adds a selected node property change listener
 */
void CDispatch::AddSelectedNodePropChangeListener(CSelectedNodePropChangeListener *inListener)
{
    m_SelectedNodePropChangeListeners.AddListener(inListener);
}

//=============================================================================
/**
 * Removes a selected node property change listener
 */
void CDispatch::RemoveSelectedNodePropChangeListener(CSelectedNodePropChangeListener *inListener)
{
    m_SelectedNodePropChangeListeners.RemoveListener(inListener);
}

//=============================================================================
/**
 * Fire an event to all the selected node property change listeners
 */
void CDispatch::FireOnSelectedNodePropChange(const QString &inPropertyName)
{
    m_SelectedNodePropChangeListeners.FireEvent(
        &CSelectedNodePropChangeListener::OnPropValueChanged, inPropertyName);
}

//=============================================================================
/**
 * Fire an event to all the selected node property change listeners
 */
void CDispatch::FireOnMouseDragged(bool inConstrainXAxis, bool inConstrainYAxis)
{
    m_SelectedNodePropChangeListeners.FireEvent(&CSelectedNodePropChangeListener::OnMouseDragged,
                                                inConstrainXAxis, inConstrainYAxis);
}

//=============================================================================
/**
 * Adds a edit camera change listener
 */
void CDispatch::AddEditCameraChangeListener(CEditCameraChangeListener *inListener)
{
    m_EditCameraChangeListeners.AddListener(inListener);
}

//=============================================================================
/**
 * Removes a edit camera change listener
 */
void CDispatch::RemoveEditCameraChangeListener(CEditCameraChangeListener *inListener)
{
    m_EditCameraChangeListeners.RemoveListener(inListener);
}

//=============================================================================
/**
 * Fire an event on changing active camera to all the render mode change listeners
 */
void CDispatch::FireOnEditCameraChanged()
{
    m_EditCameraChangeListeners.FireEvent(&CEditCameraChangeListener::onEditCameraChanged);
}

//=============================================================================
/**
 * Fire an event on changing active camera to all the render mode change listeners
 */
void CDispatch::FireOnCamerasTransformed()
{
    m_EditCameraChangeListeners.FireEvent(&CEditCameraChangeListener::onEditCamerasTransformed);
}

void CDispatch::FireAuthorZoomChanged()
{
    m_EditCameraChangeListeners.FireEvent(&CEditCameraChangeListener::onAuthorZoomChanged);
}

///////////////////////////////////////////////////////////////////////////////
// Async messages from the core
void CDispatch::AddCoreAsynchronousEventListener(CCoreAsynchronousEventListener *inListener)
{
    m_CoreAsynchronousEventListeners.AddListener(inListener);
}

void CDispatch::RemoveCoreAsynchronousEventListener(CCoreAsynchronousEventListener *inListener)
{
    m_CoreAsynchronousEventListeners.RemoveListener(inListener);
}

void CDispatch::FireOnAsynchronousCommand(CCmd *inCmd)
{
    m_CoreAsynchronousEventListeners.FireEvent(
        &CCoreAsynchronousEventListener::OnAsynchronousCommand, inCmd);
}

struct SGenericAsyncCommand : public CCmd
{
    std::function<void()> m_Function;
    SGenericAsyncCommand(std::function<void()> inFunc)
        : m_Function(inFunc)
    {
        SetUndoable(false);
        m_ShouldSetModified = false;
    }

    unsigned long Do() override
    {
        m_Function();
        return 0;
    }
    unsigned long Undo() override { return 0; }
};

void CDispatch::FireOnAsynchronousCommand(std::function<void()> inFunction)
{
    FireOnAsynchronousCommand(new SGenericAsyncCommand(inFunction));
}

///////////////////////////////////////////////////////////////////////////////
// Cleanly display app status messages
void CDispatch::AddAppStatusListener(CAppStatusListener *inListener)
{
    m_AppStatusListeners.AddListener(inListener);
}

void CDispatch::RemoveAppStatusListener(CAppStatusListener *inListener)
{
    m_AppStatusListeners.RemoveListener(inListener);
}

void CDispatch::FireOnDisplayAppStatus(const QString &inStatusMsg)
{
    m_AppStatusListeners.FireEvent(&CAppStatusListener::OnDisplayAppStatus, inStatusMsg);
}

void CDispatch::FireOnProgressBegin(const QString &inActionText,
                                    const QString &inAdditionalText)
{
    m_AppStatusListeners.FireEvent(&CAppStatusListener::OnProgressBegin, inActionText,
                                   inAdditionalText);
}

void CDispatch::FireOnProgressEnd()
{
    m_AppStatusListeners.FireEvent(&CAppStatusListener::OnProgressEnd);
}

///////////////////////////////////////////////////////////////////////////////
// Failure notification
void CDispatch::AddFailListener(CFailListener *inListener)
{
    m_FailListeners.AddListener(inListener);
}

void CDispatch::RemoveFailListener(CFailListener *inListener)
{
    m_FailListeners.RemoveListener(inListener);
}

void CDispatch::FireOnAssetDeleteFail()
{
    m_FailListeners.FireEvent(&CFailListener::OnAssetDeleteFail);
}

void CDispatch::FireOnPasteFail()
{
    m_FailListeners.FireEvent(&CFailListener::OnPasteFail);
}

void CDispatch::FireOnBuildConfigurationFileParseFail(const QString &inMessage)
{
    m_FailListeners.FireEvent(&CFailListener::OnBuildconfigurationFileParseFail, inMessage);
}

void CDispatch::FireOnSaveFail(bool inKnownError)
{
    m_FailListeners.FireEvent(&CFailListener::OnSaveFail, inKnownError);
}

void CDispatch::FireOnErrorFail(const QString &inText)
{
    m_FailListeners.FireEvent(&CFailListener::OnErrorFail, inText);
}

void CDispatch::FireOnRefreshResourceFail(const QString &inResourceName,
                                          const QString &inDescription)
{
    m_FailListeners.FireEvent(&CFailListener::OnRefreshResourceFail, inResourceName, inDescription);
}

void CDispatch::FireOnUndefinedDatainputsFail(
        const QMultiMap<QString, QPair<qt3dsdm::Qt3DSDMInstanceHandle,
                                       qt3dsdm::Qt3DSDMPropertyHandle>> *map)
{
    m_FailListeners.FireEvent(&CFailListener::OnUndefinedDatainputsFail, map);
}

void CDispatch::AddRendererListener(CRendererListener *inListener)
{
    m_RendererListeners.AddListener(inListener);
}

void CDispatch::RemoveRendererListener(CRendererListener *inListener)
{
    m_RendererListeners.RemoveListener(inListener);
}

void CDispatch::FireOnRendererInitialized()
{
    m_RendererListeners.FireEvent(&CRendererListener::OnRendererInitialized);
}

#include "Dispatch.moc"
