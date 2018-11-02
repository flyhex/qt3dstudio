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

#ifndef INCLUDED_DISPATCH_H
#define INCLUDED_DISPATCH_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "CoreConst.h"
#include "StudioObjectTypes.h"
#include "Qt3DSId.h"
#include "Multicaster.h"
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMSignals.h"
#include "SelectedValue.h"

#include <QPoint>

//==============================================================================
//	Forwards
//==============================================================================
class CSceneDragListener;
class CSelectionChangeListener;
class CAsset;
class CURL;
class CPresentationChangeListener;
class CResourceChangeListener;
class CClientPlayChangeListener;
class CAppStatusListener;
class CNavigationListener;
class CFileOpenListener;
class CToolbarChangeListener;
class CSelectedNodePropChangeListener;
class CEditCameraChangeListener;
class CTimelineChangeListener;
class CInspectableBase;
class CCoreAsynchronousEventListener;
class CCmd;
class CFailListener;
class CSharedBufferChangeListener;
class CSharedBuffer;
class IReloadListener;
class IDataModelListener;
class IDocumentBufferCacheListener;
class IDocumentResourcesListener;
class CRendererListener;
class SDispatchSignalSystem;

//==============================================================================
/**
 *	Dispatches commands
 */
//==============================================================================
class CDispatch
{
public:
    CDispatch();
    virtual ~CDispatch();

    void AddDataModelListener(IDataModelListener *inListener);
    void RemoveDataModelListener(IDataModelListener *inListener);
    // These are counted, so you can begin,begin,end,end and you will
    // get overall just one begin and one end event sent.
    // This is specifically to allow batching commands wrap their inner commands
    // with the begin/end notification set.
    void FireBeginDataModelNotifications();
    bool AreDataModificationsHappening() const;
    void FireEndDataModelNotifications();
    // Called so select views can refresh at a high rate (inspector and 3d view) when an object
    // is dragged (or when multiple keyframes are dragged.
    // Most views should ignore this or ensure they filter the message by at least their instance.
    // Inproperty may be 0 in which case it should be assumed all properties may have changed
    void FireImmediateRefreshInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void FireImmediateRefreshInstance(qt3dsdm::Qt3DSDMInstanceHandle *inInstance,
                                      long inInstanceCount);

    void AddReloadListener(IReloadListener *inListener);
    void RemoveReloadListener(IReloadListener *inListener);
    void FireReloadEffectInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance);

    void AddDocumentBufferCacheListener(IDocumentBufferCacheListener *inListener);
    void RemoveDocumentBufferCacheListener(IDocumentBufferCacheListener *inListener);
    void FireDocumentBufferCacheInvalidated(const QString &inCache);

    // Scene drag
    void AddSceneDragListener(CSceneDragListener *inListener);
    void RemoveSceneDragListener(CSceneDragListener *inListener);
    void FireSceneMouseDown(SceneDragSenderType::Enum inSender, const QPoint &inPoint, int inToolMode);
    void FireSceneMouseDrag(SceneDragSenderType::Enum inSender, const QPoint &inPoint, int inToolMode,
                            int inFlags);
    void FireSceneMouseMove(SceneDragSenderType::Enum inSender, const QPoint &inPoint);
    void FireSceneMouseDblClick(SceneDragSenderType::Enum inSender, const QPoint &inPoint);
    void FireSceneMouseWheel(SceneDragSenderType::Enum inSender, short inDelta, int inToolMode);
    void FireSceneMouseUp(SceneDragSenderType::Enum inSender);

    // Selection Change new skool.

    qt3dsdm::TSignalConnectionPtr
    ConnectSelectionChange(std::function<void(Q3DStudio::SSelectedValue)> inListener);
    void FireSelectionChange(Q3DStudio::SSelectedValue inValue);

    void AddPresentationChangeListener(CPresentationChangeListener *inListener);
    void RemovePresentationChangeListener(CPresentationChangeListener *inListener);
    void FireOnLoadingPresentation(bool inIsSubPresentation = false);
    void FireOnNewPresentation();
    void FireOnClosingPresentation();
    void FireOnSavingPresentation(const QString &inNewPresentationFile);
    void FireOnExportingAsset(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void FireOnPresentationModifiedExternally();

    void AddClientPlayChangeListener(CClientPlayChangeListener *inListener);
    void RemoveClientPlayChangeListener(CClientPlayChangeListener *inListener);
    void FireOnPlayStart();
    void FireOnPlayStop();
    void FireOnTimeChanged(long inTime);

    void AddNavigationListener(CNavigationListener *inListener);
    void RemoveNavigationListener(CNavigationListener *inListener);
    void FireOnNavigate(const QString &inURL, bool inForceDisplay);

    void AddFileOpenListener(CFileOpenListener *inListener);
    void RemoveFileOpenListener(CFileOpenListener *inListener);
    void FireOnOpenDocument(const QString &inFile, bool inSucceeded);
    void FireOnSaveDocument(const QString &inFile, bool inSucceeded, bool inSaveCopy);
    void FireOnDocumentPathChanged(const QString &inNewPath);

    // Toolbar changes
    void AddToolbarChangeListener(CToolbarChangeListener *inListener);
    void RemoveToolbarChangeListener(CToolbarChangeListener *inListener);
    void FireOnToolbarChange();

    // State Property changes
    void AddSelectedNodePropChangeListener(CSelectedNodePropChangeListener *inListener);
    void RemoveSelectedNodePropChangeListener(CSelectedNodePropChangeListener *inListener);
    void FireOnSelectedNodePropChange(const QString &inPropertyName);
    void FireOnMouseDragged(bool inConstrainXAxis, bool inConstrainYAxis);

    // Edit Camera changes
    void AddEditCameraChangeListener(CEditCameraChangeListener *inListener);
    void RemoveEditCameraChangeListener(CEditCameraChangeListener *inListener);
    void FireOnEditCameraChanged();
    void FireOnCamerasTransformed();
    void FireAuthorZoomChanged();

    // Asynchronous events originating in the core
    void AddCoreAsynchronousEventListener(CCoreAsynchronousEventListener *inListener);
    void RemoveCoreAsynchronousEventListener(CCoreAsynchronousEventListener *inListener);
    void FireOnAsynchronousCommand(CCmd *inCmd);
    void FireOnAsynchronousCommand(std::function<void()> inFunction);

    // Cleanly display app status messages
    void AddAppStatusListener(CAppStatusListener *inListener);
    void RemoveAppStatusListener(CAppStatusListener *inListener);
    void FireOnDisplayAppStatus(const QString &inStatusMsg);
    void FireOnProgressBegin(const QString &inActionText, const QString &inAdditionalText);
    void FireOnProgressEnd();

    // Failure notification
    void AddFailListener(CFailListener *inListener);
    void RemoveFailListener(CFailListener *inListener);
    void FireOnAssetDeleteFail();
    void FireOnPasteFail();
    void FireOnBuildConfigurationFileParseFail(const QString &inMessage);
    void FireOnSaveFail(bool inKnownError);
    void FireOnErrorFail(const QString &inText);
    void FireOnRefreshResourceFail(const QString &inResourceName,
                                   const QString &inDescription);
    void FireOnUndefinedDatainputsFail(
            const QMultiMap<QString,
                            QPair<qt3dsdm::Qt3DSDMInstanceHandle,
                                  qt3dsdm::Qt3DSDMPropertyHandle>> *map);

    void AddRendererListener(CRendererListener *inListener);
    void RemoveRendererListener(CRendererListener *inListener);
    void FireOnRendererInitialized();

protected:
    CMulticaster<IDocumentBufferCacheListener *> m_DocumentBufferCacheListeners;
    long m_DataModelBeginDepth;
    CMulticaster<IReloadListener *> m_ReloadListeners;
    CMulticaster<IDataModelListener *> m_DataModelListeners;
    CMulticaster<IDocumentResourcesListener *> m_DocumentResourcesListeners;
    CMulticaster<CSceneDragListener *> m_SceneDragListeners;
    CMulticaster<CSelectionChangeListener *> m_SelectionListeners;
    CMulticaster<CPresentationChangeListener *> m_PresentationChangeListeners;
    CMulticaster<CResourceChangeListener *> m_ResourceChangeListeners;
    CMulticaster<CClientPlayChangeListener *> m_ClientPlayChangeListeners;
    CMulticaster<CNavigationListener *> m_NavigationListeners;
    CMulticaster<CFileOpenListener *> m_FileOpenListeners;
    CMulticaster<CToolbarChangeListener *> m_ToolbarChangeListeners;
    CMulticaster<CSelectedNodePropChangeListener *> m_SelectedNodePropChangeListeners;
    CMulticaster<CEditCameraChangeListener *> m_EditCameraChangeListeners;
    CMulticaster<CTimelineChangeListener *> m_TimelineChangeListeners;
    CMulticaster<CCoreAsynchronousEventListener *> m_CoreAsynchronousEventListeners;
    CMulticaster<CAppStatusListener *> m_AppStatusListeners;
    CMulticaster<CFailListener *> m_FailListeners;
    CMulticaster<CSharedBufferChangeListener *> m_SharedBufferChangeListeners;
    CMulticaster<CRendererListener *> m_RendererListeners;
    std::shared_ptr<SDispatchSignalSystem> m_SignalSystem;
};

struct CDispatchDataModelNotificationScope
{
    CDispatch &m_Dispatch;
    CDispatchDataModelNotificationScope(CDispatch &dispatch)
        : m_Dispatch(dispatch)
    {
        m_Dispatch.FireBeginDataModelNotifications();
    }
    ~CDispatchDataModelNotificationScope()
    {
        // if there is already an exception in the stack, we should not call the below function,
        // exception in a destructor is bad news and there is a high chance that there is going to
        // be another exception
        // however we are still reducing the data model begin depth
        m_Dispatch.FireEndDataModelNotifications();
    }
};
struct CDispatchDataModelImmediateScope
{
    CDispatch &m_Dispatch;
    qt3dsdm::Qt3DSDMInstanceHandle m_Instance;

    CDispatchDataModelImmediateScope(CDispatch &dispatch, qt3dsdm::Qt3DSDMInstanceHandle instance)
        : m_Dispatch(dispatch)
        , m_Instance(instance)
    {
    }
    ~CDispatchDataModelImmediateScope()
    {
        // if there is already an exception in the stack, we should not call the below function,
        // exception in a destructor is bad news and there is a high chance that there is going to
        // be another exception
        if (!std::uncaught_exception()) {
            m_Dispatch.FireImmediateRefreshInstance(m_Instance);
        }
    }
};

#endif // INCLUDED_DISPATCH_H
