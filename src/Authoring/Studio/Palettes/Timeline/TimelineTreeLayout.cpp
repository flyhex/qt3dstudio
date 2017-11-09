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

//=============================================================================
// Prefix
//=============================================================================
#include "stdafx.h"

//=============================================================================
// Includes
//=============================================================================
#include "TimelineTreeLayout.h"
#include "FlowLayout.h"
#include "Scroller.h"
#include "StateRow.h"
#include "FilterToolbar.h"
#include "ToggleToolbar.h"
#include "StudioPreferences.h"
#include "TimelineControl.h"
#include "Renderer.h"
#include "ToggleBlankControl.h"
#include "ColorBlankControl.h"
#include "TreeBlankControl.h"

//=============================================================================
/**
 * Constructor
 */
CTimelineTreeLayout::CTimelineTreeLayout(CTimelineControl *inTimelineControl, IDoc *inDoc)
    : m_IsScrolling(false)
{
    m_TimelineControl = inTimelineControl;

    m_ColorScroller = new CScroller();
    m_ColorScroller->SetHorizontalScrollMode(CScroller::NEVER);
    m_ColorScroller->SetVerticalScrollMode(CScroller::NEVER);
    m_ColorScroller->AddScrollListener(this);

    m_ColorBlankControl = new CColorBlankControl();
    m_ColorList = new CFlowLayout(m_ColorBlankControl);
    m_ColorScroller->AddChild(m_ColorList);

    m_ToggleScroller = new CScroller();
    m_ToggleScroller->SetHorizontalScrollMode(CScroller::NEVER);
    m_ToggleScroller->SetVerticalScrollMode(CScroller::NEVER);

    m_ToggleBlankControl = new CToggleBlankControl();
    m_ToggleList = new CFlowLayout(m_ToggleBlankControl);
    m_ToggleScroller->AddChild(m_ToggleList);
    m_ToggleScroller->AddScrollListener(this);

    m_TreeScroller = new CScroller();
    m_TreeScroller->SetVerticalScrollMode(CScroller::NEVER);
    m_TreeScroller->SetHorizontalScrollMode(CScroller::ALWAYS);
    m_TreeScroller->AddScrollListener(this);

    m_TreeBlankControl = new CTreeBlankControl();
    m_TreeList = new CFlowLayout(m_TreeBlankControl);
    m_TreeScroller->AddChild(m_TreeList);

    m_FilterToolbar = new CFilterToolbar(this);
    m_ToggleToolbar = new CToggleToolbar(this);

    AddChild(m_TreeScroller);
    AddChild(m_ToggleScroller);
    AddChild(m_ToggleToolbar);
    AddChild(m_ColorScroller);
}

//=============================================================================
/**
 * Destructor
 */
CTimelineTreeLayout::~CTimelineTreeLayout()
{
    m_ColorScroller->RemoveChild(m_ColorList);
    m_ToggleScroller->RemoveChild(m_ToggleList);
    m_TreeScroller->RemoveChild(m_TreeList);

    RemoveChild(m_TreeScroller);
    RemoveChild(m_ToggleScroller);
    RemoveChild(m_FilterToolbar);
    RemoveChild(m_ToggleToolbar);
    RemoveChild(m_ColorScroller);
    // RemoveChild( m_BreadCrumbToolbar );

    delete m_FilterToolbar;
    delete m_ToggleToolbar;
    delete m_ColorScroller;
    delete m_ColorList;
    delete m_ToggleScroller;
    delete m_ToggleList;
    delete m_TreeList;
    delete m_TreeScroller;
    // delete m_BreadCrumbToolbar;

    // Delete all the rows, this control is responsible for the rows, maybe it should not
    // be but currently it is.
    TTimelineRowList::iterator thePos = m_Rows.begin();
    for (; thePos != m_Rows.end(); ++thePos) {
        CTimelineRow *theRow = (*thePos);
        theRow->Dispose(); // Dispose will delete the row as well
    }
}

//=============================================================================
/**
 * Clear out all the contents of this tree layout.
 * This will also delete all the rows, so make sure it is called after the
 * TimelineTimelineLayout::ClearRows is called.
 */
void CTimelineTreeLayout::ClearRows()
{
    TTimelineRowList::iterator thePos = m_Rows.begin();
    for (; thePos != m_Rows.end(); ++thePos) {
        CTimelineRow *theRow = (*thePos);
        m_ColorList->RemoveChild(theRow->GetColorControl());
        m_TreeList->RemoveChild(theRow->GetTreeControl());
        m_ToggleList->RemoveChild(theRow->GetToggleControl());

        theRow->Dispose();
    }

    m_Rows.clear();
}

//=============================================================================
/**
 * Set the filter back to it's default state.
 */
void CTimelineTreeLayout::ResetFilter()
{
    m_FilterToolbar->FilterBehaviors(false);
    m_FilterToolbar->FilterProperties(false);
    m_FilterToolbar->FilterMaterials(false);
    m_FilterToolbar->FilterShy(false);
}

//=============================================================================
/**
 * Set the size of this control.
 * Overrides CControl::SetSize so that RecalcLayout can be called.
 */
void CTimelineTreeLayout::SetSize(CPt inSize)
{
    if (inSize != GetSize()) {
        CControl::SetSize(inSize);

        RecalcLayout();
    }
}

//=============================================================================
/**
 * Recalculate the layout of all the child components.
 * Called when this changes size and all the children need to be repositioned.
 */
void CTimelineTreeLayout::RecalcLayout()
{
    CPt mySize = GetSize();
    long theHeaderHeight = CStudioPreferences::GetHeaderHeight();

    m_FilterToolbar->SetSize(CPt(120, theHeaderHeight));
    m_FilterToolbar->SetPosition(0, 0);

    m_ToggleToolbar->SetSize(CPt(61, theHeaderHeight));
    m_ToggleToolbar->SetPosition(mySize.x - m_ToggleToolbar->GetSize().x, 0);

    m_ColorScroller->SetSize(CPt(CStudioPreferences::GetRowSize(), mySize.y - theHeaderHeight
                                     - m_TreeScroller->GetHorizontalBar()->GetMinimumSize().y
                                     - theHeaderHeight));
    m_ColorScroller->SetPosition(0, theHeaderHeight);

    m_ToggleScroller->SetSize(CPt(m_ToggleToolbar->GetSize().x, mySize.y - theHeaderHeight
                                      - m_TreeScroller->GetHorizontalBar()->GetMinimumSize().y
                                      - theHeaderHeight));
    m_ToggleScroller->SetPosition(mySize.x - m_ToggleScroller->GetSize().x, theHeaderHeight);

    m_TreeScroller->SetSize(
        CPt(mySize.x, mySize.y - m_FilterToolbar->GetSize().y - theHeaderHeight));
    m_TreeScroller->SetPosition(0, theHeaderHeight);

    m_TreeScroller->SetAdditionalClippingRect(
        CRct(m_ColorScroller->GetSize().x, 0,
             m_ToggleScroller->GetPosition().x - m_ColorScroller->GetSize().x,
             m_TreeScroller->GetSize().y));
}

//=============================================================================
/**
 * Add another top level item to the left side of the timeline.
 * If there is already a top level item then this one will be appended to the
 * list.
 * @param inRow the row to be added.
 */
void CTimelineTreeLayout::AddRow(CTimelineRow *inRow)
{
    m_ColorList->AddChild(inRow->GetColorControl());
    m_TreeList->AddChild(inRow->GetTreeControl());
    m_ToggleList->AddChild(inRow->GetToggleControl());

    m_Rows.push_back(inRow);

    inRow->SetIndent(20);

    inRow->Filter(m_Filter);
}

//=============================================================================
/**
 * Applies the current filter settings to the timeline.  Although the filter
 * preferences can be set independently, they are not actually applied until
 * this function is called.
 */
void CTimelineTreeLayout::Filter()
{
    for (TTimelineRowList::iterator thePos = m_Rows.begin(); thePos != m_Rows.end(); ++thePos) {
        CTimelineRow *theRow = *thePos;
        theRow->Filter(m_Filter);
    }

    // TODO: sk - it is unclear to me what this is trying to do.. I am leavint this here till it
    // becomes obvious it is totally redundant OR someone finds a related bug
    /*
    // Call OnSelect( ) on the selected object (if there is one) to get the timeline to scroll down.
    CAsset* theSelectedObject = dynamic_cast<CAsset*>( m_Doc->GetSelectedObject( ) );
    if ( theSelectedObject )
            theSelectedObject->OnSelect( );

    m_TimelineControl->GetTimelineLayout( )->RecalcLayout( );
    */
}

void CTimelineTreeLayout::OnScroll(CScroller *inSource, CPt inScrollAmount)
{
    Q_UNUSED(inScrollAmount);

    // SetScrollPositionY triggers another onScroll event and potentially causes the position
    // to be set incorrectly.
    if (!m_IsScrolling) {
        m_IsScrolling = true;
        m_TreeBlankControl->SetVisiblePositionX(inSource->GetVisiblePosition().x);
        m_TimelineControl->SetScrollPositionY(inSource, inSource->GetVisiblePosition().y);
        m_IsScrolling = false;
    }
}

//=============================================================================
/**
 * Set the vertical position of all the scrollers in this view.
 * This is used to sync up the positions with the timebar scroller view.
 * @param inScrollPositionY the position of the scroller.
 */
void CTimelineTreeLayout::SetScrollPositionY(CScroller *inSource, long inScrollPositionY,
                                             bool inAbsolute)
{
    Q_UNUSED(inSource);

    if (!inAbsolute) {
        CRct theVisibleRect(m_ColorScroller->GetVisiblePosition(),
                            m_ColorScroller->GetVisibleSize());
        CPt thePoint(m_ColorScroller->GetVisiblePosition().x, inScrollPositionY);
        if (!theVisibleRect.IsInRect(thePoint)) {
            m_ColorScroller->SetVisiblePosition(
                CPt(m_ColorScroller->GetVisiblePosition().x, inScrollPositionY));
            m_TreeScroller->SetVisiblePosition(
                CPt(m_TreeScroller->GetVisiblePosition().x, inScrollPositionY));
            m_ToggleScroller->SetVisiblePosition(
                CPt(m_ToggleScroller->GetVisiblePosition().x, inScrollPositionY));
        }
    } else {
        m_ColorScroller->SetVisiblePosition(
            CPt(m_ColorScroller->GetVisiblePosition().x, inScrollPositionY));
        m_TreeScroller->SetVisiblePosition(
            CPt(m_TreeScroller->GetVisiblePosition().x, inScrollPositionY));
        m_ToggleScroller->SetVisiblePosition(
            CPt(m_ToggleScroller->GetVisiblePosition().x, inScrollPositionY));
    }
}

//=============================================================================
/**
 * This is overridden so the Gesture can Notify Drop Listeners that its time to Drop.
 * If the gesture is dragging something then wee will drop.
 */
void CTimelineTreeLayout::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseUp(inPoint, inFlags);
}

//=============================================================================
/**
 * @return rectangle describing the visible area of the tree control
 */
CRct CTimelineTreeLayout::GetVisibleArea()
{
    CPt theUpperLeftCorner;
    CPt theSize;

    theUpperLeftCorner.x = m_ColorScroller->GetSize().x;
    theUpperLeftCorner.y = CStudioPreferences::GetHeaderHeight();
    theSize.x = m_ToggleScroller->GetPosition().x - theUpperLeftCorner.x;
    theSize.y = ::abs(m_ToggleScroller->GetPosition().y - m_ToggleScroller->GetSize().y)
        - theUpperLeftCorner.y;

    return CRct(theUpperLeftCorner, theSize);
}
