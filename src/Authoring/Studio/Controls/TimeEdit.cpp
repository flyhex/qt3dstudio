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

#include "CoreUtils.h"
#include "TimeEdit.h"
#include "Renderer.h"
#include "IDoc.h"
#include "TimeEditDlg.h"

long DELIMITER_SIZE = 2;
long MINUTES_SIZE = 28;
long SECONDS_SIZE = 20;
long MILLIS_SIZE = 28;

CTimeEdit::CTimeEdit(IDoc *inDoc)
    : m_Time(0)
    , m_MinimumTime(0)
    , m_MaximumTime(LONG_MAX)
{
    AddChild(&m_Minutes);
    AddChild(&m_Seconds);
    AddChild(&m_Millis);
    m_Doc = inDoc;
}

CTimeEdit::~CTimeEdit()
{
}
long CTimeEdit::GetTime()
{
    return m_Time;
}

//=============================================================================
/**
  * OnMouseDown: Displays a time edit dialog, when the user clicks on the time edit box.
  * @param inPoint stores the point clicked
  * @param inFlags stores the control keys pressed
  */
bool CTimeEdit::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inPoint);
    Q_UNUSED(inFlags);

    CTimeEditDlg theTimeEditDlg;
    theTimeEditDlg.ShowDialog(m_Time, 0, m_Doc, PLAYHEAD);
    return true;
}

void CTimeEdit::SetSize(CPt inSize)
{
    CControl::SetSize(inSize);

    inSize.x -= DELIMITER_SIZE * 2;

    m_Minutes.SetPosition(CPt(0, 0));
    m_Minutes.SetAbsoluteSize(CPt(MINUTES_SIZE, inSize.y));
    m_Minutes.SetAlignment(CTextEdit::RIGHT);
    m_Minutes.SetNumDecimalPlaces(0);
    m_Minutes.AddCommitListener(this);
    m_Minutes.SetMin(0);

    m_Seconds.SetPosition(CPt(MINUTES_SIZE + DELIMITER_SIZE, 0));
    m_Seconds.SetAbsoluteSize(CPt(SECONDS_SIZE, inSize.y));
    m_Seconds.SetAlignment(CTextEdit::LEFT);
    m_Seconds.SetFixedPlaces(2);
    m_Seconds.SetNumDecimalPlaces(0);
    m_Seconds.AddCommitListener(this);
    m_Seconds.SetMin(0);

    m_Millis.SetPosition(CPt(MINUTES_SIZE + SECONDS_SIZE + DELIMITER_SIZE * 2, 0));
    m_Millis.SetAbsoluteSize(CPt(MILLIS_SIZE, inSize.y));
    m_Millis.SetAlignment(CTextEdit::LEFT);
    m_Millis.SetFixedPlaces(3);
    m_Millis.SetNumDecimalPlaces(0);
    m_Millis.AddCommitListener(this);
    m_Millis.SetMin(0);
}

//=============================================================================
/**
 * Draw this time.
 * @param inRenderer the renderer to draw to.
 */

void CTimeEdit::Draw(CRenderer *inRenderer)
{
    // Fill in the background, otherwise we'll have junk left over from before
    CRct theRect(GetSize());
    inRenderer->FillSolidRect(theRect, m_BackgroundColor);
    const float yPos = GetSize().y - 3;
    inRenderer->PushPen(CStudioPreferences::GetRulerTickColor());
    inRenderer->DrawText(static_cast<float>(m_Seconds.GetPosition().x - DELIMITER_SIZE - 1), yPos,
                         ":");
    inRenderer->DrawText(static_cast<float>(m_Millis.GetPosition().x - DELIMITER_SIZE - 1), yPos,
                         ".");
}

//=============================================================================
/**
 * Set the text background color for this component.
 * @param inColor the background color for this.
 */
void CTimeEdit::SetBackgroundColor(const CColor &inColor)
{
    m_BackgroundColor = inColor;

    m_Minutes.SetBGColorNoFocus(inColor);
    m_Seconds.SetBGColorNoFocus(inColor);
    m_Millis.SetBGColorNoFocus(inColor);
}

//=============================================================================
/**
 * Lame, yes. Basically gets the estimated width of this control.
 */
long CTimeEdit::GetWidth()
{
    return MINUTES_SIZE + SECONDS_SIZE + MILLIS_SIZE + DELIMITER_SIZE * 2;
}

//=============================================================================
/**
 * Set the time displayed by this control.
 * This will make sure the time is in the proper ranges then set it to the
 * value.
 * @param inTime the new time value for this control.
 */
void CTimeEdit::SetTime(long inTime)
{
    m_Time = inTime;

    m_Minutes.SetData((float)(m_Time / (1000 * 60)));
    m_Seconds.SetData((float)((m_Time / 1000) % 60));
    m_Millis.SetData((float)(m_Time % 1000));
    Invalidate();
}

void CTimeEdit::OnSetData(CControl *inControl)
{
    Q_UNUSED(inControl);

    long theTime = ::dtol(m_Millis.GetData()) + ::dtol(m_Seconds.GetData() * 1000)
        + ::dtol(m_Minutes.GetData() * 60 * 1000);

    m_TimeListeners.FireEvent(&CTimeEditChangeListener::OnTimeChanged, theTime);
}

//=============================================================================
/**
 * Add a listener to this control for when the time changes.
 * @param inListener the listener to be added to this control.
 */
void CTimeEdit::AddTimeChangeListener(CTimeEditChangeListener *inListener)
{
    m_TimeListeners.AddListener(inListener);
}

//=============================================================================
/**
 * Remove a time change listener from this control.
 * @param inListener the listener to be removed from this control.
 */
void CTimeEdit::RemoveTimeChangeListener(CTimeEditChangeListener *inListener)
{
    m_TimeListeners.RemoveListener(inListener);
}
