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
#include "CommentEdit.h"
#include "Renderer.h"
#include "ColorControl.h"
#include "Bindings/ITimelineTimebar.h"
#include "CoreUtils.h"
//=============================================================================
/**
 * Constructor
 */
CCommentEdit::CCommentEdit(ITimelineTimebar *inTimelineItemTimebar)
    : m_TimelineItemTimebar(inTimelineItemTimebar)
    , m_IsSelected(false)
{
    AddCommitListener(this);
}

//=============================================================================
/**
 * Destructor
 */
CCommentEdit::~CCommentEdit()
{
    RemoveCommitListener(this);
}

//=============================================================================
/**
 * Called when a property changes.
 * @param inProperty the property that was changed
 * @param inIsVisibleChange true if this is a change that effects the property's visibility in the
 * Timeline
 */
void CCommentEdit::OnSetData(CControl *inControl)
{
    if (inControl == this && m_TimelineItemTimebar) {
        m_TimelineItemTimebar->SetTimebarComment(GetString());
    }
}

//=============================================================================
/**
 * Called when this control receives a double click, edit the comment.
 *
 * @param inPoint the point where the double click occured
 * @param inFlags the flags at the time of the click
 */
bool CCommentEdit::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inPoint);
    Q_UNUSED(inFlags);

    if (GetDisplayString().Length() > 0) {
        DoChangeComment();
        return true;
    }
    return false;
}

//=============================================================================
/**
 * Edit the Comment
 *
 */
void CCommentEdit::DoChangeComment()
{
    SetEditMode(true);
    SelectAllText();
}

//=============================================================================
/**
 * Called when this control receives a mouse down, don't do anything unless we're in edit mode
 *
 * @param inPoint the point where the click occured
 * @param inFlags the flags at the time of the click
 */
bool CCommentEdit::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theRetVal = false;
    if (GetEditMode()) {
        theRetVal = CTextEditInPlace::OnMouseDown(inPoint, inFlags);
    }
    return theRetVal;
}

//=============================================================================
/**
 * Called when this control loses focus
 */
void CCommentEdit::OnLoseFocus()
{
    CStringEdit::OnLoseFocus();
    SetEditMode(false);
}

//=============================================================================
/**
 * Called when this control gains focus, don't do anytihng
 */
void CCommentEdit::OnGainFocus()
{
}

//=============================================================================
/**
 * Override for the draw, don't set the size of this control here since we are in
 * a comment and wouldn't want the size changing
 */
void CCommentEdit::Draw(CRenderer *inRenderer)
{
    CPt theRectPoint = CPt(::dtol(CalculateCharWidths(inRenderer) + GetRightBuffer()), GetSize().y);
    CPt theSize = GetSize();
    if (theSize.x < theRectPoint.x)
        theRectPoint.x = theSize.x;
    CRct theRect = CRct(theRectPoint);
    ::CColor theOutlineColor = ::CColor(0, 0, 0);
    bool theFillFlag = m_FillBackground;

    if (theFillFlag && !GetEditMode())
        SetFillBackground(false);

    if (GetEditMode())
        SetTextColor(::CColor(0, 0, 0));

    inRenderer->PushClippingRect(theRect);
    CStringEdit::Draw(inRenderer);
    inRenderer->PopClippingRect();

    if (!GetEditMode())
        SetFillBackground(theFillFlag);

    if (GetEditMode())
        inRenderer->DrawRectOutline(theRect, theOutlineColor, theOutlineColor, theOutlineColor,
                                    theOutlineColor);
}

//=============================================================================
/**
 * Called when the timebar comment or the time bar color changes on an asset
 */
void CCommentEdit::RefreshMetaData()
{
    m_Color = ::CColor(m_TimelineItemTimebar->GetTimebarColor());
    CalculateTextColor();
    SetData(m_TimelineItemTimebar->GetTimebarComment());

    Invalidate();
}

//=============================================================================
/**
 * Calculates the text color based on whether the object is selected or not
 */
void CCommentEdit::CalculateTextColor()
{
    ::CColor theColor = m_Color;
    float theLuminance = theColor.GetLuminance();
    if (m_IsSelected) {
        theLuminance = theLuminance * 0.8f;
    }
    // Duplicated Code to check luminance when the timebar changes color

    if (theLuminance < 0.5) {
        SetTextColor(::CColor(255, 255, 255));
    } else {
        SetTextColor(::CColor(0, 0, 0));
    }
}

//=============================================================================
/**
 * Sets this object as selected
 * @param inPoint the point to check
 */
void CCommentEdit::SetSelected(bool inState)
{
    m_IsSelected = inState;
    CalculateTextColor();
}

//=============================================================================
/**
 * Overrides the hit test for the case when the click occurs in the object, but outside the text
 * @param inPoint the point to check
 */
bool CCommentEdit::HitTest(const CPt &inPoint) const
{
    // rp this seems wrong but apparently it magically works
    bool theRetVal = false;
    if (inPoint.x
        < m_TotalCharWidth + GetRightBuffer() + 2 * CStudioPreferences::GetTimebarTipSize())
        theRetVal = CTextEditInPlace::HitTest(inPoint);
    return theRetVal;
}
