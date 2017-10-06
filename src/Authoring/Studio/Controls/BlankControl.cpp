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
#include "BlankControl.h"
#include "Renderer.h"
#include "SystemPreferences.h"

//=============================================================================
/**
 * constructor
 */
CBlankControl::CBlankControl(CColor inColor)
    : m_DrawBorder(false)
    , m_FillBackground(true)
{
    SetColor(inColor);
}

//=============================================================================
/**
 * destructor
 */
CBlankControl::~CBlankControl()
{
}

//=============================================================================
/**
 * Draws this control
 */
void CBlankControl::Draw(CRenderer *inRenderer)
{
    CRct theRect(GetSize());

    if (m_FillBackground)
        inRenderer->FillSolidRect(theRect, m_Color);

    if (m_DrawBorder) {
        // Get the normal colors for the 3D border
        CColor theShadowColor = CStudioPreferences::GetButtonShadowColor();
        CColor theHiliteColor = CStudioPreferences::GetButtonHighlightColor();

        // Draw a frame around the button
        inRenderer->Draw3dRect(
            CRct(theRect.position.x + 1, theRect.position.y, theRect.size.x, theRect.size.y),
            theHiliteColor, theShadowColor);
        inRenderer->PushPen(theShadowColor);
        inRenderer->MoveTo(CPt(0, 0));
        inRenderer->LineTo(CPt(0, theRect.size.y));
        inRenderer->PopPen();
    }
}

//=============================================================================
/**
 * Sets the color for this control
 */
void CBlankControl::SetColor(CColor inColor)
{
    if (m_Color == inColor)
        return;

    m_Color = inColor;

    Invalidate();
}

//=============================================================================
/**
 * @param inDrawBorder true if we should draw the border
 */
void CBlankControl::SetDrawBorder(bool inDrawBorder)
{
    m_DrawBorder = inDrawBorder;
}

//=============================================================================
/**
 * @param inFillBackground true if we should draw the background
 */
void CBlankControl::SetFillBackground(bool inFillBackground)
{
    m_FillBackground = inFillBackground;
}
