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
#include "ScrollerBar.h"
#include "Scroller.h"
#include "Renderer.h"
#include "ButtonControl.h"
#include "CoreUtils.h"
#include "ScrollerBackground.h"
#include "ScrollerThumb.h"
#include "StudioPreferences.h"
#include "Renderer.h"
#include "SystemPreferences.h"
#include "StudioUtils.h"
#include "ScrollerButtonControl.h"
#include "MasterP.h"

//==============================================================================
//	Static variables
//==============================================================================
const long CScrollerBar::DEFAULT_WIDTH = 6;
const CColor CScrollerBar::DEFAULT_COLOR = CColor(196, 194, 189);
const CColor CScrollerBar::SCROLLER_TOP = CColor(165, 162, 161);

IMPLEMENT_OBJECT_COUNTER(CScrollerBar)

//=============================================================================
/**
 * Constructor
 * @param inScroller the scroller on which this is operating.
 * @param inCreateImmediately false for lazy construction, only for subclasses.
 */
CScrollerBar::CScrollerBar(CScroller *inScroller, bool inCreateImmediately /* = true */)
{
    ADDTO_OBJECT_COUNTER(CScrollerBar)

    m_Scroller = inScroller;
    SetFlowDirection(FLOW_HORIZONTAL);

    if (inCreateImmediately)
        Initialize();
}

//=============================================================================
/**
 * Destructor
 */
CScrollerBar::~CScrollerBar()
{
    REMOVEFROM_OBJECT_COUNTER(CScrollerBar)
}

//=============================================================================
/**
 * Initialize this scroller bar.
 * This is used for lazy construction of this object by subclasses. If a subclass
 * specifies lazy construction in the constructor of this class then it must call
 * this to put it into a valid state.
 */
void CScrollerBar::Initialize()
{
    m_ScrollerThumb = CreateThumb();

    // First button
    m_ButtonBackward = new CScrollerButtonControl(this, CScrollerButtonControl::BACKWARD);

    // Background
    m_Background = new CScrollerBackground(CStudioPreferences::GetScrollBGColor());
    m_Background->SetScrollerBar(this);
    m_Background->AddChild(m_ScrollerThumb);

    m_ScrollerThumb->SetPosition(CPt(0, 0));

    // Second button (either right or bottom)
    m_ButtonForward = new CScrollerButtonControl(this, CScrollerButtonControl::FORWARD);

    // Determine the placement of the scroll bar arrows based on the system preference
    if (CSystemPreferences::AreScrollArrowsAdjacent()) {
        // Scroll arrows go together at one end of the bar
        AddChild(m_Background);
        AddChild(m_ButtonBackward);
        AddChild(m_ButtonForward);
    } else {
        // Scroll arrows go on each end of the scroll bar
        AddChild(m_ButtonBackward);
        AddChild(m_Background);
        AddChild(m_ButtonForward);
    }
}

//=============================================================================
/**
 * Virtual class to create the thumb class.
 * This is used to allow sub-classes to specialize the scroller thumb that they
 * are using.
 * @return the thumb that was created.
 */
CControl *CScrollerBar::CreateThumb()
{
    return new CScrollerThumb(this);
}

//=============================================================================
/**
 * Set the orientation of this scroller bar.
 * This determines which way this is scrolling.
 */
void CScrollerBar::SetOrientation(EOrientation inOrientation)
{
    m_Orientation = inOrientation;
    m_Background->SetOrientation(inOrientation);

    if (m_Orientation == VERTICAL)
        SetFlowDirection(FLOW_VERTICAL);
    else
        SetFlowDirection(FLOW_HORIZONTAL);
}

//=============================================================================
/**
 * Get the orientation of this scroller bar.
 */
CScrollerBar::EOrientation CScrollerBar::GetOrientation()
{
    return m_Orientation;
}

//=============================================================================
/**
 * Get the minimum size of this scroller bar.
 * The minumum size is the width or height of both of it's buttons (depending
 * on orientation.
 * @return the minimum allowable size of this bar.
 */
CPt CScrollerBar::GetMinimumSize()
{
    if (GetOrientation() == VERTICAL) {
        return CPt(DEFAULT_WIDTH, DEFAULT_WIDTH * 2);
    } else {
        return CPt(DEFAULT_WIDTH * 2, DEFAULT_WIDTH);
    }
}

//=============================================================================
/**
 * Set the size of this scroller bar.
 */
void CScrollerBar::SetSize(CPt inSize)
{
    CFlowLayout::SetSize(inSize);

    RepositionThumb();
}

//=============================================================================
/**
 * Recalculate the size and position of the thumb control.
 */
void CScrollerBar::RepositionThumb()
{
    long theBarLen;

    if (GetOrientation() == VERTICAL) {
        float thePercentage =
            (float)m_Scroller->GetVisibleSize().y / (float)m_Scroller->GetContaineeSize().y;
        theBarLen = (long)(thePercentage * ((float)m_Background->GetSize().y));

        if (theBarLen < CScrollerThumb::MIN_LENGTH)
            theBarLen = CScrollerThumb::MIN_LENGTH;

        CPt theSize(GetMinimumSize().x, theBarLen);

        m_ScrollerThumb->SetSize(theSize);

        float theVisPosY = (float)(m_Scroller->GetVisiblePosition().y);
        float theMaxPosY = (float)(m_Scroller->GetMaxVisiblePosition().y);
        long thePosY = 0;
        if (theMaxPosY != 0)
            thePosY = ::dtol((theVisPosY / theMaxPosY)
                             * (m_Background->GetSize().y - m_ScrollerThumb->GetSize().y));

        m_ScrollerThumb->SetPosition(CPt(0, thePosY));
    } else {
        float thePercentage =
            (float)m_Scroller->GetVisibleSize().x / (float)m_Scroller->GetContaineeSize().x;
        theBarLen = ::dtol(thePercentage * ((float)m_Background->GetSize().x));
        if (theBarLen < CScrollerThumb::MIN_LENGTH)
            theBarLen = CScrollerThumb::MIN_LENGTH;

        CPt theSize(theBarLen, GetMinimumSize().y);

        m_ScrollerThumb->SetSize(theSize);

        float theVisPosX = (float)(m_Scroller->GetVisiblePosition().x);
        float theMaxPosX = (float)(m_Scroller->GetMaxVisiblePosition().x);
        long thePosX = 0;
        if (theMaxPosX != 0)
            thePosX = ::dtol((theVisPosX / theMaxPosX)
                             * (m_Background->GetSize().x - m_ScrollerThumb->GetSize().x));

        m_ScrollerThumb->SetPosition(CPt(thePosX, 0));
    }

    Invalidate();
}

IScroller *CScrollerBar::GetScroller()
{
    return m_Scroller;
};

//=============================================================================
/**
 * Called by the ScrollerThumb to reposition itself.
 * This is used to reposition the thumb, the positions here are used to derive
 * the position of the visible window, so that this provides absolute positioning
 * of the thumb.
 * @param inPosition the new position of the thumb.
 */
void CScrollerBar::SetBarPosition(long inPosition)
{
    long theAvailableSpace;
    if (GetOrientation() == HORIZONTAL) {
        theAvailableSpace = m_Background->GetSize().x - m_ScrollerThumb->GetSize().x;
    } else {
        theAvailableSpace = m_Background->GetSize().y - m_ScrollerThumb->GetSize().y;
    }

    if (inPosition > theAvailableSpace)
        inPosition = theAvailableSpace;
    if (inPosition < 0)
        inPosition = 0;

    CPt theVisPos = m_Scroller->GetVisiblePosition();
    if (GetOrientation() == HORIZONTAL) {
        m_ScrollerThumb->SetPosition(CPt(inPosition, 0));
        double theMaxPosX = (double)(m_Scroller->GetMaxVisiblePosition().x);
        double theScrollDiff = (double)(m_Background->GetSize().x - m_ScrollerThumb->GetSize().x);
        theVisPos.x = ::dtol((double)inPosition / theScrollDiff * theMaxPosX);
    } else {
        m_ScrollerThumb->SetPosition(CPt(0, inPosition));
        double theMaxPosY = (double)(m_Scroller->GetMaxVisiblePosition().y);
        double theScrollDiff = (double)(m_Background->GetSize().y - m_ScrollerThumb->GetSize().y);
        theVisPos.y = ::dtol((double)inPosition / theScrollDiff * theMaxPosY);
    }
    m_Scroller->SetVisiblePosition(theVisPos);

    RepositionThumb();

    Invalidate();
}

//=============================================================================
/**
 * Get the current position of the thumb.
 * @return the current position of the thumb.
 */
long CScrollerBar::GetBarPosition()
{
    if (GetOrientation() == HORIZONTAL)
        return m_ScrollerThumb->GetPosition().x;
    else
        return m_ScrollerThumb->GetPosition().y;
}

//=============================================================================
/**
 * Get the ScrollerThumb.
 * @return the scroller thumb.
 */
CControl *CScrollerBar::GetThumb()
{
    return m_ScrollerThumb;
}

//=============================================================================
/**
 * Get the background of the thumb.
 * @return the background of the thumb.
 */
CControl *CScrollerBar::GetThumbBackground()
{
    return m_Background;
}
