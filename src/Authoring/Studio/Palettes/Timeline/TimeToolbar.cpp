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

#include "TimeToolbar.h"
#include "Renderer.h"
#include "StudioPreferences.h"
#include "BlankControl.h"
#include "IDoc.h"

//=============================================================================
/**
 * Constructor
 */
CTimeToolbar::CTimeToolbar(IDoc *inDoc)
{
    m_Doc = inDoc;
    m_TimeEdit = new CTimeEdit(inDoc);
    m_Color = CStudioPreferences::GetBaseColor();
    m_TimeEdit->SetBackgroundColor(m_Color);
    AddChild(m_TimeEdit);

    m_TimeEdit->AddTimeChangeListener(this);
}

//=============================================================================
/**
 * Destructor
 */
CTimeToolbar::~CTimeToolbar()
{
    delete m_TimeEdit;
}

//=============================================================================
/**
 * Fills in the background color for this layout.
 */
void CTimeToolbar::Draw(CRenderer *inRenderer)
{
    // Fill in the background color and draw the child controls

    // Draw the shadow lines at the top and bottom of the layout
    CRct theRect(GetSize());
    inRenderer->FillSolidRect(theRect, m_Color);

    inRenderer->PushPen(CStudioPreferences::GetButtonShadowColor());
    inRenderer->MoveTo(CPt(0, 0));
    inRenderer->LineTo(CPt(theRect.size.x, 0));
    inRenderer->MoveTo(CPt(0, 0));
    inRenderer->LineTo(CPt(0, theRect.size.y - 2));
    inRenderer->MoveTo(CPt(0, theRect.size.y - 2));
    inRenderer->LineTo(CPt(theRect.size.x, theRect.size.y - 2));
    inRenderer->PopPen();
    inRenderer->PushPen(CStudioPreferences::GetButtonHighlightColor());
    inRenderer->MoveTo(CPt(0, theRect.size.y - 1));
    inRenderer->LineTo(CPt(theRect.size.x, theRect.size.y - 1));
    inRenderer->PopPen();
}

//=============================================================================
/**
 * Override of Control's set size to reposition the TimeEdit.
 * @param inSize the new size.
 */
void CTimeToolbar::SetSize(CPt inSize)
{
    m_TimeEdit->SetSize(CPt(m_TimeEdit->GetWidth(), inSize.y - 4));
    m_TimeEdit->SetPosition(inSize.x - m_TimeEdit->GetWidth(), 2);

    CControl::SetSize(inSize);
}

//=============================================================================
/**
 * Call from the TimelineView (or thereabouts) that the scene time changed.
 * @param inTime the new time.
 */
void CTimeToolbar::SetTime(long inTime)
{
    m_TimeEdit->SetTime(inTime);
}
//=============================================================================
/**
  * Returns the playhead time
  */
long CTimeToolbar::GetTime()
{
    return m_TimeEdit->GetTime();
}

//=============================================================================
/**
 * Callback from the TimeEdit that it's time was manually changed.
 * @param inNewTime the new time.
 */
void CTimeToolbar::OnTimeChanged(long inNewTime)
{
    m_Doc->NotifyTimeChanged(inNewTime);
}