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
#ifndef INCLUDED_TEXT_BUTTON_H
#define INCLUDED_TEXT_BUTTON_H 1
#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Renderer.h"
#include "OffscreenRenderer.h"
#include "CoreUtils.h"
#include "StudioPreferences.h"
#include "ButtonControl.h"
#include "CColor.h"

#include <QSize>

const long MARGIN_X = 2;
const long MARGIN_Y = 0;

//=============================================================================
/**
 * Template class for making buttons that have text on them.  The text can be
 * displayed with or without the button's icon.  In order to use this class,
 * you must instantiate it with some sort of CButtonControl to act as the base
 * class.
 */
template <class TButton>
class CTextButton : public TButton
{
    // Enumerations
public:
    enum EAlignment {
        ALIGNMENT_LEFT,
        ALIGNMENT_CENTER,
        ALIGNMENT_RIGHT,
        ALIGNMENT_VCENTER ///< Only align center vertically
    };

    // Constuction/Destruction
public:
    CTextButton();
    virtual ~CTextButton();
    // Access
public:
    void SetText(const Q3DStudio::CString &inText);
    void SetBoldText(const Q3DStudio::CString &inText);
    Q3DStudio::CString GetText();
    void SetTextColorUp(const CColor &inColor);
    void SetTextColorDown(const CColor &inColor);
    CColor GetTextColorUp();
    CColor GetTextColorDown();
    void SetTextAlignment(EAlignment inAlignment);
    long GetTextAlignment();

    // Implementation
protected:
    virtual void Render(CRenderer *inRenderer);
    virtual void Resize();
    void SizeToFitTextLen();

    // Field members
protected:
    Q3DStudio::CString m_Text; ///< Text
    CColor m_TextColorUp; ///< Color of text
    CColor m_TextColorDown; ///< Color of text
    CPt m_TextPos; ///< Position of text
    EAlignment m_TextAlign; ///< Alignment of text
    BOOL m_BoldText;
};

//==============================================================================
//	Template implemenations
//==============================================================================

//=============================================================================
/**
 * Constructor
 */
template <class TButton>
CTextButton<TButton>::CTextButton()
    : TButton()
    , m_TextColorUp(CStudioPreferences::GetNormalColor())
    , m_TextColorDown(CStudioPreferences::GetNormalColor())
    , m_TextPos(MARGIN_X, MARGIN_Y / 2)
    , m_TextAlign(ALIGNMENT_LEFT)
    , m_BoldText(FALSE)
{
}

//=============================================================================
/**
 * Destructor
 */
template <class TButton>
CTextButton<TButton>::~CTextButton()
{
}

//=============================================================================
/**
 * Sets the text displayed on this button to the specified string.
 * @param inText new text to be displayed on this button
 */
template <class TButton>
void CTextButton<TButton>::SetText(const Q3DStudio::CString &inText)
{
    if (m_Text != inText) {
        m_Text = inText;
        SizeToFitTextLen();
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * Sets the text bold to be displayed on this button to the specified string.
 * @param inText new text to be displayed on this button
 */
template <class TButton>
void CTextButton<TButton>::SetBoldText(const Q3DStudio::CString &inText)
{
    m_Text = inText;
    m_BoldText = TRUE;
    SizeToFitTextLen();
    TButton::Invalidate();
}

//=============================================================================
/**
 * @return the text currently displayed on this button
 */
template <class TButton>
Q3DStudio::CString CTextButton<TButton>::GetText()
{
    return m_Text;
}

//=============================================================================
/**
 * Sets the color of the text displayed on this button in the up state
 * @param inColor new text color
 */
template <class TButton>
void CTextButton<TButton>::SetTextColorUp(const CColor &inColor)
{
    if (m_TextColorUp != inColor) {
        m_TextColorUp = inColor;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * Sets the color of the text displayed on this button in the down state
 * @param inColor new text color
 */
template <class TButton>
void CTextButton<TButton>::SetTextColorDown(const CColor &inColor)
{
    if (m_TextColorDown != inColor) {
        m_TextColorDown = inColor;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * @return the color of the text displayed on this button in the up state
 */
template <class TButton>
CColor CTextButton<TButton>::GetTextColorUp()
{
    return m_TextColorUp;
}

//=============================================================================
/**
 * @return the color of the text displayed on this button in the down state
 */
template <class TButton>
CColor CTextButton<TButton>::GetTextColorDown()
{
    return m_TextColorDown;
}

//=============================================================================
/**
 * Set the alignment of the text.
 */
template <class TButton>
void CTextButton<TButton>::SetTextAlignment(EAlignment inAlignment)
{
    if (inAlignment != m_TextAlign) {
        m_TextAlign = inAlignment;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * @return the alignment of the text displayed on this button
 */
template <class TButton>
long CTextButton<TButton>::GetTextAlignment()
{
    return m_TextAlign;
}

//=============================================================================
/**
 * Draws the button, then draws the specified text onto the button.
 * @param inRenderer the renderer to draw to
 */
template <class TButton>
void CTextButton<TButton>::Render(CRenderer *inRenderer)
{
    // Draw the button icon
    TButton::Render(inRenderer);

    // Detertmine position of text based off horizontal alignment
    float theTextPosX(static_cast<float>(m_TextPos.x));
    float theTextPosY(static_cast<float>(m_TextPos.y));

    long theImageX = 0;
    long theImageY = 0;
    if (!TButton::GetCurrentImage().isNull()) {
        theImageX += TButton::GetCurrentImage().size().width();
        theImageY += TButton::GetCurrentImage().size().height();
    }

    switch (m_TextAlign) {
    case ALIGNMENT_CENTER: {
        const auto textSize = inRenderer->GetTextSize(m_Text.toQString());
        theTextPosX = (TButton::GetSize().x - textSize.width() + theImageX) / 2;
        theTextPosY = static_cast<float>(m_TextPos.y);
    } break;

    case ALIGNMENT_RIGHT: {
        const auto textSize = inRenderer->GetTextSize(m_Text.toQString());
        theTextPosX = TButton::GetSize().x - textSize.width();
        theTextPosY = static_cast<float>(m_TextPos.y);
    } break;

    case ALIGNMENT_VCENTER: {
      const auto textSize = inRenderer->GetTextSize(m_Text.toQString());
        theTextPosY = (TButton::GetSize().y - textSize.height() + theImageY) / 2;
        theTextPosX = static_cast<float>(m_TextPos.x);
    }
    // Default is LEFT justification
    default:
    case ALIGNMENT_LEFT:
        theTextPosX += theImageX;
        break;
    }

    typename TButton::EButtonState theState = TButton::GetButtonState();
    ::CColor theTextColor = m_TextColorUp;
    if (theState == CButtonControl::EBUTTONSTATE_DOWN)
        theTextColor = m_TextColorDown;
    if (TButton::IsEnabled() == false)
        theTextColor = CStudioPreferences::GetDisabledTextColor();

    inRenderer->PushPen(theTextColor);

    // Draw the text
    const auto buttonSize = TButton::GetSize();
    const QRect rect(0, 0, buttonSize.x, buttonSize.y);
    if (m_BoldText && !m_Text.IsEmpty()) {
        inRenderer->DrawBoldText(theTextPosX, theTextPosY, m_Text.toQString(), rect, theTextColor);
    } else {
        if (!m_Text.IsEmpty())
            inRenderer->DrawText(theTextPosX, theTextPosY, m_Text.toQString(), rect, theTextColor);
    }

    inRenderer->PopPen();
}

//=============================================================================
/**
 *	this function calculates the text length in the button control and sets the size of the
 *button
 */
template <class TButton>
void CTextButton<TButton>::Resize()
{
    TButton::Resize();
    SizeToFitTextLen();
}

//=============================================================================
/**
 *	this function calculates the text length in the button control and sets the size of the
 *button
 */
template <class TButton>
void CTextButton<TButton>::SizeToFitTextLen()
{
    // If auto-resizing of the text field is enabled
    if (TButton::m_AutoSize) {
        // Resize the control to fit the text, plus the buffer gap
        COffscreenRenderer theRenderer(CRct(0, 0, 1, 1));
        CPt theSize;
        const auto textSize = theRenderer.GetTextSize(m_Text.toQString());
        int theX = textSize.width();
        int theY = textSize.height();

        if (!TButton::GetCurrentImage().isNull())
            theX += TButton::GetCurrentImage().size().width();

        // MARGIN_LEN * 2 is just to add some space between the text and the button
        theSize.x = ::dtol(theX) + (MARGIN_X * 2);
        theSize.y = static_cast<long>(theY + (MARGIN_Y * 2));
        TButton::SetSize(theSize);
        TButton::SetMaximumSize(theSize);
        TButton::Invalidate();
    }
}
#endif // INCLUDED_TEXT_BUTTON_H
