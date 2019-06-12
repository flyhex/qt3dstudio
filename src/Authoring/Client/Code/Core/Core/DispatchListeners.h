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

#ifndef INCLUDED_DISPATCHLISTENERS_H
#define INCLUDED_DISPATCHLISTENERS_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "CoreConst.h"
#include "Qt3DSId.h"
#include "Pt.h"
#include "Qt3DSDMHandles.h"
#include <QPoint>

//==============================================================================
//	Forwards
//==============================================================================

class CAsset;
class Qt3DSFile;
class CURL;
class ISelectable;
class CCmd;
class CSharedBuffer;

//=============================================================================
/**
 * Listener for drag events on the scene
 */
//=============================================================================
class CSceneDragListener
{
public:
    virtual ~CSceneDragListener() {}
    virtual void OnSceneMouseDown(SceneDragSenderType::Enum inSender, QPoint inPoint, int inToolMode)
    {
        Q_UNUSED(inSender);
        Q_UNUSED(inPoint);
        Q_UNUSED(inToolMode);
    }

    virtual void OnSceneMouseDrag(SceneDragSenderType::Enum inSender, QPoint inPoint, int inToolMode,
                                  int inFlags)
    {
        Q_UNUSED(inSender);
        Q_UNUSED(inPoint);
        Q_UNUSED(inToolMode);
        Q_UNUSED(inFlags);
    }

    virtual void OnSceneMouseMove(SceneDragSenderType::Enum /*inSender*/, QPoint /*inPoint*/) {}

    virtual void OnSceneMouseUp(SceneDragSenderType::Enum inSender) = 0;

    virtual void OnSceneMouseDblClick(SceneDragSenderType::Enum inSender, QPoint inPoint)
    {
        Q_UNUSED(inSender);
        Q_UNUSED(inPoint);
    }

    virtual void OnSceneMouseWheel(SceneDragSenderType::Enum inSender, short inDelta,
                                   int inToolMode)
    {
        Q_UNUSED(inSender);
        Q_UNUSED(inDelta);
        Q_UNUSED(inToolMode);
    }
};

//=============================================================================
/**
 * Listener for timebar change events
 */
//=============================================================================
class CTimeBarChangeListener
{
public:
    virtual ~CTimeBarChangeListener() {}
    virtual void OnTimeBarChange(CAsset *inAsset) = 0;
};

//=============================================================================
/**
 * Listener for object time dependent change events
 */
//=============================================================================
class CTimeDependentChangeListener
{
public:
    virtual ~CTimeDependentChangeListener() {}
    virtual void OnTimeDependentChange(CAsset *inAsset) = 0;
};

//=============================================================================
/**
 * Listener for toolbar change events
 */
//=============================================================================
class CToolbarChangeListener
{
public:
    virtual ~CToolbarChangeListener() {}
    virtual void OnToolbarChange() = 0;
};

//=============================================================================
/**
 * Listener for changes to the presentation.
 */
class CPresentationChangeListener
{
public:
    virtual ~CPresentationChangeListener() {}
    /**
     * New presentation or subpresentation (e.g. subtree such as component or pasted object) is
     * being loaded.
     */
    virtual void OnLoadingPresentation(bool inIsSubPresentation)
    {
        Q_UNUSED(inIsSubPresentation);
    }

    /**
     * New presentation is being created.
     */
    virtual void OnNewPresentation() {}

    /**
     * The current presentation is being closed.
     */
    virtual void OnClosingPresentation() {}

    /**
     * The current presentation is being saved.
     */
    virtual void OnSavingPresentation(const QString &inNewPresentationFile)
    {
        Q_UNUSED(inNewPresentationFile);
    }

    virtual void OnExportingAsset(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
    {
        Q_UNUSED(inInstance);
    }

    virtual void OnPresentationModifiedExternally() {}
};

class CClientPlayChangeListener
{
public:
    virtual ~CClientPlayChangeListener() {}

    //==========================================================================
    /**
     * The presentation is being played (preview in studio)
     */
    virtual void OnPlayStart(){}

    //==========================================================================
    /**
     * The presentation is being stopped (preview in studio)
     */
    virtual void OnPlayStop(){}

    //==========================================================================
    /**
     * The presentation time has changed.
     * @param inTime the new client time.
     */
    virtual void OnTimeChanged(long inTime) = 0;
};

class CNavigationListener
{
public:
    virtual void OnNavigate(const QString &inURL, bool inForceDisplay) = 0;
};

class CFileOpenListener
{
public:
    virtual void OnOpenDocument(const QString &inFilename, bool inSucceeded) = 0;
    virtual void OnSaveDocument(const QString &inFilename, bool inSucceeded, bool inSaveCopy) = 0;
    virtual void OnDocumentPathChanged(const QString &inNewPath) = 0;
};

class CSelectedNodePropChangeListener
{
public:
    virtual void OnPropValueChanged(const QString &inPropertyName) = 0;
    virtual void OnMouseDragged(bool inConstrainXAxis, bool inConstrainYAxis) = 0;
};

//=============================================================================
/**
 * Listener for edit camera change events
 */
//=============================================================================
class CEditCameraChangeListener
{
public:
    virtual void onEditCameraChanged() = 0;
    virtual void onEditCamerasTransformed() = 0;
    virtual void onAuthorZoomChanged() = 0;
};

class CTimelineChangeListener
{
public:
    virtual void OnTimelineLayoutChanged() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Listener for async events originating in the core
class CCoreAsynchronousEventListener
{
public:
    virtual ~CCoreAsynchronousEventListener() {}
    virtual void OnAsynchronousCommand(CCmd *inCmd) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Clean way for anyone to set the app status message
class CAppStatusListener
{
public:
    virtual ~CAppStatusListener() {}
    virtual void OnDisplayAppStatus(const QString &inStatusMsg) = 0;
    virtual void OnProgressBegin(const QString &inActionText,
                                 const QString &inAdditionalText) = 0;
    virtual void OnProgressEnd() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Notification of failures
class CFailListener
{
public:
    virtual ~CFailListener() {}
    virtual void OnAssetDeleteFail() = 0;
    virtual void OnPasteFail() = 0;
    virtual void OnBuildconfigurationFileParseFail(const QString &inMessage) = 0;
    virtual void OnSaveFail(bool inKnownError) = 0;
    virtual void OnErrorFail(const QString &inText) = 0;
    virtual void OnRefreshResourceFail(const QString &inResourceName,
                                       const QString &inDescription) = 0;
    virtual void OnUndefinedDatainputsFail(
            const QMultiMap<QString,
                            QPair<qt3dsdm::Qt3DSDMInstanceHandle,
                                  qt3dsdm::Qt3DSDMPropertyHandle>> *map, bool askFromUser) = 0;
};

class IDataModelListener
{
public:
    virtual ~IDataModelListener() {}
    // Fired before a large group of notifications come out so views can
    // only refresh their view once.
    virtual void OnBeginDataModelNotifications() = 0;
    // Fired after a large gruop of notifications (onInstancePropertyChanged, etc) come out
    // so views can be careful about refreshing their data and there view
    virtual void OnEndDataModelNotifications() = 0;

    // Fired during 3d drag or mouse move events (or keyframe drag) or likewise
    // events so that views that need to update based on the new data can.
    virtual void OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance) = 0;
    // Same thing, but fired when more than one instance is being refreshed.
    virtual void OnImmediateRefreshInstanceMultiple(qt3dsdm::Qt3DSDMInstanceHandle *inInstance,
                                                    long inInstanceCount) = 0;
};

class IReloadListener
{
public:
    virtual void OnReloadEffectInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) = 0;
};

class IDocumentBufferCacheListener
{
public:
    virtual ~IDocumentBufferCacheListener() {}
    // I can't get reference messages to work with the dispatch system.
    virtual void OnBufferInvalidated(const QString &inString) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// For StudioRenderer to send messages
class CRendererListener
{
public:
    virtual void OnRendererInitialized() = 0;
};

#endif // INCLUDED_DISPATCHLISTENERS_H
