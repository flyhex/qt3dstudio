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

//==============================================================================
//	Prefix
//==============================================================================

#include "stdafx.h"
#include "UICOptions.h"
#include "SceneView.h"
#include "Doc.h"
#include "StudioProjectSettings.h"
#include "Dispatch.h"
#include "HotKeys.h"
#include "MasterP.h"
#include "StudioApp.h"
#include "IStudioRenderer.h"

//==============================================================================
//	Includes
//==============================================================================

#include "PlatformStrings.h"
#include "PlayerContainerWnd.h"
#include "UICDMStudioSystem.h"
#include "Core.h"
#include "MainFrm.h"
#include "StudioUtils.h"

#include <QtWidgets/qscrollbar.h>
#include <QtGui/qevent.h>

//==============================================================================
//	Class CPlayerContainerWnd
//==============================================================================

//==============================================================================
/**
 *	Constructor: Initializes the object.
 */
//==============================================================================
CPlayerContainerWnd::CPlayerContainerWnd(CSceneView *inSceneView)
    : QAbstractScrollArea(inSceneView)
    , m_SceneView(inSceneView)
    , m_PlayerWnd(NULL)
    , m_IsMouseDown(false)
    , m_IsMiddleMouseDown(false)
    , m_ViewMode(VIEW_SCENE)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

//==============================================================================
/**
 *	Destructor: Releases the object.
 */
//==============================================================================
CPlayerContainerWnd::~CPlayerContainerWnd()
{
}

bool CPlayerContainerWnd::ShouldHideScrollBars()
{
    return m_ViewMode == VIEW_EDIT || g_StudioApp.IsAuthorZoom();
}

//==============================================================================
/**
 *	SetPlayerWndPosition: Sets the position of the child player window
 *
 *  Called when the view is scrolled to position the child player window
 *	@param outLeftPresentationEdge  the left edge of the presentation, with the scroll position
 *taken into consideration
 *	@param outTopPresentionEdge		the top edge of the presentation, with the scroll
 *position taken into consideration
 *
 */
//==============================================================================
void CPlayerContainerWnd::SetPlayerWndPosition(long &outLeftPresentationEdge,
                                               long &outTopPresentionEdge)
{
    long theHScrollPosition, theVScrollPosition;
    // Negate to adjust actual client positions
    theHScrollPosition = -horizontalScrollBar()->value();
    theVScrollPosition = -verticalScrollBar()->value();

    QRect theClientRect = rect();

    // Horizontal scrollbar does not exist
    if (m_ClientRect.width() < theClientRect.width()) {
        theHScrollPosition =
            theClientRect.left() + (theClientRect.width() / 2) - (m_ClientRect.width() / 2);
        outLeftPresentationEdge = theHScrollPosition;
    } else // This stays a negated value
        outLeftPresentationEdge = theHScrollPosition;

    // Vertical scrollbar does not exist
    if (m_ClientRect.height() < theClientRect.height()) {
        theVScrollPosition =
            theClientRect.top() + (theClientRect.height() / 2) - (m_ClientRect.height() / 2);
        outTopPresentionEdge = theVScrollPosition;
    } else // This stays a negated value
        outTopPresentionEdge = theVScrollPosition;

    // Move the child player window
    m_PlayerWnd->setGeometry(QRect(QPoint(theHScrollPosition, theVScrollPosition), m_ClientRect.size()));
}

//==============================================================================
/**
 *	SetScrollRanges: Sets the scroll ranges when the view is being resized
 */
//==============================================================================
void CPlayerContainerWnd::SetScrollRanges()
{

    long theScrollWidth = 0;
    long theScrollHeight = 0;

#ifdef INCLUDE_EDIT_CAMERA
    if (ShouldHideScrollBars()) {
        horizontalScrollBar()->setRange(0, 0);
        verticalScrollBar()->setRange(0, 0);
        horizontalScrollBar()->setValue(0);
        verticalScrollBar()->setValue(0);
    } else
#endif
    {
        QSize theSize = GetEffectivePresentationSize();

        theScrollWidth = theSize.width();
        theScrollHeight = theSize.height();


        // Set scrollbar ranges
        horizontalScrollBar()->setRange(0, theScrollWidth - width());
        verticalScrollBar()->setRange(0, theScrollHeight - height());
        horizontalScrollBar()->setPageStep(width());
        verticalScrollBar()->setPageStep(height());
        horizontalScrollBar()->setVisible(true);
        verticalScrollBar()->setVisible(true);
    }
}

//==============================================================================
/**
 *	OnRulerGuideToggled:
 *	Handle scrollbar position when ruler, guide has been toggled
 *
 */
//==============================================================================
void CPlayerContainerWnd::OnRulerGuideToggled()
{
    int scrollAmount = g_StudioApp.GetRenderer().AreGuidesEnabled() ? 16 : -16;
    bool hasHorz = horizontalScrollBar()->isVisible();
    bool hasVert = verticalScrollBar()->isVisible();
    int hscrollPos = 0, vscrollPos = 0;
    if (hasHorz) {
        hscrollPos = qMax(horizontalScrollBar()->value() + scrollAmount, 0);
    }
    if (hasVert) {
        vscrollPos = qMax(verticalScrollBar()->value() + scrollAmount, 0);
    }
    horizontalScrollBar()->setValue(hscrollPos);
    verticalScrollBar()->setValue(vscrollPos);
    m_PlayerWnd->update();
}

//==============================================================================
/**
 *	RecenterClient: Recenters the Client rect in the View's client area.
 */
//==============================================================================
void CPlayerContainerWnd::RecenterClient()
{
    QRect theViewClientRect = rect();
    QSize theClientSize;
    m_ClientRect = theViewClientRect;

#ifdef INCLUDE_EDIT_CAMERA
    if (ShouldHideScrollBars()) {
    } else
#endif
    {
        theClientSize = GetEffectivePresentationSize();

        // Only center if we need to scroll
        if (theClientSize.width() > theViewClientRect.width()) {
            // compute the size of the client rectangle to display
            m_ClientRect.setLeft(
                (theViewClientRect.width() / 2) - (theClientSize.width() / 2) - (theClientSize.width() % 2));
            m_ClientRect.setRight((theViewClientRect.width() / 2) + (theClientSize.width() / 2));
        }

        if (theClientSize.height() > theViewClientRect.height()) {
            m_ClientRect.setTop(
                (theViewClientRect.height() / 2) - (theClientSize.height() / 2) - (theClientSize.height() % 2));
            m_ClientRect.setBottom((theViewClientRect.height() / 2) + (theClientSize.height() / 2));
        }
    }

    QRect glRect = m_ClientRect;
    glRect.setWidth(int(devicePixelRatio() * m_ClientRect.width()));
    glRect.setHeight(int(devicePixelRatio() * m_ClientRect.height()));
    g_StudioApp.GetRenderer().SetViewRect(glRect);
}

//==============================================================================
/**
 *	OnLButtonDown: Called whenever the user left clicks in the view.
 *	This processes the WM_LBUTTONDOWN message. This message is generated whenever
 *	the user left clicks in the view. Since this could involve selection of an item
 *	in the scene, it may (if the click is in the Client rect) call ProcessMouseClick()
 *	on the Document to perform the selection.
 *	@param inFlags the flags passed in from the message call
 *	@param inPoint the point where the event takes place
 */
void CPlayerContainerWnd::mousePressEvent(QMouseEvent *event)
{
    if ((event->button() == Qt::LeftButton) || (event->button() == Qt::RightButton)) {
        long theToolMode = g_StudioApp.GetToolMode();
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDown(SceneDragSenderType::Matte, event->pos(),
                                                                 theToolMode);
        m_IsMouseDown = true;
    } else if (event->button() == Qt::MiddleButton) {
#ifdef INCLUDE_EDIT_CAMERA
    const bool theCtrlKeyIsDown = event->modifiers() & Qt::ControlModifier;
    const bool theAltKeyIsDown = event->modifiers() & Qt::AltModifier;

    bool theToolChanged = false;
    if (rect().contains(event->pos()) && !IsDeploymentView()) {
        // If both the control key and the Alt key is not down
        if (!theCtrlKeyIsDown && !theAltKeyIsDown) {
            // press Scroll Wheel Click
            // Do Camera Pan
            g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_PAN);
            theToolChanged = true;
        } else if ((theAltKeyIsDown) && (!theCtrlKeyIsDown)) {
            // press Alt-Scroll Wheel Click
            // Do Camera Rotate if we are in 3D Camera
            if (g_StudioApp.GetRenderer().DoesEditCameraSupportRotation(
                    g_StudioApp.GetRenderer().GetEditCamera())) {
                g_StudioApp.SetToolMode(STUDIO_TOOLMODE_CAMERA_ROTATE);
                theToolChanged = true;
            }
        }
    }

    if (theToolChanged) {
        Q_EMIT toolChanged();
        m_SceneView->SetViewCursor();

        long theToolMode = g_StudioApp.GetToolMode();
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDown(SceneDragSenderType::Matte,
                                                                 event->pos(), theToolMode);
        m_IsMouseDown = true;
        m_IsMiddleMouseDown = true;
    }
#else
    Q_UNUSED(inFlags);
    Q_UNUSED(inPoint);
#endif
    }
}

//==============================================================================
/**
 * OnLButtonUp: Called whenever the user releases the left mouse button.
 *
 * This processes the WM_LBUTTONUP message. This message is generated whenever
 * the left mouse button. We release the mouse capture to stop dragging.
 *
 * @param inFlags The flags passed in from the message call
 * @param inPoint The point where the event takes place
 */
//==============================================================================
void CPlayerContainerWnd::mouseReleaseEvent(QMouseEvent *event)
{
     if ((event->button() == Qt::LeftButton) || (event->button() == Qt::RightButton)) {
         // Need to commit the current command when we have a mouse up. :)
         g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseUp(SceneDragSenderType::Matte);
         g_StudioApp.GetCore()->CommitCurrentCommand();
         m_IsMouseDown = false;
     } else if (event->button() == Qt::MiddleButton) {
    #ifdef INCLUDE_EDIT_CAMERA
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseUp(SceneDragSenderType::Matte);
        g_StudioApp.GetCore()->CommitCurrentCommand();
        if (m_IsMiddleMouseDown) {
            m_IsMouseDown = false;
            m_IsMiddleMouseDown = false;

            const bool theCtrlKeyIsDown = event->modifiers() & Qt::ControlModifier;
            const bool theAltKeyIsDown = event->modifiers() & Qt::AltModifier;

            if (!IsDeploymentView()) {
                if (!theCtrlKeyIsDown && !theAltKeyIsDown) {
                    // none of the modifier key is pressed... reset to previous tool
                    m_SceneView->RestorePreviousTool();
                } else if (theCtrlKeyIsDown && theAltKeyIsDown) {
                    // since both modifier is down... let the ctrl has priority
                    m_SceneView->SetToolOnCtrl();
                }
                m_SceneView->SetViewCursor();
                Q_EMIT toolChanged();
            }
        }
    #endif
     }
}

//==============================================================================
/**
 * OnMouseMove: Called whenever the user moves the mouse in the window.
 *
 * This processes the WM_MOUSEMOVE message. This message is generated whenever
 * the user moves the mouse in the view. This tells lets the document process it
 * as well since the user could be dragging an object.
 *
 * @param inFlags The flags passed in from the message call
 * @param inPoint The point where the event takes place
 */
//==============================================================================
void CPlayerContainerWnd::mouseMoveEvent(QMouseEvent* event)
{
    if (m_IsMouseDown) {
        UICPROFILE(OnMouseMove);

        long theModifierKeys = 0;
        if (event->buttons() & Qt::LeftButton)
            theModifierKeys = CHotKeys::MOUSE_LBUTTON | CHotKeys::GetCurrentKeyModifiers();
        else if (event->buttons() & Qt::RightButton)
            theModifierKeys = CHotKeys::MOUSE_RBUTTON | CHotKeys::GetCurrentKeyModifiers();

        long theToolMode = g_StudioApp.GetToolMode();
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseDrag(
            SceneDragSenderType::Matte, event->pos(), theToolMode, theModifierKeys);
    }
}

//==============================================================================
/**
 * OnMouseWheel: Called whenever the mouse wheel.
 *
 * This processes the WM_MOUSEWHEEL message.
 *
 * @param inFlags the flags passed in from the message call
 * @param inPoint the point where the event takes place
 */
//==============================================================================
void CPlayerContainerWnd::wheelEvent(QWheelEvent* event)
{
#ifdef INCLUDE_EDIT_CAMERA
    // Note : Mouse wheel is a special animal of the scene drag tool. We dont change the tool
    // so as not to affect the toolbar button and the view cursor. This will just do the zoom
    // and the cursor is not changed.

    const bool theCtrlKeyIsDown = event->modifiers() & Qt::ControlModifier;
    const bool theAltKeyIsDown = event->modifiers() & Qt::AltModifier;

    // Keeping these codes here, till we finalized the behavior and confirm these not needed
    // long theToolMode = g_StudioApp.GetToolMode( );
    //// If both the control key and the Alt key is not down
    // if ( !theCtrlKeyIsDown && !theAltKeyIsDown && !IsDeploymentView( ) )
    //{
    //	if ( theToolMode != STUDIO_TOOLMODE_CAMERA_ZOOM )
    //	{
    //		g_StudioApp.SetToolMode( STUDIO_TOOLMODE_CAMERA_ZOOM );
    //		theToolMode = STUDIO_TOOLMODE_CAMERA_ZOOM;
    //		m_MouseWheeling = true;

    //      Q_EMIT toolChanged();
    //		SetViewCursor( );
    //	}
    //}

    // Mouse Wheel
    // Do Camera Zoom
    if (!theCtrlKeyIsDown && !theAltKeyIsDown && !IsDeploymentView())
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseWheel(
            SceneDragSenderType::Matte, event->delta(), STUDIO_TOOLMODE_CAMERA_ZOOM);
#else
    Q_UNUSED(event);
#endif
}

void CPlayerContainerWnd::scrollContentsBy(int, int)
{
    long x, y;
    SetPlayerWndPosition(x, y);
}

//==============================================================================
/**
 *	Set the view mode of the current scene view, whether we are in editing mode
 *	or deployment mode. For editing mode, we want to use the full scene area without
 *	any matte area.
 *	@param inViewMode	the view mode of this scene
 */
void CPlayerContainerWnd::SetViewMode(EViewMode inViewMode)
{
    m_ViewMode = inViewMode;
    m_SceneView->RecheckSizingMode();
    if (m_ViewMode == VIEW_SCENE) {
        // switching from edit mode to deployment mode, release the edit camera tool and set it to
        // object move
        long theCurrentToolSettings = g_StudioApp.GetToolMode();
        bool theIsCameraTool = (theCurrentToolSettings & STUDIO_CAMERATOOL_MASK ? true : false);
        if (theIsCameraTool) {
            g_StudioApp.SetToolMode(STUDIO_TOOLMODE_MOVE);
            m_SceneView->SetToolMode(STUDIO_TOOLMODE_MOVE);
        }
    }
}

//==============================================================================
/**
 *	return the view mode of the current scene view, whether we are in editing mode
 *	or deployment mode. For editing mode, we want to use the full scene area without
 *	any matte area.
 *	@return  the current view mode
 */
CPlayerContainerWnd::EViewMode CPlayerContainerWnd::GetViewMode()
{
    return m_ViewMode;
}

//==============================================================================
/**
 *	Checks whether we are in deployment view mode.
 *	@return true if is in deployment view mode, else false
 */
bool CPlayerContainerWnd::IsDeploymentView()
{
    return m_ViewMode == VIEW_SCENE ? true : false;
}

QSize CPlayerContainerWnd::GetEffectivePresentationSize() const
{
    CPt cSize = g_StudioApp.GetCore()->GetStudioProjectSettings()->GetPresentationSize();
    QSize theSize(cSize.x, cSize.y);

    // If we have guides, resize the window with enough space for the guides as well as the
    // presentation
    // This is a very dirty hack because we are of course hardcoding the size of the guides.
    // If the size of the guides never changes, the bet paid off.
    if (g_StudioApp.GetRenderer().AreGuidesEnabled()) {
        theSize += QSize(32, 32);
    }

    return theSize / devicePixelRatio();
}

//==============================================================================

void CPlayerContainerWnd::SetPlayerWnd(CPlayerWnd *inPlayerWnd)
{
    m_PlayerWnd = inPlayerWnd;
    viewport()->setBackgroundRole(QPalette::Dark);
    m_PlayerWnd->SetContainerWnd(this);
    m_PlayerWnd->setParent(viewport());
    m_PlayerWnd->setVisible(true);
    m_PlayerWnd->resize(m_PlayerWnd->sizeHint());
}

//==============================================================================
/**
 *	OnSize: Handles the WM_SIZE message
 *
 *	Recenters the Client and recaluclates the matte when a resize message is
 *	generated.
 *
 *	@param  nType	Specifies the type of resizing requested.
 *	@param	cx		Specifies the new width of the client area.
 *	@param	cy		Specifies the new height of the client area.
 */
//==============================================================================
void CPlayerContainerWnd::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);

#ifdef INCLUDE_EDIT_CAMERA
    SetScrollRanges();
#endif
}
