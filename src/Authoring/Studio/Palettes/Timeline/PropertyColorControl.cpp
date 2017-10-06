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

#include "stdafx.h"

#include "PropertyColorControl.h"
#include "Renderer.h"
#include "PropertyRow.h"
#include "StudioPreferences.h"
#include "HotKeys.h"

CPropertyColorControl::CPropertyColorControl(CPropertyRow *inPropertyRow)
{
    m_ParentRow = inPropertyRow;
}

CPropertyColorControl::~CPropertyColorControl()
{
}

//==============================================================================
/**
 *	Draw
 *
 *  draws this object
 *
 *	@param	inRenderer	 a renderer object
 */
void CPropertyColorControl::Draw(CRenderer *inRenderer)
{
    // Fill the control with the solid color
    CRct theRect(GetSize());
    CColor theBgColor(CStudioPreferences::GetPropertyBackgroundColor());
    inRenderer->FillSolidRect(theRect, theBgColor);

    // Define the colors for the side and bottom outline
    CColor theBorderColor(CStudioPreferences::GetPropertyFloorColor());

    // Right
    inRenderer->PushPen(theBorderColor);
    inRenderer->MoveTo(CPt(theRect.size.x - 1, 0));
    inRenderer->LineTo(CPt(theRect.size.x - 1, theRect.size.y - 1));

    // Left
    inRenderer->MoveTo(CPt(0, 0));
    inRenderer->LineTo(CPt(0, theRect.size.y - 1));

    // Bottom
    inRenderer->MoveTo(CPt(1, theRect.size.y - 1));
    inRenderer->LineTo(CPt(theRect.size.x - 2, theRect.size.y - 1));
    inRenderer->PopPen();
}

//==============================================================================
/**
 *	Handles the OnMouseDownEvent
 */
bool CPropertyColorControl::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inPoint);
    m_ParentRow->Select((CHotKeys::MODIFIER_SHIFT & inFlags) == CHotKeys::MODIFIER_SHIFT);
    return true;
}
