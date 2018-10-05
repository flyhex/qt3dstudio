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
#include "StudioPreferences.h"

#include <QtWidgets/qscrollbar.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#include <QtGui/qscreen.h>

CPlayerContainerWnd::CPlayerContainerWnd(CSceneView *inSceneView)
    : QScrollArea(inSceneView)
    , m_SceneView(inSceneView)
    , m_PlayerWnd(nullptr)
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
 *
 */
//==============================================================================
void CPlayerContainerWnd::SetPlayerWndPosition()
{
    QRect viewRect = rect();
    long theHScrollPosition = 0;
    long theVScrollPosition = 0;
    // Negate to adjust actual client positions
    if (!ShouldHideScrollBars()) {
        if (m_ClientRect.width() > viewRect.width()) {
            theHScrollPosition = -horizontalScrollBar()->value();
            viewRect.setWidth(m_ClientRect.width());
        }
        if (m_ClientRect.height() > viewRect.height()) {
            theVScrollPosition = -verticalScrollBar()->value();
            viewRect.setHeight(m_ClientRect.height());
        }
    }
    // Move the child player window
    m_PlayerWnd->setGeometry(QRect(QPoint(theHScrollPosition, theVScrollPosition),
                                   viewRect.size()));
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
    int scrollAmount = g_StudioApp.getRenderer().AreGuidesEnabled() ? 16 : -16;
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

template<typename T>
T even(const T val)
{
    // handle negative values
    T corr = (val > 0) ? -1 : 1;
    return (val % 2) ? (val + corr) : val;
}

//==============================================================================
/**
 *	RecenterClient: Recenters the Client rect in the View's client area.
 */
//==============================================================================
void CPlayerContainerWnd::RecenterClient()
{
    QRect theViewRect = rect();
    QSize theClientSize;
    QSize viewSize;
    m_ClientRect = theViewRect;
    viewSize = theViewRect.size();

    if (!ShouldHideScrollBars()) {
        theClientSize = GetEffectivePresentationSize();

        if (theClientSize.width() < theViewRect.width()) {
            m_ClientRect.setLeft(
                    even((theViewRect.width() / 2) - (theClientSize.width() / 2)));
        } else {
            viewSize.setWidth(theClientSize.width());
        }
        m_ClientRect.setWidth(theClientSize.width());

        if (theClientSize.height() < theViewRect.height()) {
            m_ClientRect.setTop(
                    even((theViewRect.height() / 2) - (theClientSize.height() / 2)));
        } else {
            viewSize.setHeight(theClientSize.height());
        }
        m_ClientRect.setHeight(theClientSize.height());
    }

    QRect glRect = m_ClientRect;
    glRect.setWidth(int(fixedDevicePixelRatio() * m_ClientRect.width()));
    glRect.setHeight(int(fixedDevicePixelRatio() * m_ClientRect.height()));
    g_StudioApp.getRenderer().SetViewRect(glRect, viewSize);
}

void CPlayerContainerWnd::wheelEvent(QWheelEvent* event)
{
    const bool theCtrlKeyIsDown = event->modifiers() & Qt::ControlModifier;

    if (!theCtrlKeyIsDown && !IsDeploymentView()) {
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
    SetPlayerWndPosition();
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
    m_SceneView->recheckSizingMode();
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
    if (g_StudioApp.getRenderer().AreGuidesEnabled())
        theSize += QSize(CStudioPreferences::guideSize(), CStudioPreferences::guideSize());

    return theSize / fixedDevicePixelRatio();
}

//==============================================================================

void CPlayerContainerWnd::SetPlayerWnd(Q3DStudio::Q3DSPlayerWnd *inPlayerWnd)
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
