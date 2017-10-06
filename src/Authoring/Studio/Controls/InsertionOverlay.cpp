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
// Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
// Includes
//==============================================================================
#include "InsertionOverlay.h"
#include "Renderer.h"
#include "ResourceCache.h"

//=============================================================================
/**
 * Constructor
 */
CInsertionOverlay::CInsertionOverlay()
    : m_Width(0)
{
    SetVisible(false);

    m_InsertLeftImage = CResourceCache::GetInstance()->GetBitmap("Insert-Left.png");
    m_InsertRightImage = CResourceCache::GetInstance()->GetBitmap("Insert-Right.png");

    const auto theLeftSize = m_InsertLeftImage.size();
    const auto theRightSize = m_InsertRightImage.size();

    if (theLeftSize.height() >= theRightSize.height())
        SetSize(CPt(theLeftSize.width() + theRightSize.width(), theLeftSize.height()));
    else
        SetSize(CPt(theLeftSize.width() + theRightSize.width(), theRightSize.height()));
}

//=============================================================================
/**
 * Destructor
 */
CInsertionOverlay::~CInsertionOverlay()
{
}

//=============================================================================
/**
 * Sets the width of the control.  The arrows are always drawn at the left
 * and right side of the control, as defined by this width.
 * @param inWidth total width of the control
 */
void CInsertionOverlay::SetWidth(long inWidth)
{
    m_Width = inWidth;

    SetSize(CPt(m_Width, GetSize().y));
}

//=============================================================================
/**
 * Draws the insertion markers.
 * Draws arrows at the left and right sides of the control, as defined by m_Width.
 * @param inRenderer Renderer to draw to
 */
void CInsertionOverlay::Draw(CRenderer *inRenderer)
{
    // Left arrow
    inRenderer->DrawBitmap(CPt(0, 0), m_InsertLeftImage);

    // Right arrow
    const auto theRightSize = m_InsertRightImage.size();
    inRenderer->DrawBitmap(QPoint(m_Width - theRightSize.width(), 0), m_InsertRightImage);
}

//=============================================================================
/**
 * Check to see if inPoint is over this control or not.  Overridden so that
 * this overlay control does not interfere with drag-and-drop.
 * @param inPoint not used
 * @return false
 */
bool CInsertionOverlay::HitTest(const CPt &inPoint) const
{
    Q_UNUSED(inPoint);

    return false;
}
