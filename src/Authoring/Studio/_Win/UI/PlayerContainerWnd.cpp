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

#include "stdafx.h"
#include "SceneView.h"
#include "Doc.h"
#include "StudioProjectSettings.h"
#include "Dispatch.h"
#include "HotKeys.h"
#include "MasterP.h"
#include "StudioApp.h"
#include "IStudioRenderer.h"
#include "PlatformStrings.h"
#include "PlayerContainerWnd.h"
#include "Qt3DSDMStudioSystem.h"
#include "Core.h"
#include "MainFrm.h"
#include "StudioUtils.h"

#include <QtWidgets/qscrollbar.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#include <QtGui/qscreen.h>

CPlayerContainerWnd::CPlayerContainerWnd(CSceneView *inSceneView)
    : QScrollArea(inSceneView)
    , m_SceneView(inSceneView)
    , m_PlayerWnd(NULL)
    , m_ViewMode(VIEW_SCENE)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

CPlayerContainerWnd::~CPlayerContainerWnd()
{
}

bool CPlayerContainerWnd::ShouldHideScrollBars()
{
    return m_ViewMode == VIEW_EDIT || g_StudioApp.IsAuthorZoom();
}

//==============================================================================
/**
 * SetPlayerWndPosition: Sets the position of the child player window
 *
 * Called when the view is scrolled to position the child player window
 * @param outLeftPresentationEdge   the left edge of the presentation, with the scroll position
 *                                  taken into consideration
 * @param outTopPresentionEdge      the top edge of the presentation, with the scroll
 *                                  position taken into consideration
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
    } else {
        // This stays a negated value
        outLeftPresentationEdge = theHScrollPosition;
    }

    // Vertical scrollbar does not exist
    if (m_ClientRect.height() < theClientRect.height()) {
        theVScrollPosition =
                theClientRect.top() + (theClientRect.height() / 2) - (m_ClientRect.height() / 2);
        outTopPresentionEdge = theVScrollPosition;
    } else {
        // This stays a negated value
        outTopPresentionEdge = theVScrollPosition;
    }

    // Move the child player window
    m_PlayerWnd->setGeometry(QRect(QPoint(theHScrollPosition, theVScrollPosition),
                                   m_ClientRect.size()));
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

    if (ShouldHideScrollBars()) {
        horizontalScrollBar()->setRange(0, 0);
        verticalScrollBar()->setRange(0, 0);
        horizontalScrollBar()->setValue(0);
        verticalScrollBar()->setValue(0);
    } else {
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
 */
//==============================================================================
void CPlayerContainerWnd::OnRulerGuideToggled()
{
    int scrollAmount = g_StudioApp.GetRenderer().AreGuidesEnabled() ? 16 : -16;
    bool hasHorz = horizontalScrollBar()->isVisible();
    bool hasVert = verticalScrollBar()->isVisible();
    int hscrollPos = 0, vscrollPos = 0;
    if (hasHorz)
        hscrollPos = qMax(horizontalScrollBar()->value() + scrollAmount, 0);
    if (hasVert)
        vscrollPos = qMax(verticalScrollBar()->value() + scrollAmount, 0);
    horizontalScrollBar()->setValue(hscrollPos);
    verticalScrollBar()->setValue(vscrollPos);
    m_PlayerWnd->update();
}

qreal CPlayerContainerWnd::fixedDevicePixelRatio() const
{
    // Fix a problem on X11: https://bugreports.qt.io/browse/QTBUG-65570
    qreal ratio = devicePixelRatio();
    if (QWindow *w = window()->windowHandle()) {
        if (QScreen *s = w->screen())
            ratio = s->devicePixelRatio();
    }
    return ratio;
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

    if (!ShouldHideScrollBars()) {
        theClientSize = GetEffectivePresentationSize();

        // Only center if we need to scroll
        if (theClientSize.width() > theViewClientRect.width()) {
            // compute the size of the client rectangle to display
            m_ClientRect.setLeft(
                        (theViewClientRect.width() / 2) - (theClientSize.width() / 2)
                        - (theClientSize.width() % 2));
            m_ClientRect.setRight((theViewClientRect.width() / 2) + (theClientSize.width() / 2));
        }

        if (theClientSize.height() > theViewClientRect.height()) {
            m_ClientRect.setTop(
                        (theViewClientRect.height() / 2) - (theClientSize.height() / 2)
                        - (theClientSize.height() % 2));
            m_ClientRect.setBottom((theViewClientRect.height() / 2) + (theClientSize.height() / 2));
        }
    }

    QRect glRect = m_ClientRect;
    glRect.setWidth(int(fixedDevicePixelRatio() * m_ClientRect.width()));
    glRect.setHeight(int(fixedDevicePixelRatio() * m_ClientRect.height()));
    g_StudioApp.GetRenderer().SetViewRect(glRect);
}

void CPlayerContainerWnd::wheelEvent(QWheelEvent* event)
{
    const bool theCtrlKeyIsDown = event->modifiers() & Qt::ControlModifier;
    const bool theAltKeyIsDown = event->modifiers() & Qt::AltModifier;

    if (!theCtrlKeyIsDown && !theAltKeyIsDown && !IsDeploymentView()) {
        // Zoom when in edit camera view
        g_StudioApp.GetCore()->GetDispatch()->FireSceneMouseWheel(
                    SceneDragSenderType::Matte, event->delta(), STUDIO_TOOLMODE_CAMERA_ZOOM);
    } else {
        // Otherwise, scroll the view
        QScrollArea::wheelEvent(event);
    }
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

void CPlayerContainerWnd::setToolMode(long inMode)
{
    if (m_PlayerWnd)
        m_PlayerWnd->setToolMode(inMode);
}

QSize CPlayerContainerWnd::GetEffectivePresentationSize() const
{
    CPt cSize = g_StudioApp.GetCore()->GetStudioProjectSettings()->GetPresentationSize();
    QSize theSize(cSize.x, cSize.y);

    // If we have guides, resize the window with enough space for the guides as well as the
    // presentation
    // This is a very dirty hack because we are of course hardcoding the size of the guides.
    // If the size of the guides never changes, the bet paid off.
    if (g_StudioApp.GetRenderer().AreGuidesEnabled())
        theSize += QSize(32, 32);

    return theSize / fixedDevicePixelRatio();
}

//==============================================================================

void CPlayerContainerWnd::SetPlayerWnd(CPlayerWnd *inPlayerWnd)
{
    m_PlayerWnd = inPlayerWnd;
    viewport()->setBackgroundRole(QPalette::Dark);
    m_PlayerWnd->setContainerWnd(this);
    m_PlayerWnd->setParent(viewport());
    m_PlayerWnd->setVisible(true);
    m_PlayerWnd->resize(m_PlayerWnd->sizeHint());
}

void CPlayerContainerWnd::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);

    SetScrollRanges();
}
