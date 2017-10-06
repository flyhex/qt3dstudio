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
#include "ButtonControl.h"
#include "Renderer.h"
#include "ResourceCache.h"

//=============================================================================
/**
 * Constructor
 *
 * @param inToggleButton true if this is suppose to be a toggle button.  Toggle
 * buttons stay pressed when clicked on and have to be clicked again in order
 * to release.  Normal buttons return to the up state when the button is released.
 */
CButtonControl::CButtonControl()
    : m_State(EBUTTONSTATE_UP)
    , m_AutoSize(true)
    , m_MouseEnter(true)
    , m_IsMouseDown(false)
    , m_VCenterImage(false)
    , m_HCenterImage(false)
    , m_Margin(0)
{
}

//=============================================================================
/**
 * Destructor
 */
CButtonControl::~CButtonControl()
{
}

//=============================================================================
/**
 * Returns the current state of the button.  A button can be "up", "down", or
 * "indeterminate".  The up state is the normal state of the button.  The down
 * state is when the mouse is pressing the button.  The indeterminate state is
 * provided for subclasses and not used here.
 * @return the button's current state
 */
CButtonControl::EButtonState CButtonControl::GetButtonState() const
{
    return m_State;
}

//=============================================================================
/**
 * Sets the current state of the button.  No listeners are notified.
 * @param inState new state of the button
 */
void CButtonControl::SetButtonState(EButtonState inState)
{
    if (m_State == inState)
        return;

    m_State = inState;
    Invalidate();
}

//=============================================================================
/**
 * Sets the image to use when this button is in the up state.  A button is "up"
 * if it is not disabled, not depressed, and the mouse is not over it.
 * @param inImage image to be loaded when this button is up
 */
void CButtonControl::SetUpImage(const QPixmap &inImage)
{
    m_ImageUp = inImage;
    Resize();
}

//=============================================================================
/**
 * Sets the image to use when this button is in the up state.  A button is "up"
 * if it is not disabled, not depressed, and the mouse is not over it.
 * @param inImageName name of the image to be loaded when this button is up
 */
void CButtonControl::SetUpImage(const QString &inImageName)
{
    QPixmap theImage ;
    if (!inImageName.isEmpty())
        theImage = CResourceCache::GetInstance()->GetBitmap(inImageName);
    SetUpImage(theImage);
}

//=============================================================================
/**
 * Sets the image to use when this button is in the down state.
 * @param inImage image to be loaded when this button is down
 */
void CButtonControl::SetDownImage(const QPixmap &inImage)
{
    m_ImageDown = inImage;
}

//=============================================================================
/**
 * Sets the image to use when this button is in the down state.
 * @param inImageName name of the image to be loaded when this button is down
 */
void CButtonControl::SetDownImage(const QString &inImageName)
{
    QPixmap theImage;
    if (!inImageName.isEmpty())
        theImage = CResourceCache::GetInstance()->GetBitmap(inImageName);
    SetDownImage(theImage);
}

//=============================================================================
/**
 * Sets the image to use when this button the mouse is over the button.  To
 * leave the button displaying the "up" state, pass NULL to this function.
 * @param inImage image to be loaded when the mouse is over this button
 */
void CButtonControl::SetOverImage(const QPixmap &inImage)
{
    m_ImageOver = inImage;
}

//=============================================================================
/**
 * Sets the image to use when this button the mouse is over the button.  To
 * leave the button displaying the "up" state, pass an empty string to this function.
 * @param inImageName name of the image to be loaded when the mouse is over this button
 */
void CButtonControl::SetOverImage(const QString &inImageName)
{
    QPixmap theImage;
    if (!inImageName.isEmpty())
        theImage = CResourceCache::GetInstance()->GetBitmap(inImageName);
    SetOverImage(theImage);
}

//=============================================================================
/**
 * Sets the image to use when this button is disabled, when the button is in a DOWN state
 * @param inImage image to be loaded when this button is disabled
 */
void CButtonControl::SetDisabledImage(const QPixmap &inImage)
{
    m_ImageDownDisabled = inImage;
}

//=============================================================================
/**
 * Sets the image to use when this button is in the disabled state.
 * @param inImageName name of the image to be loaded when this button is disabled
 */
void CButtonControl::SetDisabledImage(const QString &inImageName)
{
    QPixmap theImage;
    if (!inImageName.isEmpty())
        theImage = CResourceCache::GetInstance()->GetBitmap(inImageName);
    SetDisabledImage(theImage);
}

//=============================================================================
/**
 * In cases, where there is a different (disabled) image for button UP state
 */
void CButtonControl::SetUpDisabledImage(const QString &inImageName)
{
    if (!inImageName.isEmpty())
        m_ImageUpDisabled = CResourceCache::GetInstance()->GetBitmap(inImageName);
}

//=============================================================================
/**
 * Allows you to enable or disable auto sizing for this button.  Auto sizing
 * causes the button's size to change whenever the "Up" image is changed for
 * this button.  Button size does NOT change as the button state changes.
 * @return true if this button is currently in Auto Size mode
 */
void CButtonControl::SetAutoSize(bool inEnableAutoSize)
{
    m_AutoSize = inEnableAutoSize;
    Resize();
}

//=============================================================================
/**
 * Determines if this button will auto size or not.  Auto sizing causes the
 * button's size to change whenever the "Up" image is changed for this button.
 * Button size does NOT change as the button state changes.
 * @return true if this button is currently in Auto Size mode
 */
bool CButtonControl::GetAutoSize()
{
    return m_AutoSize;
}

void CButtonControl::SetCenterImage(bool inVCenter, bool inHCenter)
{
    if (m_VCenterImage == inVCenter && m_HCenterImage == inHCenter)
        return;
    m_VCenterImage = inVCenter;
    m_HCenterImage = inHCenter;
    Invalidate();
}

void CButtonControl::SetMarginImage(long inMargin)
{
    m_Margin = inMargin;
}

//=============================================================================
/**
 * Draws the button based on the current state
 */
void CButtonControl::Draw(CRenderer *inRenderer)
{
    Render(inRenderer);
}

//=============================================================================
/**
 * Handles mouse down on the button.  Flags the button as down which results
 * in some possible drawing changes.
 */
bool CButtonControl::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDown(inPoint, inFlags)) {
        m_IsMouseDown = true;
        SetButtonState(EBUTTONSTATE_DOWN);
        SigButtonDown(this);
        Invalidate();
    }

    return true;
}

//=============================================================================
/**
 * Handles mouse up on the button.  Sets the button to the "up" state and fires
 * an event to interested listeners.
 * @param inPoint location of the mouse when the event occurred
 * @param inFlags state of the modifier keys when this event occurred
 */
void CButtonControl::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseUp(inPoint, inFlags);

    SetButtonState(EBUTTONSTATE_UP);

    if (IsMouseOver()) {
        SigButtonUp(this);
        if (m_IsMouseDown)
            SigClicked(this);
    }
    m_IsMouseDown = false;

    Invalidate();
}

//=============================================================================
/**
 * This function is called when the mouse passes over this button.  The button
 * is invalidated so that any custom drawing for when the mouse is over the
 * button can occurr.
 * @param inPoint location of the mouse when the event occurred
 * @param inFlags state of the modifier keys when this event occurred
 */
void CButtonControl::OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOver(inPoint, inFlags);

    // Only invalidate the first time the mouse enters this control
    if (m_MouseEnter) {
        Invalidate();
        m_MouseEnter = false;
    }
}

//=============================================================================
/**
 * This function is called when the mouse exits the bounds of this button.  The
 * button is invalidated so that any custom drawing caused by the OnMouseOver
 * event can be undone.
 * @param inPoint location of the mouse when the event occurred
 * @param inFlags state of the modifier keys when this event occurred
 */
void CButtonControl::OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOut(inPoint, inFlags);
    Invalidate();
    // Change the flag so that next time we get a mouse over event, we know to invalidate this
    // button
    m_MouseEnter = true;
}

//=============================================================================
/**
 * Gets the current bitmap depending on the state of the button, postion of the
 * mouse, etc.  Returns the image for the up state by default.
 * @return the currently displayed image or NULL if there is no image on this button
 */
QPixmap CButtonControl::GetCurrentImage() const
{
    // Default to the up state
    auto theImage = m_ImageUp;

    EButtonState theState = GetButtonState();

    // If the mouse is over the button, switch to the mouse over image
    if (IsMouseOver() && !m_ImageOver.isNull())
        theImage = m_ImageOver;

    // If the button is currently pressed, this cancels the up and over states, so return the down
    // image
    if (theState == EBUTTONSTATE_DOWN && !m_ImageDown.isNull())
        theImage = m_ImageDown;

    // If this button is disabled just return the disabled image (overrides any other states)
    if (!IsEnabled()) { // use the specified butuon UP disabled if specified, otherwise fall back on
                        // the DOWN ( that's how the legacy system does it )
        if (theState == EBUTTONSTATE_UP && !m_ImageUpDisabled.isNull())
            theImage = m_ImageUpDisabled;
        else if (!m_ImageDownDisabled.isNull())
            theImage = m_ImageDownDisabled;
    }
    return theImage;
}

//=============================================================================
/**
 * Get the size of the image that this control is currently using.
 * @return the size of the ImageUp image.
 */
QSize CButtonControl::GetImageSize() const
{
    // Get the image map.
    QPixmap theImage = GetCurrentImage();

    return theImage.size();
}

//=============================================================================
/**
 * Override of invalidate to invalidate the parent as well.
 * @param inIsInvalidated true if this is to be invalidated.
 */
void CButtonControl::Invalidate(bool inIsInvalidated /* = true */)
{
    // Since the button is transparent the parent needs to be drawn as well.
    if (inIsInvalidated && GetParent() != nullptr)
        GetParent()->Invalidate();

    CControl::Invalidate(inIsInvalidated);
}

//=============================================================================
/**
 * Draws the appropriate image for the current button state to the renderer.
 * @param inRenderer the renderer to draw to
 */
void CButtonControl::Render(CRenderer *inRenderer)
{
    CPt thePos(0, 0);
    const auto theImage = GetCurrentImage();

    if (m_VCenterImage || m_HCenterImage) {
        const auto theSize = theImage.size();

        if (m_HCenterImage)
            thePos.x = (GetSize().x / 2) - (theSize.width() / 2);

        if (m_VCenterImage)
            thePos.y = (GetSize().y / 2) - (theSize.height() / 2);
    }

    thePos.x += m_Margin;

    if (!theImage.isNull())
        inRenderer->DrawBitmap(thePos, theImage);
}

//=============================================================================
/**
 * If auto sizing is enabled, this function resizes the button to be the same
 * size as the image that it contains.  Note: it does not set the Min/Max sizes.
 */
void CButtonControl::Resize()
{
    if (m_AutoSize) {
        auto theSize = m_ImageUp.size();

        SetSize(CPt(theSize.width(), theSize.height()));
    }
}
