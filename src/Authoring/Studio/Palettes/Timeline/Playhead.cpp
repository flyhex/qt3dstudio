/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#include "Playhead.h"
#include "Renderer.h"
#include "TimelineTimelineLayout.h"
#include "IDoc.h"
#include "HotKeys.h"
#include "ResourceCache.h"
#include "TimeEditDlg.h"
#include "TimeMeasure.h"

//=============================================================================
/**
 * Create a new Playhead.
 * The timeline is used for notifying of time changes and for scrolling.
 * @param inTimeline the timeline that this is a part of.
 */
CPlayhead::CPlayhead(CTimelineTimelineLayout *inTimeline, IDoc *inDoc)
    : m_IsMouseDown(false)
    , m_MinimumPosition(0)
    , m_MaximumPosition(LONG_MAX)
    , m_InitialOffset(0)
    , m_Doc(inDoc)
{
    m_Timeline = inTimeline;

    m_PlayheadImage = CResourceCache::GetInstance()->GetBitmap("PlaybackHead.png");
    SetName("Playhead");

    // Set the line to the middle of this.
    m_LinePos = GetSize().x / 2;
    SetAlpha(128);
}

CPlayhead::~CPlayhead()
{
}

//=============================================================================
/**
 * Call from the OverlayControl to perform the draw.
 * Wish this didn't have to be a special function but...
 * @param inRenderer the renderer to draw to.
 */
void CPlayhead::Draw(CRenderer *inRenderer)
{
    // If this goes before position then clip it at 0.
    if (GetPosition().x < 0) {
        CRct theClipRect(GetSize());
        theClipRect.Offset(CPt(-GetPosition().x, 0));
        inRenderer->PushClippingRect(theClipRect);
    }

    // Draw the playhead
    inRenderer->DrawBitmap(CPt(0, 2), m_PlayheadImage);

    // Draw the line
    inRenderer->PushPen(CColor(255, 0, 0));
    inRenderer->MoveTo(m_LinePos, 20);
    inRenderer->LineTo(m_LinePos, GetSize().y - 1);
    inRenderer->PopPen();

    // If we added an extra clipping rect then remove it.
    if (GetPosition().x < 0) {
        inRenderer->PopClippingRect();
    }
}

//=============================================================================
/**
 * Handles mouse down messages.
 * @param inPoint where the mouse was clicked.
 * @param inFlags the state of the mouse.
 */
bool CPlayhead::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    // If no one else processed it then process the command.
    if (!COverlayControl::OnMouseDown(inPoint, inFlags)) {
        m_Snapper.Clear();
        m_Snapper.SetSource(this);
        m_Timeline->PopulateSnappingList(&m_Snapper);

        //		m_Snapper.SetTimeOffset( m_Timeline->GetViewTimeOffset( ) );
        m_Snapper.SetSnappingKeyframes(true);
        m_Snapper.BeginDrag(inPoint.x, GetSize().x / 2);

        m_InitialOffset = inPoint.x;
        m_IsMouseDown = true;

        m_Timeline->RecalcTime((inFlags & CHotKeys::MODIFIER_CONTROL) == 0, 0);
    }

    return true;
}

//=============================================================================
/**
  * Handles mouse double click messages. Pops up a time edit dialog box for
  * modifying playhead time.
  * @param inPoint the location of the mouse in local coordinates.
  * @param inFlags the state of the mouse.
  */
bool CPlayhead::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inPoint);
    Q_UNUSED(inFlags);

    CTimeEditDlg theTimeEditDlg;
    theTimeEditDlg.ShowDialog(GetCurrentTime(), 0, m_Doc, PLAYHEAD);
    return true;
}

//=============================================================================
/**
 * Handles mouse move messages.
 * @param inPoint the location of the mouse in local coordinates.
 * @param inFlags the state of the mouse.
 */
void CPlayhead::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    COverlayControl::OnMouseMove(inPoint, inFlags);

    // If we are down then move the playhead
    if (m_IsMouseDown) {
        long theTime = m_Snapper.ProcessDrag(GetCurrentTime(), inPoint.x, inFlags);
        bool theUpdateClientTimeFlag = (inFlags & CHotKeys::MODIFIER_CONTROL) == 0;

        UpdateTime(theTime, theUpdateClientTimeFlag);
    }
}

//=============================================================================
/**
 * Updates the time of the active context
 * @param inTime the new time
 * @param inUpdateClient true if scene is to be redrawn
 */
void CPlayhead::UpdateTime(long inTime, bool /*inUpdateClient*/)
{
    // true to "check bounds" to ensure playhead is within valid range.
    m_Doc->NotifyTimeChanged(inTime);
}

//=============================================================================
/**
 * Notification that the mouse was let go.
 * @param inPoint the location of the mouse in local coordinates.
 * @param inFlags the state of the mouse.
 */
void CPlayhead::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    COverlayControl::OnMouseUp(inPoint, inFlags);

    m_IsMouseDown = false;
}

//=============================================================================
/**
 * Set the size of this control.
 * @param inSize the size to be set.
 */
void CPlayhead::SetSize(CPt inSize)
{
    COverlayControl::SetSize(inSize);
    // Update the line position.
    m_LinePos = inSize.x / 2;
}

//=============================================================================
/**
 * Set the minimum and maximum allowed positions of the line.
 * @param inMinimumPosition the minimum allowed position.
 * @param inMaximumPosition the maximum allowed position.
 */
void CPlayhead::SetMinMaxPosition(long inMinimumPosition, long inMaximumPosition)
{
    m_MinimumPosition = inMinimumPosition;
    m_MaximumPosition = inMaximumPosition;
}

long CPlayhead::GetCurrentTime()
{
    return m_Doc->GetCurrentViewTime();
}

//=============================================================================
/**
 * Check to see if inPoint is over this control or not.
 * This overrides COverlayControl::HitTest so that the playhead line can be
 * exempt from mouse hits.
 * @param inPoint the location of the mouse in local coordinates.
 */
bool CPlayhead::HitTest(const CPt &inPoint) const
{
    if (inPoint.y < m_PlayheadImage.height()) {
        return COverlayControl::HitTest(inPoint);
    }
    return false;
}

void CPlayhead::StepRightSmall()
{
    long theInterval = m_Timeline->GetTimeMeasure()->GetSmallHashInterval();

    long theCurrentTime = GetCurrentTime();
    long theTime = theCurrentTime / theInterval * theInterval + theInterval;
    UpdateTime(theTime, true);
}

void CPlayhead::StepRightLarge()
{
    long theInterval = m_Timeline->GetTimeMeasure()->GetLargeHashInterval();

    long theCurrentTime = GetCurrentTime();
    long theTime = theCurrentTime / theInterval * theInterval + theInterval;
    UpdateTime(theTime, true);
}

void CPlayhead::StepLeftSmall()
{
    long theInterval = m_Timeline->GetTimeMeasure()->GetSmallHashInterval();

    long theCurrentTime = GetCurrentTime();
    long theTime = theCurrentTime / theInterval * theInterval - theInterval;

    UpdateTime(theTime, true);
}

void CPlayhead::StepLeftLarge()
{
    long theInterval = m_Timeline->GetTimeMeasure()->GetLargeHashInterval();

    long theCurrentTime = GetCurrentTime();
    long theTime = theCurrentTime / theInterval * theInterval - theInterval;

    UpdateTime(theTime, true);
}
