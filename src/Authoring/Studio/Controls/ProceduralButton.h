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
#ifndef INCLUDED_PROCEDURAL_BUTTON_H
#define INCLUDED_PROCEDURAL_BUTTON_H 1
#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Renderer.h"
#include "ButtonControl.h"
#include "StudioPreferences.h"

//=============================================================================
/**
 * Template class for making procedural buttons.  A procedural button has
 * additional drawing options compared to a normal button.  You must specify
 * a derivative of CButtonControl as the class for the procedural button to
 * extend.
 */
template <typename TButton>
class CProceduralButton : public TButton
{
public:
    /// Fill color styles for procedurally drawn buttons
    enum EFillStyle {
        EFILLSTYLE_GRADIENT, ///< Button background is filled using a gradient
        EFILLSTYLE_FLOOD, ///< Button background is filled in a solid color
        EFILLSTYLE_NONE ///< Button background is not filled at all
    };

    /// Struct enabling you to specify whether or not to draw each border of the button individually
    struct SBorderOptions
    {
        bool m_DrawLeft; ///< If true, you want the left border to be drawn
        bool m_DrawTop; ///< If true, you want the top border to be drawn
        bool m_DrawRight; ///< If true, you want the right border to be drawn
        bool m_DrawBottom; ///< If true, you want the bottom border to be drawn

        /// Default Constructor - makes all borders be drawn
        SBorderOptions()
        {
            m_DrawLeft = true;
            m_DrawTop = true;
            m_DrawRight = true;
            m_DrawBottom = true;
        }

        /// Constructor to initialize each member of the struct individually
        SBorderOptions(bool inLeft, bool inTop, bool inRight, bool inBottom)
        {
            m_DrawLeft = inLeft;
            m_DrawTop = inTop;
            m_DrawRight = inRight;
            m_DrawBottom = inBottom;
        }
    };

public:
    CProceduralButton();
    virtual ~CProceduralButton();

    void SetFillStyleUp(EFillStyle inStyle);
    void SetFillStyleDown(EFillStyle inStyle);
    void SetFillStyleDisabled(EFillStyle inStyle);
    void SetFillStyleOver(EFillStyle inStyle);
    void SetFillStyleAll(EFillStyle inStyle);
    void SetFillColorUp(const CColor &inColor);
    void SetFillColorDown(const CColor &inColor);
    void SetFillColorDisabled(const CColor &inColor);
    void SetFillColorOver(const CColor &inColor);

    void SetBorderVisibilityAll(const SBorderOptions &inBorderOptions);
    void SetBorderVisiblityUp(const SBorderOptions &inBorderOptions);
    void SetBorderVisiblityDown(const SBorderOptions &inBorderOptions);
    void SetBorderVisiblityDisabled(const SBorderOptions &inBorderOptions);
    void SetBorderVisiblityOver(const SBorderOptions &inBorderOptions);

    void SetLeftBorderColor(const CColor &inColor);
    void SetTopBorderColor(const CColor &inColor);
    void SetRightBorderColor(const CColor &inColor);
    void SetBottomBorderColor(const CColor &inColor);

protected:
    virtual void Render(CRenderer *inRenderer);
    void DrawBackground(CRenderer *inRenderer);
    void DrawBorder(CRenderer *inRenderer);
    EFillStyle GetCurrentFillStyle();
    CColor GetCurrentBackgroundColor();
    SBorderOptions GetCurrentBorderOptions();

protected:
    EFillStyle m_UpFillStyle;
    EFillStyle m_DownFillStyle;
    EFillStyle m_DisabledFillStyle;
    EFillStyle m_OverFillStyle;
    CColor m_UpColor;
    CColor m_DownColor;
    CColor m_DisabledColor;
    CColor m_OverColor;
    CColor m_LeftBorderColor;
    CColor m_TopBorderColor;
    CColor m_RightBorderColor;
    CColor m_BottomBorderColor;
    SBorderOptions m_UpBorders;
    SBorderOptions m_DownBorders;
    SBorderOptions m_DisabledBorders;
    SBorderOptions m_OverBorders;
};

//==============================================================================
//	Template implemenations
//==============================================================================

//=============================================================================
/**
 * Constructor
 */
template <typename TButton>
CProceduralButton<TButton>::CProceduralButton()
    : TButton()
    , m_UpFillStyle(EFILLSTYLE_GRADIENT)
    , m_DownFillStyle(EFILLSTYLE_GRADIENT)
    , m_DisabledFillStyle(EFILLSTYLE_NONE)
    , m_OverFillStyle(EFILLSTYLE_GRADIENT)
    , m_UpColor(CStudioPreferences::GetBaseColor())
    , m_DownColor(CStudioPreferences::GetButtonDownColor())
    , m_DisabledColor(CStudioPreferences::GetBaseColor())
    , m_OverColor(CStudioPreferences::GetBaseColor())
    , m_LeftBorderColor(CStudioPreferences::GetButtonShadowColor())
    , m_TopBorderColor(CStudioPreferences::GetButtonShadowColor())
    , m_RightBorderColor(CStudioPreferences::GetButtonShadowColor())
    , m_BottomBorderColor(CStudioPreferences::GetButtonShadowColor())
{
}

//=============================================================================
/**
 * Destructor
 */
template <typename TButton>
CProceduralButton<TButton>::~CProceduralButton()
{
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::SetFillStyleUp(EFillStyle inStyle)
{
    if (m_UpFillStyle != inStyle) {
        m_UpFillStyle = inStyle;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::SetFillStyleDown(EFillStyle inStyle)
{
    if (m_DownFillStyle != inStyle) {
        m_DownFillStyle = inStyle;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::SetFillStyleDisabled(EFillStyle inStyle)
{
    if (m_DisabledFillStyle != inStyle) {
        m_DisabledFillStyle = inStyle;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::SetFillStyleOver(EFillStyle inStyle)
{
    if (m_OverFillStyle != inStyle) {
        m_OverFillStyle = inStyle;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::SetFillStyleAll(EFillStyle inStyle)
{
    SetFillStyleUp(inStyle);
    SetFillStyleDown(inStyle);
    SetFillStyleDisabled(inStyle);
    SetFillStyleOver(inStyle);
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::SetFillColorUp(const CColor &inColor)
{
    if (m_UpColor != inColor) {
        m_UpColor = inColor;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::SetFillColorDown(const CColor &inColor)
{
    if (m_DownColor != inColor) {
        m_DownColor = inColor;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::SetFillColorDisabled(const CColor &inColor)
{
    if (m_DisabledColor != inColor) {
        m_DisabledColor = inColor;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::SetFillColorOver(const CColor &inColor)
{
    if (m_OverColor != inColor) {
        m_OverColor = inColor;
        TButton::Invalidate();
    }
}

template <typename TButton>
void CProceduralButton<TButton>::SetBorderVisibilityAll(const SBorderOptions &inBorderOptions)
{
    SetBorderVisiblityUp(inBorderOptions);
    SetBorderVisiblityDown(inBorderOptions);
    SetBorderVisiblityDisabled(inBorderOptions);
    SetBorderVisiblityOver(inBorderOptions);
}

template <typename TButton>
void CProceduralButton<TButton>::SetBorderVisiblityUp(const SBorderOptions &inBorderOptions)
{
    m_UpBorders = inBorderOptions;
}

template <typename TButton>
void CProceduralButton<TButton>::SetBorderVisiblityDown(const SBorderOptions &inBorderOptions)
{
    m_DownBorders = inBorderOptions;
}

template <typename TButton>
void CProceduralButton<TButton>::SetBorderVisiblityDisabled(const SBorderOptions &inBorderOptions)
{
    m_DisabledBorders = inBorderOptions;
}

template <typename TButton>
void CProceduralButton<TButton>::SetBorderVisiblityOver(const SBorderOptions &inBorderOptions)
{
    m_OverBorders = inBorderOptions;
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::SetLeftBorderColor(const CColor &inColor)
{
    if (m_LeftBorderColor != inColor) {
        m_LeftBorderColor = inColor;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::SetTopBorderColor(const CColor &inColor)
{
    if (m_TopBorderColor != inColor) {
        m_TopBorderColor = inColor;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::SetRightBorderColor(const CColor &inColor)
{
    if (m_RightBorderColor != inColor) {
        m_RightBorderColor = inColor;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::SetBottomBorderColor(const CColor &inColor)
{
    if (m_BottomBorderColor != inColor) {
        m_BottomBorderColor = inColor;
        TButton::Invalidate();
    }
}

//=============================================================================
/**
 * xxx
 */
template <typename TButton>
void CProceduralButton<TButton>::Render(CRenderer *inRenderer)
{
    // Fill the background if desired
    DrawBackground(inRenderer);

    // Draw the button icon
    TButton::Render(inRenderer);

    // Draw the border
    DrawBorder(inRenderer);
}

//=============================================================================
/**
 * Fills the background of this button with the appropriate color and style.
 * The style is specified by calling "GetCurrentFillStyle" which may indicate
 * not to fill the background at all.
 * @param inRenderer the renderer to draw to
 */
template <typename TButton>
void CProceduralButton<TButton>::DrawBackground(CRenderer *inRenderer)
{
    CPt theSize = TButton::GetSize();

    // Fill in the background color if necessary
    EFillStyle theFillStyle = GetCurrentFillStyle();

    switch (theFillStyle) {
    // Perform a gradient fill (block of color that varies slightly along the y-axis)
    case EFILLSTYLE_GRADIENT: {
        // The gradient inverts if the button is currently in the down state
        bool theInvertGradientFlag = (TButton::GetButtonState() == TButton::EBUTTONSTATE_DOWN) ? true : false;
        inRenderer->DrawGradientBitmap(QRect(0, 0, theSize.x, theSize.y), GetCurrentBackgroundColor(), theInvertGradientFlag,
                                       0.75);
    } break;

    // Fill the whole area with a block of solid color
    case EFILLSTYLE_FLOOD:
        inRenderer->FillSolidRect(QRect(0, 0, theSize.x, theSize.y), GetCurrentBackgroundColor());
        break;

    // Do not fill the background at all
    case EFILLSTYLE_NONE:
    // No break
    default:
        // No filling in of the background
        break;
    }
}

//=============================================================================
/**
 * Draws the borders around this button as specified in the border options for
 * the current state of the button.  You can turn border drawing on and off
 * per side of the button, and you can also specify different colors for each
 * side of the button.
 * @param inRenderer renderer to draw to
 */
template <typename TButton>
void CProceduralButton<TButton>::DrawBorder(CRenderer *inRenderer)
{
    CPt theSize = TButton::GetSize();
    SBorderOptions theBorders = CProceduralButton::GetCurrentBorderOptions();

    if (theBorders.m_DrawLeft) {
        inRenderer->PushPen(m_LeftBorderColor);
        inRenderer->MoveTo(CPt(0, theSize.y - 1));
        inRenderer->LineTo(CPt(0, 0));
        inRenderer->PopPen();
    }

    if (theBorders.m_DrawTop) {
        inRenderer->PushPen(m_TopBorderColor);
        inRenderer->MoveTo(CPt(0, 0));
        inRenderer->LineTo(CPt(theSize.x - 1, 0));
        inRenderer->PopPen();
    }

    if (theBorders.m_DrawRight) {
        inRenderer->PushPen(m_RightBorderColor);
        if (!theBorders.m_DrawTop)
            inRenderer->MoveTo(CPt(theSize.x - 1, 0));
        else
            inRenderer->MoveTo(CPt(theSize.x - 1, 1));
        inRenderer->LineTo(CPt(theSize.x - 1, theSize.y - 1));
        inRenderer->PopPen();
    }

    if (theBorders.m_DrawBottom) {
        inRenderer->PushPen(m_BottomBorderColor);
        inRenderer->MoveTo(CPt(theSize.x - 1, theSize.y - 1));
        if (!theBorders.m_DrawLeft)
            inRenderer->LineTo(CPt(0, theSize.y - 1));
        else
            inRenderer->LineTo(CPt(1, theSize.y - 1));
        inRenderer->PopPen();
    }
}

//=============================================================================
/**
 * @return the current fill style based upon state (up, down, mouse over, disabled) of the button
 */
template <typename TButton>
typename CProceduralButton<TButton>::EFillStyle
CProceduralButton<TButton>::GetCurrentFillStyle()
{
    // Default to the up state
    EFillStyle theFillStyle = m_UpFillStyle;

    typename TButton::EButtonState theState = TButton::GetButtonState();

    // If the mouse is over the button, switch to the mouse over style
    if (TButton::IsMouseOver())
        theFillStyle = m_OverFillStyle;

    // If the button is currently pressed, this cancels the up and over states, so return the down
    // style
    if (theState == TButton::EBUTTONSTATE_DOWN)
        theFillStyle = m_DownFillStyle;

    // If this button is disabled just return the disabled style (overrides any other states)
    if (!TButton::IsEnabled())
        theFillStyle = m_DisabledFillStyle;

    return theFillStyle;
}

//=============================================================================
/**
 * @return the current background color based upon state (up, down, mouse over, disabled) of the
 * button
 */
template <typename TButton>
::CColor CProceduralButton<TButton>::GetCurrentBackgroundColor()
{
    // Default to the up color
    ::CColor theColor = m_UpColor;

    typename TButton::EButtonState theState = TButton::GetButtonState();

    // If the mouse is over the button, switch to the mouse over color
    if (TButton::IsMouseOver())
        theColor = m_OverColor;

    // If the button is currently pressed, this cancels the up and over states, so return the down
    // color
    if (theState == TButton::EBUTTONSTATE_DOWN)
        theColor = m_DownColor;

    // If this button is disabled just return the disabled color (overrides any other states)
    if (!TButton::IsEnabled())
        theColor = m_DisabledColor;

    return theColor;
}

//=============================================================================
/**
 * @return the current border drawing options based upon state (up, down, mouse over, disabled) of
 * the button
 */
template <typename TButton>
typename CProceduralButton<TButton>::SBorderOptions
CProceduralButton<TButton>::GetCurrentBorderOptions()
{
    SBorderOptions theOptions = m_UpBorders;
    typename TButton::EButtonState theState = TButton::GetButtonState();

    // If the mouse is over the button, switch to the mouse over border options
    if (TButton::IsMouseOver())
        theOptions = m_OverBorders;

    // If the button is currently pressed, this cancels the up and over states, so return the over
    // border options
    if (theState == TButton::EBUTTONSTATE_DOWN)
        theOptions = m_DownBorders;

    // If this button is disabled just return the disabled border options (overrides any other
    // states)
    if (!TButton::IsEnabled())
        theOptions = m_DisabledBorders;

    return theOptions;
}

#endif // INCLUDED_PROCEDURAL_BUTTON_H
