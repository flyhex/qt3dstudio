/****************************************************************************
**
** Copyright (C) 2002 Anark Corporation.
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

#ifndef INCLUDED_EDIT_IN_PLACE_H
#define INCLUDED_EDIT_IN_PLACE_H 1

#pragma once

#include "Renderer.h"
#include "Pt.h"
#include "StudioPreferences.h"

//==============================================================================
//	Forwards
//==============================================================================
class CRenderer;

template <class T>
class CEditInPlace : public T
{
public:
    CEditInPlace();
    virtual ~CEditInPlace();

    bool OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnLoseFocus() override;
    void OnGainFocus() override;
    void Draw(CRenderer *inRenderer) override;
    void SetEditable(bool inIsEditable);
    void SetEditMode(bool inEditMode);
    void EnterText(bool inHighlight) override;
    void Invalidate(bool inInvalidate = true)  override;
    bool GetEditMode();
    static long GetRightBuffer();

protected:
    bool m_IsInEditMode; ///< If in edit mode, the user is currently editing the control
    bool m_IsEditable; ///< Can only enter edit mode when the control is editable

private:
    static const long s_RightBuffer = 6;
};

template <class T>
CEditInPlace<T>::CEditInPlace()
    : T()
    , m_IsInEditMode(false)
    , m_IsEditable(true)
{
    T::SetReadOnly(true);
    T::SetFillBackground(false);
}

template <class T>
CEditInPlace<T>::~CEditInPlace()
{
}

//==============================================================================
/**
 * Handles the mouse double-click event.  Enters the text into edit mode so that
 * the user can change its value.
 * @param inPoint location of the mouse
 * @param inFlags modifier flags for the mouse
 * @return true if this message should not be passed to any other children, otherwise false
 */
template <class T>
bool CEditInPlace<T>::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_IsEditable && CStudioPreferences::IsSudoMode()) {
        SetEditMode(true);
        this->SetSelection(0, T::GetString().Length());
        return true;
    }
    return T::OnMouseDoubleClick(inPoint, inFlags);
}

template <class T>
bool CEditInPlace<T>::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (T::GetParent()->HasFocus(this) && !(inFlags & CHotKeys::MOUSE_RBUTTON))
        SetEditMode(true);
    return T::OnMouseDown(inPoint, inFlags);
}

//==============================================================================
/**
 * Called when this control loses focus.  Turns off edit mode and redraws the
 * control.
 */
template <class T>
void CEditInPlace<T>::OnLoseFocus()
{
    T::OnLoseFocus();
    SetEditMode(false);
    Invalidate();
}

//==============================================================================
/**
 * Handles most of the drawing, with some help from the parent class.
 * @param inRenderer Renderer to draw to
 */
template <class T>
void CEditInPlace<T>::Draw(CRenderer *inRenderer)
{
    CRct theRect = CRct(T::GetSize()); // CRct( CPt( 0, 0 ), CPt( CalculateCharWidths( inRenderer ) +
                                    // s_RightBuffer, GetSize( ).y ) );
    CColor theOutlineColor = CColor(0, 0, 0);

    T::SetAbsoluteSize(theRect.size);

    bool theFillFlag = T::m_FillBackground;

    if (theFillFlag && !m_IsInEditMode) {
        T::SetFillBackground(false);
        inRenderer->FillSolidRect(theRect, T::m_BackgroundColorNoFocus);
    }

    if (m_IsInEditMode)
        T::SetTextColor(CColor(0, 0, 0));

    inRenderer->PushClippingRect(theRect);
    T::Draw(inRenderer);
    inRenderer->PopClippingRect();

    if (!m_IsInEditMode)
        T::SetFillBackground(theFillFlag);

    if (m_IsInEditMode)
        inRenderer->DrawRectOutline(theRect, theOutlineColor, theOutlineColor, theOutlineColor,
                                    theOutlineColor);
}

//==============================================================================
/**
 * Enables or disables this control from being able to enter "Edit Mode" when
 * SetEditMode() is called.
 * @param inIsEditable true if you want the control to be editable when double-clicked, otherwise
 * false
 */
template <class T>
void CEditInPlace<T>::SetEditable(bool inIsEditable)
{
    m_IsEditable = inIsEditable;

    if (!m_IsEditable && m_IsInEditMode)
        SetEditMode(false);
}

//==============================================================================
/**
 * Starts or stops "Edit Mode".  While in Edit Mode, the user can change the
 * text, move the caret, and highlight the text.  An edit box is also drawn
 * around the text.
 * @param inEditMode true to turn on Edit Mode, false to turn off Edit Mode
 */
template <class T>
void CEditInPlace<T>::SetEditMode(bool inEditMode)
{
    if (m_IsEditable) {
        m_IsInEditMode = inEditMode;
        T::SetFillBackground(!m_IsInEditMode);
        T::SetReadOnly(!m_IsInEditMode);
        if (!m_IsInEditMode)
            T::FireCommitEvent();
    }
}

//==============================================================================
/**
 * Overriden from the parent to cause this control to lose focus when the Enter
 * button is pressed on the keyboard.
 * @param inHighlight true to highlight all of the text, false to just change the string
 */
template <class T>
void CEditInPlace<T>::EnterText(bool inHighlight)
{
    T::EnterText(inHighlight);
    OnLoseFocus();
}

//==============================================================================
/**
 * Overriden to also invalidate/validate the parent.
 * @param inInvalidate true to invalidate this control and the parent control
 */
/*
template<class T>
void CEditInPlace<T>::Invalidate( bool inInvalidate )
{
        if ( inInvalidate && GetParent() )
                GetParent()->Invalidate( inInvalidate );

        CControl::Invalidate( inInvalidate );
}*/

//==============================================================================
/**
 * Override to avoid selecting all the text unless in edit mode.
 */
template <class T>
void CEditInPlace<T>::OnGainFocus()
{
    if (m_IsInEditMode && m_IsEditable)
        T::OnGainFocus();
}

template <class T>
bool CEditInPlace<T>::GetEditMode()
{
    return m_IsInEditMode;
}

template <class T>
long CEditInPlace<T>::GetRightBuffer()
{
    return s_RightBuffer;
}

template <class T>
bool CEditInPlace<T>::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers)
{
    bool theReturn = false;

    if (m_IsInEditMode) {
        T::DisplayContextMenu(inPoint);
        theReturn = true;
    }

    return theReturn;
}

//==============================================================================
/**
 * Overriden to also invalidate/validate the parent.
 * @param inInvalidate true to invalidate this control and the parent control
 */
template <class T>
void CEditInPlace<T>::Invalidate(bool inInvalidate)
{
    if (inInvalidate && T::GetParent())
        T::GetParent()->Invalidate(inInvalidate);

    T::Invalidate(inInvalidate);
}

#endif // INCLUDED_EDIT_IN_PLACE_H
