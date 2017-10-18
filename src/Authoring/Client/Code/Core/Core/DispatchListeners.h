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
#include "UICId.h"
#include "Pt.h"
#include "UICDMHandles.h"
#include <QPoint>

//==============================================================================
//	Forwards
//==============================================================================

class CAsset;
class CUICFile;
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

    virtual void OnNudge(ENudgeDirection inNudgeDirection, int inToolMode, int inFlags)
    {
        Q_UNUSED(inNudgeDirection);
        Q_UNUSED(inToolMode);
        Q_UNUSED(inFlags);
    }

    virtual void OnNudgeDone() {}
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
    //==========================================================================
    /**
     * New presentation or subpresentation (e.g. subtree such as component or pasted object) is
     * being loaded.
     */
    virtual void OnLoadingPresentation(bool inIsSubPresentation)
    {
        Q_UNUSED(inIsSubPresentation);
    }
    //==========================================================================
    /**
     * A subpresentation is loaded.
     */
    virtual void OnLoadedSubPresentation() {}
    //==========================================================================
    /**
     * New presentation is being created.
     */
    virtual void OnNewPresentation() {}

    //==========================================================================
    /**
     * The current presentation is being closed.
     */
    virtual void OnClosingPresentation() {}

    //==========================================================================
    /**
     * The current presentation is being saved.
     */
    virtual void OnSavingPresentation(const CUICFile *inNewPresentationFile)
    {
        Q_UNUSED(inNewPresentationFile);
    }

    virtual void OnExportingAsset(UICDM::CUICDMInstanceHandle inInstance)
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
    virtual void OnNavigate(const Q3DStudio::CString &inURL, bool inForceDisplay) = 0;
};

class CFileOpenListener
{
public:
    virtual void OnOpenDocument(const CUICFile &inFilename, bool inSucceeded) = 0;
    virtual void OnSaveDocument(const CUICFile &inFilename, bool inSucceeded, bool inSaveCopy) = 0;
    virtual void OnDocumentPathChanged(const CUICFile &inNewPath) = 0;
};

class CSelectedNodePropChangeListener
{
public:
    virtual void OnPropValueChanged(const Q3DStudio::CString &inPropertyName) = 0;
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
    virtual void OnEditCameraChanged() = 0;
    virtual void OnEditCamerasTransformed() = 0;
    virtual void OnAuthorZoomChanged() = 0;
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
    virtual void OnDisplayAppStatus(Q3DStudio::CString &inStatusMsg) = 0;
    virtual void OnProgressBegin(const Q3DStudio::CString &inActionText,
                                 const Q3DStudio::CString &inAdditionalText) = 0;
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
    virtual void OnBuildconfigurationFileParseFail(const Q3DStudio::CString &inMessage) = 0;
    virtual void OnSaveFail(bool inKnownError) = 0;
    virtual void OnProjectVariableFail(const Q3DStudio::CString &inMessage) = 0;
    virtual void OnErrorFail(const Q3DStudio::CString &inText) = 0;
    virtual void OnRefreshResourceFail(const Q3DStudio::CString &inResourceName,
                                       const Q3DStudio::CString &inDescription) = 0;
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
    virtual void OnImmediateRefreshInstanceSingle(UICDM::CUICDMInstanceHandle inInstance) = 0;
    // Same thing, but fired when more than one instance is being refreshed.
    virtual void OnImmediateRefreshInstanceMultiple(UICDM::CUICDMInstanceHandle *inInstance,
                                                    long inInstanceCount) = 0;
};

class IReloadListener
{
public:
    virtual void OnReloadEffectInstance(UICDM::CUICDMInstanceHandle inInstance) = 0;
};

class IDocumentBufferCacheListener
{
public:
    virtual ~IDocumentBufferCacheListener() {}
    // I can't get reference messages to work with the dispatch system.
    virtual void OnBufferInvalidated(Q3DStudio::CString inString) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// For StudioRenderer to send messages
class CRendererListener
{
public:
    virtual void OnRendererInitialized() = 0;
};

#endif // INCLUDED_DISPATCHLISTENERS_H
