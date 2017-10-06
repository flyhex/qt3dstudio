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

#include "Renderer.h"
#include "ToggleButton.h"
#include "BaseStateRow.h"
#include "StudioPreferences.h"
#include "TimelineDropTarget.h"
#include "BaseTimelineTreeControl.h"
#include "NameEdit.h"
#include "Bindings/ITimelineItem.h"
#include "StudioPreferences.h"

//=============================================================================
/**
 * Create a new tree control for the specified state row.
 * This control contains the toggle button and item name controls.
 * @param inStateRow the state row of which this belongs to.
 */
CBaseTimelineTreeControl::CBaseTimelineTreeControl(CBaseStateRow *inStateRow, bool inMaster)
    : m_Selected(false)
    , m_MouseDown(false)
{
    m_StateRow = inStateRow;

    m_BackgroundColor = m_StateRow->GetTimebarBackgroundColor(m_StateRow->GetObjectType());

    // Create the expand/collapse button.
    m_ExpandButton = new CToggleButton();
    m_ExpandButton->SetUpImage("arrow.png");
    m_ExpandButton->SetDownImage("arrow_down.png");

    // Add the button and initialize all the listeners for the events on it.
    AddChild(m_ExpandButton);
    m_ExpandButton->SigToggle.connect(std::bind(&CBaseStateRow::ToggleExpansion, m_StateRow,
                                                std::placeholders::_1, std::placeholders::_2));
    m_ExpandButton->SetVisible(false);

    m_Icon = new CSIcon(m_StateRow->GetIcon(), m_StateRow->GetDisabledIcon());
    AddChild(m_Icon);

    // Create and add the name label.
    m_Text = nullptr; // withdrawn from constructor to delay creation of text object

    // Initialize all the component's positions to 0.
    SetIndent(CStudioPreferences::GetRowSize());

    SetMinimumSize(CPt(CBaseStateRow::DEFAULT_TOGGLE_LENGTH + m_Icon->GetPosition().x
                           + m_Icon->GetSize().x + 5,
                       CStudioPreferences::GetRowSize()));

    m_TrackingPoint.x = 0;
    m_TrackingPoint.y = 0;
    m_DrawAcceptBefore = false;
    m_DrawAcceptAfter = false;

    // Set up default text colors
    m_NormalTextColor = CStudioPreferences::GetNormalColor();
    m_SelectedTextColor = CStudioPreferences::GetNormalColor();
    if (inMaster) {
        m_NormalTextColor = CStudioPreferences::GetMasterColor();
        m_SelectedTextColor = CStudioPreferences::GetMasterColor();
    }
    m_LockedTextColor = CStudioPreferences::GetLockedTextColor();
}

CBaseTimelineTreeControl::~CBaseTimelineTreeControl()
{
    delete m_Icon;
    delete m_ExpandButton;
    delete m_Text;
}

//=============================================================================
/**
 * Create a new text object. For performance reasons we delay
 * creating this object until it is needed, i.e. until the row is exposed
 * by the user and the Draw method is called
 */
void CBaseTimelineTreeControl::CreateText()
{
    if (!m_Text) {
        ITimelineItem *theTimelineItem = m_StateRow->GetTimelineItem();

        m_Text = new CNameEdit(theTimelineItem);

        m_Text->SetSize(
            CPt(CStudioPreferences::GetTimelineNameSize(),
                CStudioPreferences::GetRowSize() - 3)); /* m_ExpandButton->GetSize( ).y - 3*/
        // m_Text->SetBGColorNoFocus( CStudioPreferences::GetNormalColor( ) );
        // if ( theTimelineItem->IsMaster( ) )
        //	m_Text->SetBGColorNoFocus( CStudioPreferences::GetMasterColor( ) );
        // m_Text->SetFillBackground( false );
        m_Text->SetBoldText(false);

        // If the object is the scene, you can't edit it's name
        m_Text->SetEditable(m_StateRow->GetObjectType() != OBJTYPE_SCENE);
        AddChild(m_Text);
        m_Text->SetPosition(CPt(m_Icon->GetPosition().x + m_Icon->GetSize().x + 5, 1));

        // This was disabled before Text was created.
        if (!IsEnabled()) {
            m_Text->SetEnabled(false);
            m_Text->SetParentEnabled(false);
            m_Text->SetTextColor(m_LockedTextColor);
        } else // since we do delay-creation, "sync" with the parent's selection state
            UpdateTextSelection();

        // This is so that make the timeline scrollbar scrolls correctly
        // ( i.e. to the end of the asset name )
        CPt theSize(GetSize());
        theSize.x =
            CBaseStateRow::DEFAULT_TOGGLE_LENGTH + m_Text->GetPosition().x + m_Text->GetSize().x;
        SetAbsoluteSize(theSize);
    }
}

void CBaseTimelineTreeControl::UpdateTextSelection()
{
    // since we do delay-creation for the Text only when we have to draw it.. this checks if it is
    // created first
    if (m_Text) {
        if (!IsEnabled())
            m_Text->SetTextColor(m_LockedTextColor);
        else
            m_Text->SetTextColor(m_Selected ? m_SelectedTextColor : m_NormalTextColor);
        // m_Text->SetFillBackground( m_Selected );
        // m_Text->SetBoldText( m_Selected );
    }
}

//=============================================================================
/**
 * Perform the drawing of this control.
 * @param inRenderer the renderer to draw to.
 */
void CBaseTimelineTreeControl::Draw(CRenderer *inRenderer)
{
    CreateText(); // the row is now exposed and we can't delay creating the text object any longer

    CRct theRect(GetSize());
    // Fill in the background
    if (!m_Selected)
        inRenderer->FillSolidRect(theRect, m_BackgroundColor);
    else
        inRenderer->FillSolidRect(theRect, CStudioPreferences::GetTimelineSelectColor());

    // if ( m_Text )
    // m_Text->SetBoldText( m_Selected );
    // m_Text->SetFillBackground( m_Selected );

    // Draw the line at the bottom of this control
    inRenderer->PushPen(CStudioPreferences::GetTreeFloorColor());
    inRenderer->MoveTo(CPt(0, theRect.size.y - 1));
    inRenderer->LineTo(CPt(theRect.size.x, theRect.size.y - 1));
    inRenderer->PopPen();
}

//=============================================================================
/**
 * Override for the set parent enabled function which tells children the state of the parent
 */
void CBaseTimelineTreeControl::SetEnabled(bool inIsEnabled)
{
    CControl::SetEnabled(inIsEnabled);
    if (m_Text) {
        m_Text->SetEnabled(inIsEnabled);
        if (!inIsEnabled)
            m_Text->SetTextColor(m_LockedTextColor);
        else
            m_Text->SetTextColor(m_Selected ? m_SelectedTextColor : m_NormalTextColor);
        Invalidate();
    }
}

//=============================================================================
/**
 * Notification that something has changed on the asset that this
 * represents, update it.
 */
void CBaseTimelineTreeControl::Refresh(ITimelineItem *inTimelineItem)
{
    bool theEnabled = !inTimelineItem->IsLocked();
    if (m_Text) {
        // Make sure the color is correct depending on if its is a master object
        if (m_NormalTextColor != CStudioPreferences::GetMasterColor()
            && inTimelineItem->IsMaster()) {
            m_NormalTextColor = CStudioPreferences::GetMasterColor();
            m_Text->SetBGColorNoFocus(CStudioPreferences::GetMasterColor());
            if (!m_Selected)
                m_Text->SetTextColor(m_NormalTextColor);
        }

        m_Text->SetData(inTimelineItem->GetName());
    }
    m_Icon->SetImage((theEnabled) ? m_StateRow->GetIcon() : m_StateRow->GetDisabledIcon());
    SetEnabled(theEnabled);
}

//=============================================================================
/**
 * Set the indent of this control.
 * The indent gives the semblance of a tree control, and causes the toggle
 * name and icon to be pushed in some.
 * @param inIndent the indent for this control.
 */
void CBaseTimelineTreeControl::SetIndent(long inIndent)
{
    m_Indent = inIndent;

    // Set the new position for all the children.
    m_ExpandButton->SetPosition(CPt(inIndent, 0));

    m_Icon->SetPosition(CPt(m_ExpandButton->GetPosition().x + m_ExpandButton->GetSize().x, 0));
    if (m_Text)
        m_Text->SetPosition(CPt(m_Icon->GetPosition().x + m_Icon->GetSize().x + 5, 1));
}

//=============================================================================
/**
 * Get the current indent of this control.
 */
long CBaseTimelineTreeControl::GetIndent()
{
    return m_Indent;
}

//=============================================================================
/**
 * Set whether or not to have the toggle control visible.
 * The toggle is turned off by the state row when there are no visible children.
 * @param inIsToggleVisible false if the toggle is not to be visible.
 */
void CBaseTimelineTreeControl::SetToggleVisible(bool inIsToggleVisible)
{
    m_ExpandButton->SetVisible(inIsToggleVisible);
}

//=============================================================================
/**
 * Set whether or not this control is expanded.
 * This is used to set the state of the expand button.
 */
void CBaseTimelineTreeControl::SetExpanded(bool inIsExpanded)
{
    m_ExpandButton->SetToggleState(inIsExpanded);
}

//=============================================================================
/**
 * Set the current background color for this control.
 * The background color changes when the control gets a mouse over/mouse out.
 */
void CBaseTimelineTreeControl::SetBackgroundColor(CColor inColor)
{
    if (m_BackgroundColor == inColor)
        return;

    m_BackgroundColor = inColor;

    Invalidate();
}

//=============================================================================
/**
 * Notify the row that a mouse out occurred.
 * The row will in turn turn off the highlighting.
 * @param inPoint the location of the mouse.
 * @param inFlags the state of the mouse.
 */
void CBaseTimelineTreeControl::OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    m_DrawAcceptAfter = false;
    m_DrawAcceptBefore = false;

    CControl::OnMouseOut(inPoint, inFlags);

    m_StateRow->OnMouseOut();

    if (m_TimerHandler) {

        // nullptr out our handle so we can create a new one.
        m_TimerHandler = std::shared_ptr<UICDM::ISignalConnection>();
    }

    AcceptDropAfter(false);
    AcceptDropBefore(false);
}

//=============================================================================
/**
 * Notify the row that a mouse over occurred.
 * The row will in turn turn on the highlighting.
 * @param inPoint the location of the mouse.
 * @param inFlags the state of the mouse.
 */
void CBaseTimelineTreeControl::OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOver(inPoint, inFlags);

    m_StateRow->OnMouseOver();
}

//=============================================================================
/**
 * Pass the double click notification on to the row and have it process it.
 * The row will do object-specific actions on doubleclicks.
 * @param inPoint the location of the mouse.
 * @param inFlags the state of the mouse.
 * @return true stating that the event was processed.
 */
bool CBaseTimelineTreeControl::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDoubleClick(inPoint, inFlags)) {
        m_StateRow->OnMouseDoubleClick(inPoint, inFlags);
        GrabFocus(nullptr);
    }
    return true;
}

//=============================================================================
/**
 * Handles mouse down on the this control.  Flags the button as down which results
 * in some possible drawing changes.
 * @param inPoint location of the mouse when event occurred
 * @param inFlags state of modifier keys when event occurred
 * @return true
 */
bool CBaseTimelineTreeControl::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDown(inPoint, inFlags)) {
        SBaseStateRowSelectionKeyState theKeyState;
        if ((CHotKeys::MODIFIER_SHIFT & inFlags) == CHotKeys::MODIFIER_SHIFT)
            theKeyState.SetShiftDown();
        if ((CHotKeys::MODIFIER_CONTROL & inFlags) == CHotKeys::MODIFIER_CONTROL)
            theKeyState.SetControlDown();
        m_StateRow->Select(theKeyState);

        // Always track where the mouse is.
        m_MouseDown = true;

        Invalidate();
    }

    return true;
}

bool CBaseTimelineTreeControl::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseRDown(inPoint, inFlags))
        m_StateRow->OnMouseRDown(inPoint, inFlags);

    return true;
}

void CBaseTimelineTreeControl::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    m_MouseDown = false;

    CControl::OnMouseUp(inPoint, inFlags);

    AcceptDropAfter(false);
    AcceptDropBefore(false);
}

//=============================================================================
/**
 * This method handles the keydown event for a StateTreeControl. It calls
 * CControl::OnKeyDown method to make sure that the keydown event is handled
 * by its children. If the keydown event is not handled and F2 is down, it
 * enables text edit mode.
 * @param inChar is the char pressed
 * @param inFlags state of modifier keys when event occurred
 * @return if the key was handled
 */
bool CBaseTimelineTreeControl::OnKeyDown(unsigned int inChar, Qt::KeyboardModifiers inFlags)
{
    bool theKeyWasHandled = CControl::OnKeyDown(inChar, inFlags);

    if (!theKeyWasHandled && (inChar == Qt::Key_F2)) {
        DoRename();
        theKeyWasHandled = true;
    }

    return theKeyWasHandled;
}

//=============================================================================
/**
 * This is so the Gesture can this object to get something ready to Drag.
 */
void CBaseTimelineTreeControl::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseMove(inPoint, inFlags);

    if (m_MouseDown /*&& inFlags & MOUSE_LBUTTON*/) {
        long theDeltaX = inPoint.x - m_TrackingPoint.x;
        long theDeltaY = inPoint.y - m_TrackingPoint.y;

        if (::abs(theDeltaX) > 3 || ::abs(theDeltaY) > 3) {
            m_TrackingPoint = inPoint;

            m_StateRow->DoStartDrag(GetWindowListener());
        }
    }
}

//=============================================================================
/**
 * Notification that the state that this is displaying has been selected.
 */
void CBaseTimelineTreeControl::OnSelect()
{
    m_Selected = true;

    UpdateTextSelection();

    Invalidate();
}

//=============================================================================
/**
 * Notification that the state that this is displaying has been deselected.
 */
void CBaseTimelineTreeControl::OnDeselect()
{
    m_Selected = false;

    UpdateTextSelection();
    Invalidate();
}

void CBaseTimelineTreeControl::GrabTextFocus()
{
    GrabFocus(m_Text);
}

//=============================================================================
/**
 * To enable F2 editing.
 */
void CBaseTimelineTreeControl::OnGainFocus()
{
    CControl::OnGainFocus();
    GrabFocus(m_Text);
}

//=============================================================================
/**
 * Called when this control loses focus.  Overridden because we need to set the
 * text color depending on whether or not the asset for this row is still
 * selected.
 */
void CBaseTimelineTreeControl::OnLoseFocus()
{
    CControl::OnLoseFocus();

    if (m_Text) {
        if (m_Selected) {
            m_Text->SetTextColor(m_SelectedTextColor);
        } else // If this asset is no longer selected
        {
            // If the row is enabled, use the normal text color
            if (m_Text->IsEnabled()) {
                m_Text->SetTextColor(m_NormalTextColor);
            }
            // Otherwise use the locked text color
            else {
                m_Text->SetTextColor(m_LockedTextColor);
            }
        }
    }

    AcceptDropAfter(false);
    AcceptDropBefore(false);
}

//=============================================================================
/**
 *	 If the name is changed, the size has to be adjusted accordingly.
 */
void CBaseTimelineTreeControl::OnChildSizeChanged(CControl *inChild)
{
    CControl::OnChildSizeChanged(inChild);

    if (inChild == m_Text) { // This is so that make the timeline scrollbar scrolls correctly
        // ( i.e. to the end of the asset name )
        CPt theSize(GetSize());
        theSize.x =
            CBaseStateRow::DEFAULT_TOGGLE_LENGTH + m_Text->GetPosition().x + m_Text->GetSize().x;
        SetAbsoluteSize(theSize);
    }
}

//=============================================================================
/**
 *	 This will do a vertical hit test on this control.
 *	 This need to figure out if the point is toward teh top or toward the bottom, or on this
 *control.
 *	 @param inMousePoint the point where the dropp wants to occure.
 *	 @return  An enumeration representing the location of the potential drop.
 */
CBaseTimelineTreeControl::ECONTROLREGION CBaseTimelineTreeControl::FindHitRegion(CPt &inMousePoint)
{
    // Default Region is "on"
    CBaseTimelineTreeControl::ECONTROLREGION theDropRegion =
        CBaseTimelineTreeControl::ECONTROLREGION_ON;

    CPt theSize = GetSize();
    long theTop = 0;
    long theBottom = theSize.y - 1;
    long thePointY = inMousePoint.y;

    // check if we are in the upper part of the control
    if ((thePointY >= theTop) && (thePointY <= (theTop + 3))) {
        theDropRegion = CBaseTimelineTreeControl::ECONTROLREGION_ABOVE;
    }
    // check if we are in the lower part of the control
    else if ((thePointY <= (theBottom)) && (thePointY >= (theBottom - 3))) {
        theDropRegion = CBaseTimelineTreeControl::ECONTROLREGION_BELOW;
    }

    return theDropRegion;
}

//=============================================================================
/**
 *		Find an object under the point.
 *		If tht point is close to the top or the bottom of the control,
 *		then the Asset to use would be the parent of the current asset.
 *		@param inMousePoint the point where the Drop wants to occure.
 */
CDropTarget *CBaseTimelineTreeControl::BuildDropTarget(CPt &inMousePoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inFlags);

    // This will do the hit testing to see where we are with the point.
    ECONTROLREGION theRegion = FindHitRegion(inMousePoint);

    // Make a new DropTarget to return.
    CTimeLineDropTarget *theTarget = new CTimeLineDropTarget();

    EDROPDESTINATION theDropDest = EDROPDESTINATION_ON;

    switch (theRegion) {
    case ECONTROLREGION_BELOW:
        theDropDest = EDROPDESTINATION_BELOW;

        AcceptDropAfter(true);
        AcceptDropBefore(false);
        break;

    case ECONTROLREGION_ABOVE:
        theDropDest = EDROPDESTINATION_ABOVE;

        AcceptDropAfter(false);
        AcceptDropBefore(true);
        break;

    case ECONTROLREGION_ON:

        AcceptDropAfter(false);
        AcceptDropBefore(false);
        break;
    }
    theTarget->SetDestination(theDropDest);
    // For insertion markers
    theTarget->SetInsertionMarkerRow(this);
    theTarget->SetInsertionMarkerIndent(m_Icon->GetPosition().x);

    // connect the data portion of the drag&drop action
    m_StateRow->SetDropTarget(theTarget);

    return theTarget;
}

//=============================================================================
/**
 *	This function is overriden from the CControl class.
 *	It will find an Asset that can be dropped upon. Also it will
 *	figureout if the DropTarget should contain this Asset or the parent
 *	of this Asset.
 *	@param inMousePoint the coords [in local space] of the drop action.
 *	@param inFlags the Modifier flags for the keyboard state.
 *	@return the found DropTarget or null if not found.
 */
CDropTarget *CBaseTimelineTreeControl::FindDropCandidate(CPt &inMousePoint, Qt::KeyboardModifiers inFlags)
{
    // Make sure the Mouse Highlighting happens.
    m_StateRow->OnMouseOver();

    // This will do all of the work.
    CDropTarget *theReturnTarget = BuildDropTarget(inMousePoint, inFlags);

    // Expand the object [ once again ask CE for an explaination of this ]
    if (!m_TimerHandler && m_ExpandButton->IsVisible() && m_ExpandButton->IsEnabled()) {
        if (!m_DrawAcceptBefore && !m_DrawAcceptAfter)
            m_TimerHandler = Q3DStudio::ITickTock::GetInstance().AddTimer(
                1000, false, std::bind(&CBaseTimelineTreeControl::OnTimer, this),
                "CStateTreeControl::FindDropCandidate::" + GetName());
    }

    // we always return true, since we should be the only one to handle it.
    return theReturnTarget;
}

//=============================================================================
/**
 * Notification that the Hover Time has expired,
 */
void CBaseTimelineTreeControl::OnTimer()
{
    // Expand the Row to show the children.
    m_StateRow->Expand();
}

//=============================================================================
/**
 * This will set the Flag so we can Draw the Bottom Line.
 * @param inAccept true to draw the line false otherwise.
 */
void CBaseTimelineTreeControl::AcceptDropAfter(bool inAccept)
{
    if (inAccept != m_DrawAcceptAfter) {
        m_DrawAcceptAfter = inAccept;
    }
}

//=============================================================================
/**
 * This will set the Flag so we can Draw the Top Line.
 * @param inAccept true to draw the line false otherwise.
 */
void CBaseTimelineTreeControl::AcceptDropBefore(bool inAccept)
{
    if (inAccept != m_DrawAcceptBefore) {
        m_DrawAcceptBefore = inAccept;
    }
}

//=============================================================================
/**
 * Called by the state context menu to do the renaming portion of the menu
 */
void CBaseTimelineTreeControl::DoRename()
{
    CreateText();
    m_Text->SetEditMode(true);
    GrabFocus(m_Text);
    m_Text->SelectAllText();
}

void CBaseTimelineTreeControl::SetNameReadOnly(bool inReadOnly)
{
    CreateText(); // Create the text if it's not ready.
    m_Text->SetEditable(!inReadOnly);
}
