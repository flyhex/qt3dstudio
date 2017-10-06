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

//==============================================================================
//	Includes
//==============================================================================
#include "TimebarTip.h"
#include "TimebarControl.h"
#include "MouseCursor.h"
#include "TimelineControl.h"
#include "ResourceCache.h"
#include "Renderer.h"
#include "StudioUtils.h"

#include <QApplication>

//=============================================================================
/**
 * Create a timebar tip for the timebar.
 * This handles displaying the resize cursor and processing the mouse commands.
 * @param inTimebar the timebar on which this tip is attached.
 * @param inIsLeft true if this is the left timebar tip.
 */
CTimebarTip::CTimebarTip(CTimebarControl *inTimebar, bool inIsLeft, bool inHasHandle /*=false*/)
    : m_IsMouseDown(false)
    , m_MaybeDragStart(false)
    , m_HasHandle(false)
{
    m_Timebar = inTimebar;
    m_IsLeft = inIsLeft;

    ShowHandles(inHasHandle);
}

//=============================================================================
/**
 * Destructor
 */
CTimebarTip::~CTimebarTip()
{
}

//=============================================================================
/**
* Updates the ToolTip and moves it to the correct place on screen.
* @param inPoint the point that the tooltip is supposed to be placed.
*/
void CTimebarTip::RefreshToolTip(CPt inPoint)
{
    Q3DStudio::CString theCommentText;

    // format label as: startTime - endTime (timeDifference)
    theCommentText = " " + FormatTimeString(m_Timebar->GetStartTime()) + " - "
        + FormatTimeString(m_Timebar->GetEndTime()) + " ("
        + FormatTimeString(m_Timebar->GetEndTime() - m_Timebar->GetStartTime()) + ")";

    CRct theTimelineBounds(m_Timebar->GetTopControlBounds());
    inPoint.y = GetPosition().y - GetSize().y;
    ShowMoveableWindow(inPoint, theCommentText, theTimelineBounds);
}

//=============================================================================
/**
 * Starts the dragging of the timebar tip.
 * @param inPoint the location of the mouse.
 * @param inFlags the state of the mouse.
 */
bool CTimebarTip::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseDown(inPoint, inFlags);

    m_Timebar->OnBeginDrag();

    m_Snapper.Clear();
    m_Snapper.SetSource(m_Timebar);
    m_Timebar->GetSnappingListProvider().PopulateSnappingList(&m_Snapper);
    m_Snapper.BeginDrag(inPoint.x);

    m_IsMouseDown = true;
    m_MaybeDragStart = true;
    m_MouseDownLoc = inPoint;

    setCursorIfNotSet(CMouseCursor::CURSOR_RESIZE_LEFTRIGHT);

    // display the time range tooltip
    RefreshToolTip(inPoint);

    return true;
}

//=============================================================================
/**
 * Ends the dragging of the tip and commits the commands.
 * @param inPoint the location of the mouse.
 * @param inFlags the state of the mouse.
 */
void CTimebarTip::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    // try to prevent stuck mousetips on exceptions
    try {
        CControl::OnMouseUp(inPoint, inFlags);

        // Commit the current command so it will not be merged with drag commands if this gets
        // dragged again.
        m_Timebar->CommitTimeChange();
    } catch (...) {
    }

    m_IsMouseDown = false;
    m_MaybeDragStart = false;
    HideMoveableWindow();
    resetCursor();
}

//=============================================================================
/**
 * If the mouse is down then this handles the resizing of the timebar.
 * @param inPoint the location of the mouse.
 * @param inFlags the state of the mouse.
 */
void CTimebarTip::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseMove(inPoint, inFlags);

    // Don't show the cursor if the mouse is down from someone else.
    if (!(inFlags & CHotKeys::MOUSE_RBUTTON) && !(inFlags & CHotKeys::MOUSE_LBUTTON))
        setCursorIfNotSet(CMouseCursor::CURSOR_RESIZE_LEFTRIGHT);

    if (m_IsMouseDown) {
        if (m_MaybeDragStart) {
            // Dragging in the first 5 pixels will be ignored to avoid unconsciously accidental
            // moves
            CPt theDragDistance = inPoint - m_MouseDownLoc;
            if (theDragDistance.x * theDragDistance.x + theDragDistance.y * theDragDistance.y <= 25)
                return;

            m_MaybeDragStart = false;
        }

        // Figure out which method to call based on which tip we are.
        if (m_IsLeft) {
            long theNewTime = m_Snapper.ProcessDrag(m_Timebar->GetStartTime(), inPoint.x, inFlags);
            m_Timebar->ResizeTimebarLeftTo(theNewTime);
        } else {
            long theNewTime = m_Snapper.ProcessDrag(m_Timebar->GetEndTime(), inPoint.x, inFlags);
            m_Timebar->ResizeTimebarRightTo(theNewTime);
        }

        // display the time range tooltip
        RefreshToolTip(inPoint);
    }
}

//=============================================================================
/**
 * Resets the cursor back to normal.
 * @param inPoint the location of the mouse.
 * @param inFlags the state of the mouse/modifier buttons.
 */
void CTimebarTip::OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOut(inPoint, inFlags);

    resetCursor();
}

//=============================================================================
/**
 * Draws timebar handles if necessary.
 */
void CTimebarTip::Draw(CRenderer *inRenderer)
{
    if (m_HasHandle) { // to show or not is based on Studio preferences
        bool theShowHandle =
            CPreferences::GetUserPreferences("Timeline").GetValue("ShowTimebarHandles", false);
        if (theShowHandle) {
            if (IsEnabled())
                inRenderer->DrawBitmap(CPt(0, 0), m_HandleImage);
            else
                inRenderer->DrawBitmap(CPt(0, 0), m_HandleDisabledImage);
        }
    }
}

void CTimebarTip::ShowHandles(bool inShowHandles)
{
    m_HasHandle = inShowHandles;

    // If this tip can have a handle
    if (m_HasHandle) {
        if (!m_HandleImage) {
            // If this is a tip on the left side, load the images for the left side
            const char *theBitMap =
                (m_IsLeft) ? "timebarhandle-left.png" : "timebarhandle-right.png";
            m_HandleImage = CResourceCache::GetInstance()->GetBitmap(theBitMap);
        }

        if (!m_HandleDisabledImage) {
            const char *theBitMap =
                (m_IsLeft) ? "timebarhandle-disabled-left.png" : "timebarhandle-disabled-right.png";
            m_HandleDisabledImage = CResourceCache::GetInstance()->GetBitmap(theBitMap);
        }
    }
}
