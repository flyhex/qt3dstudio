/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#include "FlowLayout.h"
#include "Renderer.h"
#include "MasterP.h"
#include "ControlData.h"
#include "CoreUtils.h"
#include "Qt3DSMath.h"
// using namespace Q3DStudio;  <-- Do not do this here because it will conflict with CList and make
// the template generator go blah

using namespace Q3DStudio::Control;

//=============================================================================
/**
 * Checks to see if this has hit it's max X
 */
bool CanBeExpandedX(CControl *inControl)
{
    bool theRetVal = false;
    CPt theSize = inControl->GetSize();
    CPt theMaxSize = inControl->GetMaximumSize();
    if (theSize.x < theMaxSize.x) {
        theRetVal = true;
    }
    return theRetVal;
}

//=============================================================================
/**
 * Checks to see if this has hit it's max Y
 */
bool CanBeExpandedY(CControl *inControl)
{
    bool theRetVal = false;
    CPt theSize = inControl->GetSize();
    CPt theMaxSize = inControl->GetMaximumSize();
    if (theSize.y < theMaxSize.y) {
        theRetVal = true;
    }
    return theRetVal;
}

IMPLEMENT_OBJECT_COUNTER(CFlowLayout)

//=============================================================================
/**
 * Constructs a new FlowLayout flowing vertically (down).
 */
CFlowLayout::CFlowLayout(CControl *inControl, bool inUseControl)
    : m_FlowDirection(FLOW_VERTICAL)
    , m_VerticalAlignment(ALIGN_TOP)
    , m_HorizontalAlignment(ALIGN_LEFT)
    , m_ResizingChildren(false)
    , m_LeftMargin(0)
    , m_RightMargin(0)
    , m_TopMargin(0)
    , m_BottomMargin(0)
    , m_ChildGap(0)
    , m_DebugFlag(false)
    , m_AutoMin(true)
    , m_SuspendRecalcLayout(false)
{
    ADDTO_OBJECT_COUNTER(CFlowLayout)

    m_BlankDisable = false;
    m_HasUnusedSpace = false;

    if (inControl != nullptr) {
        m_BlankControl = inControl;
    } else if (inUseControl) {
        m_BlankControl = new CBlankControl();
    } else {
        m_BlankControl = nullptr;
    }

    SetMaximumSize(CPt(LONG_MAX, LONG_MAX));
}

//=============================================================================
/**
 * Destructor
 */
CFlowLayout::~CFlowLayout()
{
    delete m_BlankControl;

    REMOVEFROM_OBJECT_COUNTER(CFlowLayout)
}

//=============================================================================
/**
 * The left margin is the gap on the left side of the flow layout where no
 * controls are drawn.  The default left margin of a flow layout is zero.  If
 * you have a horizontal flow layout with left alignment, you can use the margin
 * to push all the child controls over by a fixed amount.
 * @param inMargin Width in pixels of the left margin
 * @return the previous width of the left margin
 */
long CFlowLayout::SetLeftMargin(long inMargin)
{
    long theOldMargin = m_LeftMargin;
    m_LeftMargin = inMargin;
    ResetMinMaxPref();
    RecalcLayout();
    return theOldMargin;
}

//=============================================================================
/**
 * The left margin is the gap on the left side of the flow layout where no
 * controls are drawn.  The default left margin of a flow layout is zero.  Only
 * valid with a horizontal flow layout with left alignment.
 * @return the gap on the left side of this flow, in pixels
 */
long CFlowLayout::GetLeftMargin() const
{
    return m_LeftMargin;
}

long CFlowLayout::GetRightMargin() const
{
    return m_RightMargin;
}

long CFlowLayout::GetTopMargin() const
{
    return m_TopMargin;
}

long CFlowLayout::GetBottomMargin() const
{
    return m_BottomMargin;
}

//=============================================================================
/**
 * Disables the drawing of blank rectangles in blank areas on the screen
 * @param NONE
 * @return NONE
 */
void CFlowLayout::DisableBlank()
{
    m_BlankDisable = true;
}

//=============================================================================
/**
 * The right margin is the gap on the rgiht side of the flow layout where no
 * controls are drawn.  The default right margin of a flow layout is zero.  If
 * you have a horizontal flow layout with right alignment, you can use the margin
 * to push all the child controls over by a fixed amount.
 * @param inMargin Width in pixels of the right margin
 * @return the previous width of the right margin
 */
long CFlowLayout::SetRightMargin(long inMargin)
{
    long theOldMargin = m_RightMargin;
    m_RightMargin = inMargin;
    ResetMinMaxPref();
    RecalcLayout();
    return theOldMargin;
}

//=============================================================================
/**
 * The top margin is the gap on the top of the flow layout where no controls
 * are drawn.  The default top margin of a flow layout is zero.  If you have
 * a vertical flow layout with top alignment, you can use the margin to push
 * all the child controls down by a fixed amount.
 * @param inMargin Height in pixels of the top margin
 * @return the previous height of the top margin
 */
long CFlowLayout::SetTopMargin(long inMargin)
{
    long theOldMargin = m_TopMargin;
    m_TopMargin = inMargin;
    ResetMinMaxPref();
    RecalcLayout();
    return theOldMargin;
}

//=============================================================================
/**
 * The bottom margin is the gap on the bottom of the flow layout where no
 * controls are drawn.  The default bottom margin of a flow layout is zero.  If
 * you have a vertical flow layout with bottom alignment, you can use the margin
 * to push all the child controls up by a fixed amount.
 * @param inMargin Height in pixels of the bottom margin
 * @return the previous height of the bottom margin
 */
long CFlowLayout::SetBottomMargin(long inMargin)
{
    long theOldMargin = m_BottomMargin;
    m_BottomMargin = inMargin;
    ResetMinMaxPref();
    RecalcLayout();
    return theOldMargin;
}

//=============================================================================
/**
 * This functions sets a fixed gap between children of the flow layout.
 * @param inGap pixel gap between each child of the flow layout
 * @return the previous pixel gap between children
 */
long CFlowLayout::SetGapBetweenChildren(long inGap)
{
    long theOldGap = m_ChildGap;
    m_ChildGap = inGap;
    ResetMinMaxPref();
    RecalcLayout();
    return theOldGap;
}

//=============================================================================
/**
 * Get the preferred size of this control.
 * The preferred size is the sum of all the preferred sizes of all the children.
 * @return the preferred size of this control.
 */
// CPt CFlowLayout::GetPreferredSize( )
//{
//	return m_PreferredSize;
//}

//=============================================================================
/**
 * Get the minimum size of this control.
 * The minimum size is the sum of all the minimum sizes of all the children in
 * the flow direction, and the largest minimum size in the non-flow direction.
 * @return the minimum size of this control.
 */
// CPt CFlowLayout::GetMinimumSize( )
//{
//	return m_MinimumSize;
//}

//=============================================================================
/**
 * Get the maximum size of this control.
 * The maximum size is the sum of all the maximum sizes of all the children in
 * the flow direction, and the smallest maximum size in the non-flow direction.
 * @return the maximum size of this control.
 */
// CPt CFlowLayout::GetMaximumSize( )
//{
//	return m_MaximumSize;
//}

//=============================================================================
/**
 * Set the direction the components are flowing.
 * Vertical means the components will be positioned one on top of the other,
 * starting at the top and flowing down.
 * Horizontal means the components will be positions side by side, starting on
 * the left and flowing right.
 * @param inFlowDirection the new flow direction.
 */
void CFlowLayout::SetFlowDirection(EFlowDirection inFlowDirection)
{
    if (inFlowDirection != m_FlowDirection) {
        m_FlowDirection = inFlowDirection;

        RecalcLayout();
    }
}

//=============================================================================
/**
 * Add a child control to this control.
 * This adds the child control to this and includes it in the layout.
 * @param inControl the control to be added.
 * @param inInsertBefore the location to insert the control, nullptr = end.
 */
void CFlowLayout::AddChild(CControl *inControl, CControl *inInsertBefore /*= nullptr*/)
{
    CControl::AddChild(inControl, inInsertBefore);

    if (GetParent() != nullptr) {
        ResetMinMaxPref();
        GetParent()->OnChildSizeChanged(this);
    }

    RecalcLayout();
}

//=============================================================================
/**
 * Remove a child from this control.
 * @param inControl the control to be removed.
 */
void CFlowLayout::RemoveChild(CControl *inControl)
{
    CControl::RemoveChild(inControl);

    if (GetParent() != nullptr) {
        ResetMinMaxPref();
        GetParent()->OnChildSizeChanged(this);
    }

    RecalcLayout();
}

/**
 * Sets the flag on whether to suspend the recalculation of the layout.
 * Recalc is expensive, especially when there are many children.
 * if you need to add lots of children items to this control, do this:
 * SuspendRecalcLayout( true ); AddLotsOFItems( ); SuspendRecalcLayout( false );
 * DoSomethingThatWillTriggerRecalcLayout( );
 * @param inSuspendFlag true would suspend recalc
 * @see RecalcLayout
 * @see CListBoxControl
*/
void CFlowLayout::SuspendRecalcLayout(bool inSuspendFlag)
{
    m_SuspendRecalcLayout = inSuspendFlag;
}

//=============================================================================
/**
 * Recalculate the positions of all the child controls of this control.
 * This does the work of all the layout of the children and figures out where
 * everything belongs.
 */
void CFlowLayout::RecalcLayout()
{
    UICPROFILE(RecalcLayout);

    // if you need to add lots of children items to this control, do this:
    // SuspendRecalcLayout( true ); AddLotsOFItems( ); SuspendRecalcLayout( false );
    // DoSomethingThatWillTriggerRecalcLayout( );
    if (m_SuspendRecalcLayout)
        return;

    m_ResizingChildren = true;

    CPt thePosition(0, 0);
    CPt theCurSize = GetSize();
    long theNumExpandableY = 0;
    long theNumExpandableX = 0;
    m_HasUnusedSpace = false;

    if ((m_FlowDirection == FLOW_HORIZONTAL && m_HorizontalAlignment != ALIGN_RIGHT)
        || (m_FlowDirection == FLOW_VERTICAL && m_VerticalAlignment != ALIGN_BOTTOM)) {
        thePosition.x = m_LeftMargin;
        thePosition.y = m_TopMargin;
        ControlGraph::SIterator thePos = GetChildren();
        for (; thePos.HasNext(); ++thePos) {
            std::shared_ptr<CControlData> theChild = *thePos;
            RecalcChild(theChild->GetControl(), thePosition, theNumExpandableX, theNumExpandableY);

            if (m_FlowDirection == FLOW_HORIZONTAL)
                thePosition += CPt(m_ChildGap, 0);
            else
                thePosition += CPt(0, m_ChildGap);
        }
    } else {
        //		thePosition.x = m_RightMargin;
        //		thePosition.y = m_BottomMargin;
        ControlGraph::SReverseIterator thePos = GetReverseChildren();
        for (; thePos.HasNext(); ++thePos) {
            std::shared_ptr<CControlData> theChild = *thePos;
            RecalcChild(theChild->GetControl(), thePosition, theNumExpandableX, theNumExpandableY);

            if (m_FlowDirection == FLOW_HORIZONTAL)
                thePosition -= CPt(m_ChildGap, 0);
            else
                thePosition -= CPt(0, m_ChildGap);
        }
    }

    // If there is extra space and there are items able to expand still
    if (thePosition.y < theCurSize.y && m_FlowDirection == FLOW_VERTICAL) {
        if (theNumExpandableY > 0)
            ResizeExpandableControlsY((theCurSize.y - thePosition.y), theNumExpandableY);
        else
            m_HasUnusedSpace = true;

        // SDJ: See below
        // ResetChildPositions( );
    } else if (thePosition.x < theCurSize.x && m_FlowDirection == FLOW_HORIZONTAL) {
        if (theNumExpandableX > 0)
            ResizeExpandableControlsX((theCurSize.x - thePosition.x), theNumExpandableX);
        else
            m_HasUnusedSpace = true;

        // SDJ: See below
        // ResetChildPositions( );
    }

    // SDJ: There is a case where the top margin is not properly calculated when
    // neither of the above conditions is met. Calling ResetChildPositions fixes
    // this, but might be ineffiecient. If you attempt to fix this, replace the
    // commented out calls to ResetChildPositions above.
    ResetChildPositions();

    m_ResizingChildren = false;

    Invalidate();
}

//=============================================================================
/**
 * XXX
 */
void CFlowLayout::RecalcChild(CControl *inChild, CPt &ioPosition, long &ioNumExpandableX,
                              long &ioNumExpandableY)
{
    CPt theTotalPrefSizes = GetTotalPreferredSizes();
    CPt theCurSize = GetSize();
    CPt theChildSize = theCurSize;

    theCurSize.x -= m_RightMargin;
    theCurSize.y -= m_BottomMargin;

    if (inChild->IsVisible()) {
        inChild->ResetMinMaxPref();
        CPt thePrefSize = inChild->GetPreferredSize();
        CPt theMinSize = inChild->GetMinimumSize();
        CPt theMaxSize = inChild->GetMaximumSize();

        if (theMaxSize.x > theCurSize.x - ioPosition.x)
            theMaxSize.x = theCurSize.x - ioPosition.x;
        if (theMaxSize.x < 0)
            theMaxSize.x = 0;
        if (theMaxSize.y > theCurSize.y - ioPosition.y)
            theMaxSize.y = theCurSize.y - ioPosition.y;
        if (theMaxSize.y < 0)
            theMaxSize.y = 0;

        if (m_FlowDirection == FLOW_VERTICAL) {
            CPt theChildPosition;
            theChildPosition.y = ioPosition.y;
            theChildPosition.x = 0;
            theChildSize.x = theCurSize.x;
            ///*
            // SDJ; Why is this commented out?
            switch (m_HorizontalAlignment) {
            case ALIGN_LEFT:
                theChildPosition.x = 0;
                break;

            case ALIGN_RIGHT:
                theChildPosition.x =
                    (GetSize().x - (m_LeftMargin + m_RightMargin)) - inChild->GetSize().x;
                break;

            case ALIGN_MIDDLE:
                theChildPosition.x = (GetSize().x / 2) - (inChild->GetSize().x / 2);
                break;

            case ALIGN_HORIZ_NEITHER:
            // NO BREAK
            default:
                theChildPosition.x = inChild->GetPosition().x;
                break;
            }
            //*/

            inChild->SetPosition(theChildPosition);

            if (theTotalPrefSizes.y) {
                float theModifier = ((float)thePrefSize.y) / ((float)theTotalPrefSizes.y);
                // Check to see if they have hit their min or max
                if (theMinSize.y > ::dtol(theCurSize.y * theModifier)) {
                    theChildSize.y = theMinSize.y;
                    ++ioNumExpandableY;

                } else if (theMaxSize.y < ::dtol(theCurSize.y * theModifier)) {
                    theChildSize.y = theMaxSize.y;
                } else {
                    // This item can be expanded
                    ++ioNumExpandableY;
                    theChildSize.y = inChild->GetSize().y;
                }
            }

            // If the child cannot be expanded, maintain its current width
            //			if ( !CanBeExpandedX( inChild ) )
            //					theChildSize.x = inChild->GetSize( ).x;

            theChildSize.x = Q3DStudio::MAX(theChildSize.x, theMinSize.x);
            theChildSize.y = Q3DStudio::MAX(theChildSize.y, theMinSize.y);

            theChildSize.x = Q3DStudio::MIN(theChildSize.x, theMaxSize.x);
            theChildSize.y = Q3DStudio::MIN(theChildSize.y, theMaxSize.y);

            inChild->SetSize(theChildSize);
            ioPosition.y += theChildSize.y;
        } else {
            CPt theChildPosition;
            theChildPosition.x = ioPosition.x;

            switch (m_VerticalAlignment) {
            case ALIGN_TOP:
                theChildPosition.y = m_TopMargin;
                break;

            case ALIGN_BOTTOM:
                theChildPosition.y = GetSize().y - inChild->GetSize().y;
                break;

            case ALIGN_CENTER:
                theChildPosition.y = (GetSize().y / 2) - (inChild->GetSize().y / 2);
                break;

            case ALIGN_VERT_NEITHER:
            // NO BREAK
            default:
                theChildPosition.y = inChild->GetPosition().y;
                break;
            }

            inChild->SetPosition(theChildPosition);

            if (theTotalPrefSizes.x) {
                float theModifier = ((float)thePrefSize.x) / ((float)theTotalPrefSizes.x);
                // Check to see if they have hit their min or max
                if (theMinSize.x > ::dtol(theCurSize.x * theModifier)) {
                    theChildSize.x = theMinSize.x;
                    ++ioNumExpandableX;

                } else if (theMaxSize.x < ::dtol(theCurSize.x * theModifier)) {
                    theChildSize.x = theMaxSize.x;
                } else {
                    // This item can be expanded
                    theChildSize.x = ::dtol((theCurSize.x * theModifier));
                    ++ioNumExpandableX;
                }
            }

            // If the child cannot by expanded, maintain the same height
            if (!CanBeExpandedY(inChild))
                theChildSize.y = inChild->GetSize().y;

            theChildSize.x = Q3DStudio::MAX(theChildSize.x, theMinSize.x);
            theChildSize.y = Q3DStudio::MAX(theChildSize.y, theMinSize.y);

            theChildSize.x = Q3DStudio::MIN(theChildSize.x, theMaxSize.x);
            theChildSize.y = Q3DStudio::MIN(theChildSize.y, theMaxSize.y);

            inChild->SetSize(theChildSize);
            ioPosition.x += theChildSize.x;
        }
    }
}

//==============================================================================
/**
 *	Resets all child positions accd to their size
 */
void CFlowLayout::ResetChildPositions()
{
    if ((m_FlowDirection == FLOW_HORIZONTAL && m_HorizontalAlignment != ALIGN_RIGHT)
        || (m_FlowDirection == FLOW_VERTICAL && m_VerticalAlignment != ALIGN_BOTTOM)) {
        CPt thePosition(m_LeftMargin, m_TopMargin);
        ControlGraph::SIterator thePos = GetChildren();
        for (; thePos.HasNext(); ++thePos) {
            std::shared_ptr<CControlData> theChild = (*thePos);
            if (theChild->IsVisible()) {
                CPt theChildSize = theChild->GetSize();
                if (m_FlowDirection == FLOW_VERTICAL) {
                    thePosition.x = theChild->GetPosition().x + m_LeftMargin;
                    theChild->SetPosition(thePosition);
                    thePosition.y = thePosition.y + theChildSize.y + m_ChildGap;
                } else {
                    thePosition.y = theChild->GetPosition().y + m_TopMargin;
                    theChild->SetPosition(thePosition);
                    thePosition.x = thePosition.x + theChildSize.x + m_ChildGap;
                }
            }
        }

        if (m_HasUnusedSpace && m_BlankControl) {
            CPt theBlankPosition;
            if (m_FlowDirection == FLOW_HORIZONTAL) {
                theBlankPosition.x = thePosition.x;
                theBlankPosition.y = 0;
            } else {
                theBlankPosition.x = 0;
                theBlankPosition.y = thePosition.y;
            }
            m_BlankControl->SetPosition(theBlankPosition);
            m_BlankControl->SetSize(GetSize() - theBlankPosition);
        }
    } else {
        CPt thePosition(GetSize() - CPt(m_RightMargin, m_BottomMargin));
        ControlGraph::SReverseIterator thePos = GetReverseChildren();
        for (; thePos.HasNext(); ++thePos) {
            std::shared_ptr<CControlData> theChild = (*thePos);
            if (theChild->IsVisible()) {
                CPt theChildSize = theChild->GetSize();
                if (m_FlowDirection == FLOW_VERTICAL) {
                    thePosition.x = theChild->GetPosition().x - m_RightMargin;
                    thePosition.y = thePosition.y - theChildSize.y - m_ChildGap;
                    theChild->SetPosition(thePosition);
                } else {
                    thePosition.y = theChild->GetPosition().y - m_BottomMargin;
                    thePosition.x = thePosition.x - theChildSize.x - m_ChildGap;
                    theChild->SetPosition(thePosition);
                }
            }
        }

        if (m_HasUnusedSpace && m_BlankControl) {
            CPt theBlankSize;
            if (m_FlowDirection == FLOW_HORIZONTAL) {
                theBlankSize.x = thePosition.x;
                theBlankSize.y = GetSize().y;
            } else {
                theBlankSize.x = GetSize().x;
                theBlankSize.y = thePosition.y;
            }
            m_BlankControl->SetPosition(CPt(0, 0));
            m_BlankControl->SetSize(theBlankSize);
        }
    }
}

//==============================================================================
/**
 *	Resizes any expandable Controls in the Y Dir
 *
 *	@param inExtraSpace the Amount left
 *	@param inNumExpandable used to calc percentages
 */
void CFlowLayout::ResizeExpandableControlsY(long inExtraSpace, long inNumExpandable)
{
    m_HasUnusedSpace = false;
    long theSpaceDistributed = 0;
    if (inExtraSpace / inNumExpandable < 1) {
        this->DistributeRemaining(inExtraSpace);
    } else {
        ControlGraph::SIterator thePos = GetChildren();
        long theNumExpandable = 0;

        // Loop through teh children for items that can expand
        for (; thePos.HasNext(); ++thePos) {
            std::shared_ptr<CControlData> theChild = (*thePos);

            // If this item can be expanded then expand accd to new percent
            if (CanBeExpandedY(theChild->GetControl()) && theChild->IsVisible()) {
                CPt theMaxSize = theChild->GetMaximumSize();
                CPt theChildSize = theChild->GetSize();
                float theModifier = (float)1 / inNumExpandable;
                if (theMaxSize.y < ::dtol((inExtraSpace * theModifier) + theChildSize.y)) {
                    theSpaceDistributed += theMaxSize.y - theChildSize.y;
                    theChildSize.y = theMaxSize.y;
                } else {
                    theSpaceDistributed += ::dtol((inExtraSpace * theModifier));
                    theChildSize.y += ::dtol((inExtraSpace * theModifier));
                    ++theNumExpandable;
                }

                theChild->SetSize(theChildSize);
            }
        }

        // If there is extra space and there are items able to expand still
        if ((inExtraSpace - theSpaceDistributed) > 0) {
            if (theNumExpandable > 0) {
                this->ResizeExpandableControlsY((inExtraSpace - theSpaceDistributed),
                                                theNumExpandable);
            } else {
                m_HasUnusedSpace = true;
            }
        }
    }
}

//==============================================================================
/**
 *	Resizes any expandable Controls in the X Dir
 *
 *	@param inExtraSpace the Amount left
 *	@param inNumExpandable used to calc percentages
 */
void CFlowLayout::ResizeExpandableControlsX(long inExtraSpace, long inNumExpandable)
{
    m_HasUnusedSpace = false;
    long theSpaceDistributed = 0;
    if (inExtraSpace / inNumExpandable < 1) {
        this->DistributeRemaining(inExtraSpace);
    } else {
        long theNumExpandable = 0;
        ControlGraph::SIterator thePos = GetChildren();
        // Loop through teh children for items that can expand
        for (; thePos.HasNext(); ++thePos) {
            std::shared_ptr<CControlData> theChild = (*thePos);

            // If this item can be expanded then expand accd to new percent
            if (CanBeExpandedX(theChild->GetControl()) && theChild->IsVisible()) {
                CPt theMaxSize = theChild->GetMaximumSize();
                CPt theChildSize = theChild->GetSize();
                float theModifier = (float)1 / inNumExpandable;
                if (theMaxSize.x < ::dtol((inExtraSpace * theModifier) + theChildSize.x)) {
                    theSpaceDistributed += theMaxSize.x - theChildSize.x;
                    theChildSize.x = theMaxSize.x;
                } else {
                    theSpaceDistributed += ::dtol((inExtraSpace * theModifier));
                    theChildSize.x += ::dtol((inExtraSpace * theModifier));
                    ++theNumExpandable;
                }

                theChild->SetSize(theChildSize);
            }
        }

        // If there is extra space and there are items able to expand still
        if ((inExtraSpace - theSpaceDistributed) > 0) {
            if (theNumExpandable > 0) {
                this->ResizeExpandableControlsX((inExtraSpace - theSpaceDistributed),
                                                theNumExpandable);
            } else {
                m_HasUnusedSpace = true;
            }
        }
    }
}

//==============================================================================
/**
 *	Distributes remaining size to items that can be expanded.
 *  this should only be called when theNumExpandable items > the extra space
 */
void CFlowLayout::DistributeRemaining(long inExtraSpace)
{
    long theNumToDistribute = inExtraSpace;
    if (m_FlowDirection == FLOW_VERTICAL) {
        ControlGraph::SIterator thePos = GetChildren();
        // Loop through teh children for items that can expand
        for (; thePos.HasNext() && theNumToDistribute > 0; ++thePos) {
            std::shared_ptr<CControlData> theChild = (*thePos);

            // If this item can be expanded then expand accd to new percent
            if (CanBeExpandedY(theChild->GetControl()) && theChild->IsVisible()) {
                CPt theChildSize = theChild->GetSize();
                theChildSize.y++;
                theChild->SetSize(theChildSize.x, theChildSize.y);
                theNumToDistribute--;
            }
        }
    } else {
        ControlGraph::SIterator thePos = GetChildren();
        // Loop through teh children for items that can expand
        for (; thePos.HasNext() && theNumToDistribute > 0; ++thePos) {
            std::shared_ptr<CControlData> theChild = (*thePos);

            // If this item can be expanded then expand accd to new percent
            if (CanBeExpandedX(theChild->GetControl()) && theChild->IsVisible()) {
                CPt theChildSize = theChild->GetSize();
                theChildSize.x++;
                theChild->SetSize(theChildSize.x, theChildSize.y);
                theNumToDistribute--;
            }
        }
    }
}

//==============================================================================
/**
 *	Gets the total prefered sizes of the children
 */
CPt CFlowLayout::GetTotalPreferredSizes()
{
    CPt theTotalPrefSizes(0, 0);

    ControlGraph::SIterator thePos = GetChildren();
    for (; thePos.HasNext(); ++thePos) {
        std::shared_ptr<CControlData> theChild = (*thePos);
        if (theChild->IsVisible()) {
            theTotalPrefSizes += theChild->GetPreferredSize();
        }
    }

    return theTotalPrefSizes;
}

//==============================================================================
/**
 *	sets the size and does a recalc
 */
void CFlowLayout::SetSize(CPt inSize)
{
    if (inSize != GetSize()) {
        CControl::SetSize(inSize);

        RecalcLayout();
    }
}

void CFlowLayout::SetLayout(CPt inSize, CPt inPosition)
{
    CControl::SetLayout(inSize, inPosition);

    RecalcLayout();
}
//==============================================================================
/**
 *	override to force a recalc when the parent is changed
 */
void CFlowLayout::OnParentChanged(CControl *inParent)
{
    CControl::OnParentChanged(inParent);
    RecalcLayout();
}

//==============================================================================
/**
 *	Sets this object's alignment
 */
void CFlowLayout::SetAlignment(EVerticalAlignment inVertical, EHorizontalAlignment inHorizontal)
{
    m_VerticalAlignment = inVertical;
    m_HorizontalAlignment = inHorizontal;

    RecalcLayout();
}

//==============================================================================
/**
 *	Overloaded draw function.  Draws the extra control if it is needed
 */
void CFlowLayout::Draw(CRenderer *inRenderer)
{
    // If there is unused space in this control, then use the blank control
    if (m_HasUnusedSpace && m_BlankControl) {
        if (m_BlankControl->IsVisible()) {
            inRenderer->PushTranslation(m_BlankControl->GetPosition());
            if (!m_BlankDisable)
                m_BlankControl->Draw(inRenderer);
            inRenderer->PopTranslation();
        }
    }
}

//==============================================================================
/**
 *	called when a child's size changes
 */
void CFlowLayout::OnChildSizeChanged(CControl *inChild)
{
    Q_UNUSED(inChild);

    if (!m_ResizingChildren) {
        ResetMinMaxPref();

        if (GetParent() != nullptr)
            GetParent()->OnChildSizeChanged(this);

        RecalcLayout();
    }
}

//==============================================================================
/**
 * Called to calc the min/max/and pre size of this layout.  NOTE: this function
 * no longer adjusts the max size of the flow layout.  The max size is not dictated
 * by the size of the children, so it can be set independently with a call to
 * SetMaximumSize().
 */
void CFlowLayout::ResetMinMaxPref()
{
    if (m_AutoMin) {
        CPt thePreferredSize(0, 0);
        CPt theMinimumSize(0, 0);

        thePreferredSize = theMinimumSize;

        // Keep track of the largest, minimum size of all the children for use later
        CPt theBiggestChildMinSize(0, 0);

        // Go through all the children and find their maximum sizes.
        ControlGraph::SIterator thePos = GetChildren();

        for (; thePos.HasNext(); ++thePos) {
            std::shared_ptr<CControlData> theChild = (*thePos);
            if (theChild->IsVisible()) {
                theChild->ResetMinMaxPref();
                CPt theChildPrefSize = theChild->GetPreferredSize();
                CPt theChildMinSize = theChild->GetMinimumSize();

                // use the sum of the children in the flow direction and the smallest size
                // in the non-flow direction.
                if (m_FlowDirection == FLOW_VERTICAL) {
                    thePreferredSize.y += theChildPrefSize.y;

                    theMinimumSize.y += theChildMinSize.y;

                    if (theChildMinSize.x > theBiggestChildMinSize.x)
                        theBiggestChildMinSize = theChildMinSize;
                } else {
                    thePreferredSize.x += theChildPrefSize.x;

                    theMinimumSize.x += theChildMinSize.x;

                    if (theChildMinSize.y > theBiggestChildMinSize.y)
                        theBiggestChildMinSize = theChildMinSize;
                }
            }
        }

        // If this is a vertical flow
        if (m_FlowDirection == FLOW_VERTICAL) {
            // The width is determined by minimum size of the largest child
            theMinimumSize.x = theBiggestChildMinSize.x;

            // If the flow is aligned to top, the top margin must be added to the minimum height
            if (m_VerticalAlignment == ALIGN_TOP)
                theMinimumSize.y += m_TopMargin;
            // If the flow is aligned to bottom, the bottom margin must be added to the minimum
            // height
            else if (m_VerticalAlignment == ALIGN_BOTTOM)
                theMinimumSize.y += m_BottomMargin;

            theMinimumSize.y += (m_ChildGap * GetChildCount());
        }
        // If this is a horzontal flow
        else {
            // The height is determined by minimum size of the largest child
            theMinimumSize.y = theBiggestChildMinSize.y;

            // If the flow is left aligned, the left margin must be added to the minimum width
            if (m_HorizontalAlignment == ALIGN_LEFT)
                theMinimumSize.x += m_LeftMargin;
            // If the flow is right aligned, the right margin must be added to the minimum width
            else if (m_HorizontalAlignment == ALIGN_RIGHT)
                theMinimumSize.x += m_RightMargin;

            theMinimumSize.x += (m_ChildGap * GetChildCount());
        }

        thePreferredSize.x = max(thePreferredSize.x, theMinimumSize.x);
        thePreferredSize.y = max(thePreferredSize.y, theMinimumSize.y);

        SetMinimumSize(theMinimumSize);
        SetPreferredSize(thePreferredSize);
    }
}

//=============================================================================
/**
 * Override the default in case the blank control was hit
 * @param inPoint where the mouse was clicked, in local coordinates.
 * @param inFlags Modifier keys that are down at time of event.  Can be any
 * @return true if the mouse event is processed.
 */
bool CFlowLayout::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theRetVal = CControl::OnMouseDown(inPoint, inFlags);
    if (!theRetVal) {
        if (m_BlankControl && m_BlankControl->HitTest(inPoint))
            GrabFocus(nullptr);
    }
    return theRetVal;
}

//=============================================================================
/**
 * Override the default in case the blank control was hit
 * @param inPoint where the mouse was clicked, in local coordinates.
 * @param inFlags Modifier keys that are down at time of event.  Can be any
 * @return true if the mouse event is processed.
 */
bool CFlowLayout::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    bool theRetVal = CControl::OnMouseRDown(inPoint, inFlags);
    if (!theRetVal) {
        if (m_BlankControl && m_BlankControl->HitTest(inPoint))
            GrabFocus(nullptr);
    }
    return theRetVal;
}
