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
#include "InsertionLine.h"
#include "Renderer.h"
#include "ResourceCache.h"
#include "StudioUtils.h"
#include "CColor.h"
#include "CoreUtils.h"

//=============================================================================
/**
 * Constructor
 */
CInsertionLine::CInsertionLine()
    : m_LineWidth(0)
    , m_LineHeight(2)
    , m_LineColor(CColor(0, 0, 0))
{
    SetVisible(false);

    m_InsertLeftImage = CResourceCache::GetInstance()->GetBitmap("Insert-Rearrange-Left.png");
    m_InsertRightImage = CResourceCache::GetInstance()->GetBitmap("Insert-Rearrange-Right.png");

    const auto theLeftSize = m_InsertLeftImage.size();
    const auto theRightSize = m_InsertRightImage.size();

    if (theLeftSize.height() >= theRightSize.width()) {
        SetSize(CPt(theLeftSize.width() + theRightSize.width(), theLeftSize.height()));
        SetMaximumSize(CPt(LONG_MAX, theLeftSize.height()));
    } else {
        SetSize(CPt(theLeftSize.width() + theRightSize.width(), theRightSize.height()));
        SetMaximumSize(CPt(LONG_MAX, theLeftSize.height()));
    }
}

//=============================================================================
/**
 * Destructor
 */
CInsertionLine::~CInsertionLine()
{
}

//=============================================================================
/**
 * Sets the position of the line.  Overridden since the line is technically
 * drawn in the middle of the area specified as the size of this control, so
 * we have to adjust the point accordingly.  If you specify that the line should
 * be drawn at inPoint = ( 100, 100 ), the line will be drawn there, but the
 * actual control will be positioned at y = 100 - m_LineHeight.
 * @param inPoint starting point of the line to be drawn
 */
void CInsertionLine::SetPosition(CPt inPoint)
{
    inPoint.y -= m_LineHeight;
    COverlayControl::SetPosition(inPoint);
}

//=============================================================================
/**
 * Sets the width of the horizontal line.  The arrows on the ends will
 * automatically overlap this line.  Thus the control will be exactly as wide
 * as you specify here.  You should call this method instead of SetSize.
 * @param inWidth The new width of the line
 */
void CInsertionLine::SetLineWidth(long inWidth)
{
    m_LineWidth = inWidth;

    SetSize(CPt(m_LineWidth, GetSize().y));
}

//=============================================================================
/**
 * Draws the insertion line at the current position.
 * @param inRenderer Renderer to draw to
 */
void CInsertionLine::Draw(CRenderer *inRenderer)
{
    // Actual insertion line
    CPt theTotalSize = GetSize();
    inRenderer->FillSolidRect(
        CRct(CPt(0, ::dtol(theTotalSize.y / 2.0f) - ::dtol(m_LineHeight / 2.0f)),
             CPt(m_LineWidth, m_LineHeight)),
        m_LineColor);

    // Left arrow (draws on top of the left side of the line)
    inRenderer->DrawBitmap(QPoint(0, 0), m_InsertLeftImage);

    // Right arrow (draws on top of the right side of the line)
    const auto theRightSize = m_InsertRightImage.size();
    inRenderer->DrawBitmap(QPoint(theTotalSize.x - theRightSize.width(), 0), m_InsertRightImage);
}

//=============================================================================
/**
 * Check to see if inPoint is over this control or not.  Overridden so that
 * this overlay control does not interfere with drag-and-drop.
 * @param inPoint not used
 * @return false
 */
bool CInsertionLine::HitTest(const CPt &inPoint) const
{
    Q_UNUSED(inPoint);

    return false;
}
