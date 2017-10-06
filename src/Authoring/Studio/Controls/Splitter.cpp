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
#include "Splitter.h"
#include "ControlData.h"

using namespace Q3DStudio;
using namespace Q3DStudio::Control;
//=============================================================================
/**
 * Constructor, creates a Splitter.
 */
CSplitter::CSplitter()
    : m_SplitDirection(SPLIT_VERTICAL)
    , m_SplitLocation(0)
    , m_InRecalcLayoutFlag(false)
{
    m_SplitMinMargin = 10;
    m_SplitMaxMargin = 10;
    m_SplitBar = new CSplitBar(this);
    AddChild(m_SplitBar);
}

CSplitter::~CSplitter()
{
    RemoveChild(m_SplitBar);
    delete m_SplitBar;
}

//=============================================================================
/**
 * Set the size of this splitter.
 * @param inSize the size of this splitter.
 */
void CSplitter::SetSize(CPt inSize)
{
    if (inSize != GetSize()) {
        CControl::SetSize(inSize);

        RecalcLayout();
    }
}

void CSplitter::SetLayout(CPt inSize, CPt inPosition)
{
    CControl::SetLayout(inSize, inPosition);
    RecalcLayout();
}

//=============================================================================
/**
 * Get the preferred size of this splitter.
 * The preferred size is the sum of both the panes.
 * @return the preferred size of this splitter.
 */
CPt CSplitter::GetPreferredSize()
{
    CPt theSize = CControl::GetPreferredSize();

    if (m_SplitDirection == SPLIT_VERTICAL) {
        theSize.x = m_SplitBar->GetWidth();

        // If vertical then sum up x's
        ControlGraph::SIterator thePos = GetChildren();
        for (; thePos.HasNext(); ++thePos) {
            CPt thePrefSize = (*thePos)->GetPreferredSize();
            theSize.x += thePrefSize.x;
        }
    } else {
        theSize.y = m_SplitBar->GetWidth();

        // If horizontal then sum up y's
        ControlGraph::SIterator thePos = GetChildren();
        for (; thePos.HasNext(); ++thePos) {
            CPt thePrefSize = (*thePos)->GetPreferredSize();
            theSize.y += thePrefSize.y;
        }
    }

    return theSize;
}

//=============================================================================
/**
 * Get the minimum allowable size of this splitter.
 * The minimum size is the sum of both the minimum sizes in the split direciton
 * and the largest minimum in the non-split direction.
 * @return the minimum size of this control.
 */
CPt CSplitter::GetMinimumSize()
{
    CPt theSize(0, 0);

    if (m_SplitDirection == SPLIT_VERTICAL) {
        theSize.x += m_SplitBar->GetWidth();

        // Sum up in the split direction and take max min size in non-split direction
        ControlGraph::SIterator thePos = GetChildren();
        for (; thePos.HasNext(); ++thePos) {
            CPt theMinSize = (*thePos)->GetMinimumSize();
            theSize.x += theMinSize.x;

            if (theSize.y < theMinSize.y) {
                theSize.y = theMinSize.y;
            }
        }
    } else {
        theSize.y += m_SplitBar->GetWidth();

        // Sum up in the split direction and take max min size in non-split direction.
        ControlGraph::SIterator thePos = GetChildren();
        for (; thePos.HasNext(); ++thePos) {
            CPt theMinSize = (*thePos)->GetMinimumSize();
            theSize.y += theMinSize.y;

            if (theSize.x > theMinSize.x) {
                theSize.x = theMinSize.x;
            }
        }
    }

    return theSize;
}

//=============================================================================
/**
 * Get the maximum size that this control should be.
 * The maximum size is the sum of both the maximum sizes in the split direction
 * and the smalled maximum size in the non-split direction.
 * @return the maximum size of this control.
 */
CPt CSplitter::GetMaximumSize()
{
    CPt theSize(0, 0);

    if (m_SplitDirection == SPLIT_VERTICAL) {
        theSize.x += m_SplitBar->GetWidth();
        theSize.y = LONG_MAX;

        // Sum up the split direction and take min max size in non-split direction.
        ControlGraph::SIterator thePos = GetChildren();
        // Skip the separator
        ++thePos;
        for (; thePos.HasNext(); ++thePos) {
            CPt theMaxSize = (*thePos)->GetMaximumSize();
            theSize.x += theMaxSize.x;

            if (theSize.y > theMaxSize.y) {
                theSize.y = theMaxSize.y;
            }
        }
    } else {
        theSize.y += m_SplitBar->GetWidth();
        theSize.x = LONG_MAX;

        // Sum up the split direction and take min max size in the non-split direction.
        ControlGraph::SIterator thePos = GetChildren();
        // Skip the separator.
        ++thePos;
        for (; thePos.HasNext(); ++thePos) {
            CPt theMaxSize = (*thePos)->GetMaximumSize();
            theSize.y += theMaxSize.y;

            if (theSize.x > theMaxSize.x) {
                theSize.x = theMaxSize.x;
            }
        }
    }

    return theSize;
}

//=============================================================================
/**
 * Set the location of the splitter bar.
 * @param inSplitLocation the location of the splitter bar, in pixels from the right/top.
 */
void CSplitter::SetSplitLocation(long inSplitLocation)
{
    ControlGraph::SIterator theChildren = GetChildren();

    std::shared_ptr<CControlData> theControl1;
    std::shared_ptr<CControlData> theControl2;
    ++theChildren;
    if (theChildren.HasNext()) {
        theControl1 = *theChildren;

        ++theChildren;
        if (theChildren.HasNext()) {
            theControl2 = *theChildren;
        }
    }
    if (m_SplitDirection == SPLIT_VERTICAL && theControl1) {
        CPt theControl1Min = theControl1->GetMinimumSize();
        if (theControl1Min.x > inSplitLocation)
            inSplitLocation = theControl1Min.x;
    } else if (theControl2) {
        CPt theControl1Min = theControl1->GetMinimumSize();
        if (theControl1Min.y > inSplitLocation && theControl2)
            inSplitLocation = theControl1Min.y;
    }
    m_SplitLocation = inSplitLocation;

    RecalcLayout();
}

//=============================================================================
/**
 * Get the location of the splitter bar.
 * @return the location of the splitter bar, in pixels from the right/top.
 */
long CSplitter::GetSplitLocation() const
{
    long theSplitLocation = m_SplitLocation;

    // Enforce the max margin and then the min margin (in that order)
    // this is done on the get so that internally we maintain
    // the correct location of the splitter
    if (m_SplitDirection == SPLIT_VERTICAL) {
        if (m_SplitLocation > GetSize().x - m_SplitMaxMargin /*- m_SplitBar->GetWidth( )*/)
            theSplitLocation = GetSize().x - m_SplitMaxMargin /*- m_SplitBar->GetWidth( )*/;
    } else {
        if (m_SplitLocation > GetSize().y - m_SplitMaxMargin)
            theSplitLocation = GetSize().y - m_SplitMaxMargin;
    }

    if (m_SplitLocation < m_SplitMinMargin)
        theSplitLocation = m_SplitMinMargin;

    return theSplitLocation;
}

//=============================================================================
/**
 * Set the minimum and maximum split location.  This will limit the splitter
 * left and right or up and down.
 * @param inSplitMinMargin the number of pixels from the left/top of the pane
 * @param inSplitMaxMargin the number of pixels from the right/bottom of the pane.
 */
void CSplitter::SetSplitLimits(long inSplitMinMargin, long inSplitMaxMargin)
{
    m_SplitMinMargin = inSplitMinMargin;
    m_SplitMaxMargin = inSplitMaxMargin;
}

//=============================================================================
/**
 * Set the direction that this is being split in.
 * @param inSplitDirection the direction that this is being split in.
 */
void CSplitter::SetSplitDirection(CSplitterBase::ESplitDirection inSplitDirection)
{
    m_SplitDirection = inSplitDirection;
    RecalcLayout();
}

//=============================================================================
/**
 * Get the direction that this is being split in.
 * @return the direction that this is being split in.
 */
CSplitterBase::ESplitDirection CSplitter::GetSplitDirection() const
{
    return m_SplitDirection;
}

//=============================================================================
/**
 * Add a child to this splitter.
 * If more than 2 children are added then this will pop off the last one to be
 * added before inControl.
 * @param inControl the control to add to this.
 * @param inInsertAfter the position to inster the control.
 */
void CSplitter::AddChild(CControl *inControl, CControl *inInsertAfter /*=nullptr*/)
{
    CControl::AddChild(inControl, inInsertAfter);

    // If there are more than 3 objects then ditch the last one, allows the insert after to still
    // work.
    if (GetChildCount() > 3)
        RemoveChild(GetReverseChildren().GetCurrent()->GetControl());

    RecalcLayout();
}

void CSplitter::RecalcLayout()
{
    CPt thePoint(0, 0);
    CPt theSize = GetSize();

    std::shared_ptr<CControlData> theControl1;
    std::shared_ptr<CControlData> theControl2;

    long theSplitLocation = GetSplitLocation();

    ControlGraph::SIterator theChildren = GetChildren();
    ++theChildren;
    if (theChildren.HasNext()) {
        theControl1 = *theChildren;

        ++theChildren;
        if (theChildren.HasNext()) {
            theControl2 = *theChildren;
        }
    }

    // Prevent OnChildSizeChanged from screwing with us
    m_InRecalcLayoutFlag = true;

    if (theControl2 != nullptr) {
        CPt theControl1Max = theControl1->GetMaximumSize();
        CPt theControl2Max = theControl2->GetMaximumSize();

        if (m_SplitDirection == SPLIT_VERTICAL) {
            CPt theControl1Min = theControl1->GetMinimumSize();
            if (theControl1Min.x > theSplitLocation) {
                theSplitLocation = theControl1Min.x;
            }
            theControl1->SetPosition(CPt(0, 0));
            theControl1->SetSize(CPt(theSplitLocation, min(theSize.y, theControl1Max.y)));

            m_SplitBar->SetPosition(CPt(theSplitLocation, 0));
            m_SplitBar->SetSize(CPt(m_SplitBar->GetWidth(), theSize.y));

            theControl2->SetPosition(CPt(theSplitLocation + m_SplitBar->GetWidth(), 0));
            theControl2->SetSize(CPt(theSize.x - (theSplitLocation + m_SplitBar->GetWidth()),
                                     min(theSize.y, theControl2Max.y)));
        } else {
            CPt theControl1Min = theControl1->GetMinimumSize();
            if (theControl1Min.y > theSplitLocation) {
                theSplitLocation = theControl1Min.y;
            }
            theControl1->SetPosition(CPt(0, 0));
            theControl1->SetSize(CPt(min(theSize.x, theControl1Max.x), theSplitLocation));

            m_SplitBar->SetPosition(CPt(0, theSplitLocation));
            m_SplitBar->SetSize(CPt(theSize.x, m_SplitBar->GetWidth()));

            theControl2->SetPosition(CPt(0, theSplitLocation + m_SplitBar->GetWidth()));
            theControl2->SetSize(CPt(min(theSize.x, theControl2Max.x),
                                     theSize.y - (theSplitLocation + m_SplitBar->GetWidth())));
        }
    }

    m_InRecalcLayoutFlag = false;

    Invalidate();
}

void CSplitter::OnChildSizeChanged(CControl *)
{
    if (m_InRecalcLayoutFlag == false) {
        std::shared_ptr<CControlData> theControl1;
        std::shared_ptr<CControlData> theControl2;

        ControlGraph::SIterator theChildren = GetChildren();
        ++theChildren;
        if (theChildren.HasNext()) {
            theControl1 = *theChildren;

            ++theChildren;
            if (theChildren.HasNext()) {
                theControl2 = *theChildren;
            }
        }

        CPt theSize = GetSize();
        if (theControl2 != nullptr) {
            CPt theSize1 = theControl1->GetSize();
            CPt theSize2 = theControl2->GetSize();
            if (m_SplitDirection == SPLIT_VERTICAL)
                theSize.y = max(theSize1.y, theSize2.y);
            else
                theSize.x = max(theSize1.x, theSize2.x);
        }

        SetSize(theSize);
        SetMaximumSize(theSize);

        RecalcLayout();
    }

    // Notify the folks
    if (GetParent() != nullptr)
        GetParent()->OnChildSizeChanged(this);
}

void CSplitter::SetSplitBarWidth(const long inWidth)
{
    m_SplitBar->SetWidth(inWidth);
}