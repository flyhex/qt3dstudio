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
#include "Scroller.h"
#include "Renderer.h"
#include "ButtonControl.h"
#include "MasterP.h"
#include "UICDMSignals.h"
#include "ControlData.h"

#include <QDateTime>

using namespace Q3DStudio;
const unsigned long ORIGINAL_DELAY_TIME = 200;
const unsigned long ORIGINAL_OFFSET_AMMOUNT = 205;
//=============================================================================
/**
 * Creates a scroller.
 * @param inCreateImmediately used by subclasses, true if using base class of CScroller
 */
CScroller::CScroller(bool inCreateImmediately /* = true */)
    : m_VisibleSize(0, 0)
    , m_VisiblePosition(0, 0)
    , m_VerticalScrollMode(AS_NEEDED)
    , m_HorizontalScrollMode(AS_NEEDED)
    , m_ResizingChildren(false)
    , m_IsMouseDown(false)
    , m_OffsetAmmount(ORIGINAL_OFFSET_AMMOUNT)
    , m_DelayTime(ORIGINAL_DELAY_TIME)
{
    if (inCreateImmediately)
        Initialize();
    m_ControlData->SetMouseWheelEnabled(true);

    SetName("Scroller");
}

CScroller::~CScroller()
{
    delete m_VerticalBar;
    delete m_HorizontalBar;
}

//=============================================================================
/**
 * Performed once and only once!
 * Do not call if CScroller was called with the createImmediately flag as true.
 */
void CScroller::Initialize()
{
    m_VerticalBar = CreateVerticalBar();
    m_HorizontalBar = CreateHorizontalBar();

    AddChild(m_VerticalBar);
    AddChild(m_HorizontalBar);
}

//=============================================================================
/**
 * Virtual function to get the vertical scroll bar.
 * This allows overriding so subclasses can subclass the ScrollerBar as well.
 * The created bar will be deleted on the destruction of this class.
 * @return the created scrollerbar.
 */
CScrollerBar *CScroller::CreateVerticalBar()
{
    CScrollerBar *theBar = new CScrollerBar(this);
    theBar->SetOrientation(CScrollerBar::VERTICAL);

    return theBar;
}

//=============================================================================
/**
 * Virtual function to get the horizontal scroll bar.
 * This allows overriding so subclasses can subclass the ScrollerBar as well.
 * The created bar will be deleted on the destruction of this class.
 * @return the created scrollerbar.
 */
CScrollerBar *CScroller::CreateHorizontalBar()
{
    CScrollerBar *theBar = new CScrollerBar(this);
    theBar->SetOrientation(CScrollerBar::HORIZONTAL);

    return theBar;
}

void CScroller::OnSizeChanged(CPt /*inSize*/)
{
    CPt theMaxPoint = GetMaxVisiblePosition();
    CPt theVisPoint = GetVisiblePosition();
    if (theVisPoint.x > theMaxPoint.x)
        theVisPoint.x = theMaxPoint.x;
    if (theVisPoint.y > theMaxPoint.y)
        theVisPoint.y = theMaxPoint.y;

    SetVisiblePosition(theVisPoint);
    RecalcLayout();
}
//=============================================================================
/**
 * Sets the size of the scroller.
 * @param inSize the new size for this scroller.
 */
void CScroller::SetSize(CPt inSize)
{
    if (inSize != GetSize()) {
        CControl::SetSize(inSize);
        OnSizeChanged(inSize);
    }
}

void CScroller::SetLayout(CPt inSize, CPt inPosition)
{
    CControl::SetLayout(inSize, inPosition);
    OnSizeChanged(inSize);
}

//=============================================================================
/**
 * Gets the minimum size that this scroller is allowed to be.
 * This is calculated from the minimum size of the child control and the
 * needed size of the scroll bars.
 * @return the minimum size of this object.
 */
CPt CScroller::GetMinimumSize()
{
    CPt theSize(0, 0);
    CControl *theChildControl = GetControl();
    if (theChildControl != nullptr) {
        CPt theChildMin = theChildControl->GetMinimumSize();
        // If at zero the scroll bars are up then include them in the size.
        if (theChildMin.x != 0) {
            theSize += m_HorizontalBar->GetMinimumSize();
        }
        if (theChildMin.y != 0) {
            theSize += m_VerticalBar->GetMinimumSize();
        }
    }

    return theSize;
}

//=============================================================================
/**
 * Get the maximum size of this scroller.
 * This is just the maximum size of the child window, since the scroll bars
 * will not be up when at max size.
 * @return the maximum size of this control.
 */
CPt CScroller::GetMaximumSize()
{
    CPt theMaxSize;
    CControl *theChildControl = GetControl();
    if (theChildControl != nullptr)
        theMaxSize = theChildControl->GetMaximumSize();
    else
        theMaxSize = CControl::GetMaximumSize();

    return theMaxSize;
}

//=============================================================================
/**
 * Add a child control to this control.
 * This is used to add a new child to this control. If there is already a
 * component in the scrolling window then inControl will take the place of the
 * previous control and the previous will be removed as a child of this.
 * @param inControl the control to be added.
 * @param inInsertBefore not used.
 */
void CScroller::AddChild(CControl *inControl, CControl *inInsertBefore /*= nullptr*/)
{
    Q_UNUSED(inInsertBefore);

    if (GetChildCount() > 3)
        RemoveChild(GetChildren().GetCurrent()->GetControl());

    if (GetChildCount() > 0)
        CControl::AddChild(inControl, GetChildren().GetCurrent()->GetControl());
    else
        CControl::AddChild(inControl);

    RecalcLayout();
}

//=============================================================================
/**
 * Recalculate the layout of this object.
 * This is used anytime this is changing size or needs to figure out where the
 * controls go. This does all the work of setting the scrollerbar positions and
 * the child window positions.
 */
void CScroller::RecalcLayout()
{
    UICPROFILE(RecalcLayout);

    m_ResizingChildren = true;

    CPt mySize = GetSize();
    m_VisibleSize = mySize;

    // If the side bar should be visible
    if (IsVerticalVisible()) {
        // Make sure the side bar is visible.
        m_VerticalBar->SetVisible(true);

        // Set the side bars position
        m_VerticalBar->SetPosition(CPt(mySize.x - m_VerticalBar->GetMinimumSize().x, 0));
        // the width of the vertical bar is subtracted from the visible area.
        m_VisibleSize.x -= m_VerticalBar->GetMinimumSize().x;

        if (!IsVerticalScrolling())
            m_VerticalBar->SetEnabled(false);
        else
            m_VerticalBar->SetEnabled(true);
    } else
        m_VerticalBar->SetVisible(false);

    if (IsHorizontalVisible()) {
        // Make sure the bottom bar is visible.
        m_HorizontalBar->SetVisible(true);

        // Set the bottom bar's position.
        m_HorizontalBar->SetPosition(CPt(0, mySize.y - m_HorizontalBar->GetMinimumSize().y));
        // the height of the horizontal bar is subtracted from the visible area.
        m_VisibleSize.y -= m_HorizontalBar->GetMinimumSize().y;

        if (!IsHorizontalScrolling())
            m_HorizontalBar->SetEnabled(false);
        else
            m_HorizontalBar->SetEnabled(true);
    } else
        m_HorizontalBar->SetVisible(false);

    SetVisibleSize(m_VisibleSize);
    SetVisiblePosition(GetVisiblePosition());

    if (IsVerticalVisible())
        m_VerticalBar->SetSize(CPt(m_VerticalBar->GetMinimumSize().x, m_VisibleSize.y));
    if (IsHorizontalVisible())
        m_HorizontalBar->SetSize(CPt(m_VisibleSize.x, m_HorizontalBar->GetMinimumSize().y));

    m_ResizingChildren = false;
}

//=============================================================================
/**
 * Set the size of the inner control that is visible.
 * @param inSize the size of the inner control that is to be visible.
 */
void CScroller::SetVisibleSize(CPt inSize)
{
    m_VisibleSize = inSize;

    // If we have a child then set all of it's properties up.
    CControl *theChild = GetControl();
    if (theChild) {
        CPt theChildSize = m_VisibleSize;
        CPt theMinSize = theChild->GetMinimumSize();

        if (theMinSize.x < theChildSize.x)
            theMinSize.x = theChildSize.x;
        if (theMinSize.y < theChildSize.y)
            theMinSize.y = theChildSize.y;

        // Set the sizes of the child to be either the size of the control or it's minimum size
        if (theMinSize.x > theChildSize.x)
            theChildSize.x = theMinSize.x;
        if (theMinSize.y > theChildSize.y)
            theChildSize.y = theMinSize.y;

        theChild->SetSize(theChildSize);
    }
}

//=============================================================================
/**
 * Check to see is the vertical bar should be visible or not.
 * @return true if the vertical bar should be visible.
 */
bool CScroller::IsVerticalVisible()
{
    if (m_VerticalScrollMode != NEVER) {
        if (m_VerticalScrollMode == ALWAYS || IsVerticalScrolling()) {
            return true;
        }
    }
    return false;
}

//=============================================================================
/**
 * Check to see if the horizontal bar should be visible or not.
 * @return true if the horizontal bar should be visible.
 */
bool CScroller::IsHorizontalVisible()
{
    if (m_HorizontalScrollMode != NEVER) {
        if (m_HorizontalScrollMode == ALWAYS || IsHorizontalScrolling()) {
            return true;
        }
    }
    return false;
}

//=============================================================================
/**
 * Checks to see if the client view should be scrolled vertically or not.
 * This does not mean that the scroll bar is visible or not, just that the client
 * view can be scrolled vertically.
 * @return true if the client min height larger than this height.
 */
bool CScroller::IsVerticalScrolling()
{
    CControl *theChild = GetControl();
    if (theChild != nullptr) {
        CPt mySize = GetSize();
        if (m_HorizontalScrollMode == ALWAYS)
            mySize.y -= m_HorizontalBar->GetSize().y;

        CPt theChildSize = theChild->GetMinimumSize();

        if (theChildSize.y > mySize.y)
            return true;
    }
    return false;
}

//=============================================================================
/**
 * Checks to see if the client view should be scrolled horizontally or not.
 * This does not mean that the scroll bar is visible or not, just that the
 * client view can be scrolled horizontally.
 * @return true if the client min width is larget than this width.
 */
bool CScroller::IsHorizontalScrolling()
{
    CControl *theChild = GetControl();
    if (theChild != nullptr) {
        CPt mySize = GetSize();
        if (m_VerticalScrollMode == ALWAYS)
            mySize.x -= m_VerticalBar->GetSize().x;

        CPt theChildSize = theChild->GetMinimumSize();

        if (theChildSize.x > mySize.x)
            return true;
    }
    return false;
}

//=============================================================================
/**
 * Get the child control that this is wrapping.
 * @return the child control, or nullptr if there is not one.
 */
CControl *CScroller::GetControl()
{
    if (GetChildCount() > 2)
        return GetChildren().GetCurrent()->GetControl();
    return nullptr;
}

//=============================================================================
/**
 * Get the size of this control that the Client is actually visible in.
 * This is not the size of the client, but the size of the area that the client
 * is being drawn to.
 * @return the visible area of the child control.
 */
CPt CScroller::GetVisibleSize()
{
    return m_VisibleSize;
}

//=============================================================================
/**
 * Get the size of the actual child control.
 * This is the total size of the child, and is always at least the size of
 * the visible size.
 * @return the size of the child control.
 */
CPt CScroller::GetContaineeSize()
{
    UICPROFILE(GetContaineeSize);

    CControl *theControl = GetControl();
    CPt theSize;
    if (theControl != nullptr)
        theSize = theControl->GetSize();
    else
        theSize = GetSize();

    return theSize;
}

//=============================================================================
/**
 * Get the offset of the child control inside of this control.
 * @return the offset location of the child.
 */
CPt CScroller::GetVisiblePosition()
{
    return m_VisiblePosition;
}

//=============================================================================
/**
 * Set the visible position of the child control.
 * @param inVisiblePosition the visible position of the child control.
 */
void CScroller::SetVisiblePosition(CPt inVisiblePosition)
{
    if (inVisiblePosition.x < 0)
        inVisiblePosition.x = 0;

    if (inVisiblePosition.y < 0)
        inVisiblePosition.y = 0;

    CPt theMaxVisiblePosition = GetMaxVisiblePosition();

    if (inVisiblePosition.x > theMaxVisiblePosition.x)
        inVisiblePosition.x = theMaxVisiblePosition.x;

    if (inVisiblePosition.y > theMaxVisiblePosition.y)
        inVisiblePosition.y = theMaxVisiblePosition.y;

    if (inVisiblePosition != m_VisiblePosition) {
        CPt theScrolledAmount = m_VisiblePosition - inVisiblePosition;
        m_ScrolledAmount += theScrolledAmount;

        m_VisiblePosition = inVisiblePosition;

        if (IsVerticalScrolling())
            m_VerticalBar->RepositionThumb();
        if (IsHorizontalScrolling())
            m_HorizontalBar->RepositionThumb();

        m_ScrollListeners.FireEvent(&CScrollListener::OnScroll, this, theScrolledAmount);
    }

    CControl *theChild = GetControl();
    if (theChild != nullptr) {
        theChild->SetPosition(-m_VisiblePosition);
    }
}

//=============================================================================
/**
 * Get the maximum allowable offset of the child control.
 * This is where the child would be offset to if the scroll bars were at their
 * far ends.
 * @return the maximum visible position allowed.
 */
CPt CScroller::GetMaxVisiblePosition()
{
    CPt theMaxPoint(0, 0);
    CPt mySize = GetSize();
    CControl *theChild = GetControl();

    mySize.x -= IsVerticalVisible() ? m_VerticalBar->GetSize().x : 0;
    mySize.y -= IsHorizontalVisible() ? m_HorizontalBar->GetSize().y : 0;

    if (theChild != nullptr) {
        CPt theChildSize = theChild->GetSize();

        if (theChildSize.x > mySize.x)
            theMaxPoint.x = theChildSize.x - mySize.x;

        if (theChildSize.y > mySize.y)
            theMaxPoint.y = theChildSize.y - mySize.y;
    }

    return theMaxPoint;
}

void CScroller::OnDraw(CRenderer *inRenderer, CRct &inDirtyRect, bool inIgnoreValidation)
{
    UICPROFILE(OnDraw);

    CRct theDirtyRect;
    bool isInvalidated = IsInvalidated();

    if (isInvalidated || inIgnoreValidation || m_VerticalBar->IsChildInvalidated()
        || m_HorizontalBar->IsChildInvalidated()) {
        if (isInvalidated || inIgnoreValidation)
            DrawBackground(inRenderer);
        Draw(inRenderer);
        theDirtyRect.Or(CRct(GetSize()));
    }

    CControl *theChild = GetControl();
    if (theChild != nullptr) {
        // Create an off screen buffer to draw the child to, this makes it so the child will draw
        // it's
        // entire self to the offscreen section, then we'll only draw the visible part to the actual
        // renderer
        CRct theClippingRect = m_VisibleSize;
        if (m_AddtlClippingRect.size.x != 0 && m_AddtlClippingRect.size.y != 0)
            theClippingRect.And(m_AddtlClippingRect);

        inRenderer->PushClippingRect(theClippingRect);
        inRenderer->PushTranslation(-m_VisiblePosition);
        theChild->OnDraw(inRenderer, theDirtyRect, isInvalidated || inIgnoreValidation);
        inRenderer->PopTranslation();
        inRenderer->PopClippingRect();

        m_ScrolledAmount = CPt(0, 0);
    }

    Invalidate(false);

    CRct theBoundingBox = inRenderer->GetClippingRect();
    theDirtyRect.And(theBoundingBox);
    theDirtyRect.Offset(inRenderer->GetTranslation());
    inDirtyRect.Or(theDirtyRect);
}

//=============================================================================
/**
 * Overrides CControl::Draw to handle custom child buffering.
 * This just allows the child to be drawn offset without actually letting it
 * know that it's offset.
 * @param inRenderer the renderer to draw to.
 */
void CScroller::Draw(CRenderer *inRenderer)
{
    UICPROFILE(Draw);

    CRct theDirtyRct;
    // Only draw the side bar if it's visible
    if (m_VerticalBar->IsVisible()) {
        inRenderer->PushTranslation(m_VerticalBar->GetPosition());
        m_VerticalBar->OnDraw(inRenderer, theDirtyRct, true);
        inRenderer->PopTranslation();
    }

    // only draw the bottom bar if it's visible
    if (m_HorizontalBar->IsVisible()) {
        inRenderer->PushTranslation(m_HorizontalBar->GetPosition());
        m_HorizontalBar->OnDraw(inRenderer, theDirtyRct, true);
        inRenderer->PopTranslation();
    }
}

//=============================================================================
/**
 * Gets the horizontal scroll bar of this scroller.
 * @return the horizontal scroll bar of this scroller.
 */
CScrollerBar *CScroller::GetHorizontalBar()
{
    return m_HorizontalBar;
}

//=============================================================================
/**
 * Gets the vertical scroll bar of this scroller.
 * @return the vertical scroll bar of this scroller.
 */
CScrollerBar *CScroller::GetVerticalBar()
{
    return m_VerticalBar;
}

//=============================================================================
/**
 * Sets the vertical scroll mode of this scroller.
 * This controls when the scroll bars will be visible and when thes will not be
 * visible.
 * @param inScrollMode the vertical scroll mode.
 */
void CScroller::SetVerticalScrollMode(EScrollMode inScrollMode)
{
    m_VerticalScrollMode = inScrollMode;
}

//=============================================================================
/**
 * Gets the vertical scroll mode of this scroller.
 * This controls when the scroll bars will be visible and when they will not be
 * visible.
 * @return the vertical scroll mode.
 */
CScroller::EScrollMode CScroller::GetVerticalScrollMode()
{
    return m_VerticalScrollMode;
}

//=============================================================================
/**
 * Sets the horizontal scroll mode of this scroller.
 * This controls when the scroll bars will be visible and when they will not be
 * visible.
 * @param inScrollMode the vertical scroll mode.
 */
void CScroller::SetHorizontalScrollMode(EScrollMode inScrollMode)
{
    m_HorizontalScrollMode = inScrollMode;
}

//=============================================================================
/**
 * Gets the horizontal scroll mode of this scroller.
 * This controls when the scroll bars will be visible and when they will not be
 * visible.
 * @return the horizontal scroll mode.
 */
CScroller::EScrollMode CScroller::GetHorizontalScrollMode()
{
    return m_HorizontalScrollMode;
}

void CScroller::OnChildSizeChanged(CControl *inControl)
{
    UICPROFILE(OnChildSizeChanged);

    CControl *theChild = GetControl();

    if (inControl == theChild) {
        CPt theChildSize = theChild->GetSize();
        CPt theMinSize = GetVisibleSize();
        if (theChildSize.x < theMinSize.x) {
            theChild->SetMinimumSize(CPt(theMinSize.x, theChild->GetMinimumSize().y));
            theChildSize.x = theMinSize.x;
        }
        if (theChildSize.y < theMinSize.y) {
            theChild->SetMinimumSize(CPt(theChild->GetMinimumSize().x, theMinSize.y));
            theChildSize.y = theMinSize.y;
        }

        if (theChildSize != theChild->GetSize())
            theChild->SetSize(theChildSize);
        else if (!m_ResizingChildren) {
            RecalcLayout();
            Invalidate();
        }
    }
}

//=============================================================================
/**
 * Add a listener to get notified when this control scrolls.
 * @param inListener the listener to be notified when this scrolls.
 */
void CScroller::AddScrollListener(CScrollListener *inListener)
{
    m_ScrollListeners.AddListener(inListener);
}

//=============================================================================
/**
 * Remove a listener from the list of listeners to be notified when this scrolls.
 * @param inListener the listener to be removed from the list of listeners.
 */
void CScroller::RemoveScrollListener(CScrollListener *inListener)
{
    m_ScrollListeners.RemoveListener(inListener);
}

void CScroller::SetAdditionalClippingRect(CRct inAddtlClippingRect)
{
    m_AddtlClippingRect = inAddtlClippingRect;
}

bool CScroller::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CPt theVertPoint = inPoint - m_VerticalBar->GetPosition();
    if (!m_VerticalBar->HitTest(inPoint) && !m_HorizontalBar->HitTest(inPoint)) {
        m_IsMouseDown = true;
    }

    return CControl::OnMouseDown(inPoint, inFlags);
}

void CScroller::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_IsMouseDown) {
        m_CurrentTickTock = std::shared_ptr<UICDM::ISignalConnection>();

        CPt theOffset;
        if ((inPoint.x < 0))
            theOffset.x = inPoint.x;
        else if (inPoint.x > m_VisibleSize.x)
            theOffset.x = inPoint.x - m_VisibleSize.x;

        //		if ( ( inPoint.y < 0 && inPoint.y < m_PrevMousePoint.y ) || ( inPoint.y >
        //m_VisibleSize.y && inPoint.y > m_PrevMousePoint.y ) )
        //			theOffset.y = inPoint.y - m_PrevMousePoint.y;

        SetVisiblePosition(m_VisiblePosition + theOffset);

        if (theOffset.x != 0) {
            m_ScrollingDir = theOffset;
            m_MousePos = inPoint;
            m_MouseFlags = inFlags;
            m_CurrentTickTock = ITickTock::GetInstance().AddTimer(
                m_DelayTime, true, std::bind(&CScroller::OnTimer, this),
                "CScroller::OnMouseMove::" + GetName());
            //			OnTimer( );
        }
    }

    CControl::OnMouseMove(inPoint, inFlags);
}

void CScroller::OnTimer()
{
    CPt theOffset;
    const auto theBeginCurrentTime = QDateTime::currentMSecsSinceEpoch();
    if (m_ScrollingDir.x > 0)
        theOffset.x = m_OffsetAmmount;
    else if (m_ScrollingDir.x < 0)
        theOffset.x = -m_OffsetAmmount;

    SetVisiblePosition(m_VisiblePosition + theOffset);
    CControl::OnMouseMove(m_MousePos, m_MouseFlags);
    const auto theEndCurrentTime = QDateTime::currentMSecsSinceEpoch();
    AdjustDelayTimeAccordingToOnTimerTime(theEndCurrentTime - theBeginCurrentTime);
}

//=============================================================================
/**
 * This function will adjust the delay time of the auto scroller depending on how long the timer
 * took
 * the numbers in here are pretty arbitrary and this should be redone at some point.
 * @param inTime the amount of time that the timer took
 */
void CScroller::AdjustDelayTimeAccordingToOnTimerTime(unsigned long inTime)
{
    for (long theIndex = 2; theIndex < 6; theIndex++) {
        if ((inTime)*theIndex > m_DelayTime) {
            m_DelayTime = static_cast<unsigned long>(m_DelayTime * 10 / theIndex);
            m_OffsetAmmount = static_cast<unsigned long>(m_OffsetAmmount * 10 / theIndex);
            m_CurrentTickTock = ITickTock::GetInstance().AddTimer(
                m_DelayTime, true, std::bind(&CScroller::OnTimer, this),
                "CScroller::AdjustDelayTimeAccordingToOnTimerTime");
            break;
        }
    }
}

void CScroller::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    m_IsMouseDown = false;

    CControl::OnMouseUp(inPoint, inFlags);

    m_CurrentTickTock = std::shared_ptr<UICDM::ISignalConnection>();
}

void CScroller::OnLoseFocus()
{
    m_IsMouseDown = false;
    m_CurrentTickTock = std::shared_ptr<UICDM::ISignalConnection>();
    CControl::OnLoseFocus();
}

//=============================================================================
/**
 * Handles mouse wheel messages to scroll the view.
 */
bool CScroller::OnMouseWheel(CPt inPoint, long inScrollAmount, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseWheel(inPoint, inScrollAmount, inFlags)) {
        if (inScrollAmount < 0)
            SetVisiblePosition(CPt(GetVisiblePosition().x, GetVisiblePosition().y + 60));
        else
            SetVisiblePosition(CPt(GetVisiblePosition().x, GetVisiblePosition().y - 60));
    }
    return true;
}

//=============================================================================
/**
 * Handles messages from children to scroll the view to appropriate loc.
 */
void CScroller::EnsureVisible(CRct inRect)
{
    CControl *theChild = GetControl();
    inRect.position -= theChild->GetPosition();

    CPt theVisSize = GetVisibleSize();
    CPt theVisPos = GetVisiblePosition();

    // Check to see if the top is off the top.
    if (inRect.position.y < theVisPos.y)
        theVisPos.y = inRect.position.y;
    // Check to see if the bottom is off the bottom.
    else if (inRect.position.y + inRect.size.y > theVisPos.y + theVisSize.y)
        theVisPos.y = inRect.position.y + inRect.size.y - theVisSize.y;

    SetVisiblePosition(theVisPos);
}
