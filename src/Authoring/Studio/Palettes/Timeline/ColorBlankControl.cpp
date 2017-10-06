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
#include "ColorBlankControl.h"
#include "Renderer.h"
#include "StudioPreferences.h"

//=============================================================================
/**
 * Constructor
 */
CColorBlankControl::CColorBlankControl(CColor inColor)
    : CBlankControl(inColor)
{
}

//=============================================================================
/**
 * Destructor
 */
CColorBlankControl::~CColorBlankControl()
{
}

//=============================================================================
/**
 * Handles custom drawing of the blank control underneath the tree control
 * on the timeline palette.
 */
void CColorBlankControl::Draw(CRenderer *inRenderer)
{
    CBlankControl::Draw(inRenderer);

    // Draw the line on the right side of this control
    CPt theSize = GetSize();
    inRenderer->PushPen(CStudioPreferences::GetPropertyFloorColor());
    inRenderer->MoveTo(theSize.x - 1, 0);
    inRenderer->LineTo(theSize.x - 1, theSize.y - 1);
    inRenderer->PopPen();

    // Draw the line on the left side of this control
    inRenderer->PushPen(CStudioPreferences::GetRowTopColor());
    inRenderer->MoveTo(0, 0);
    inRenderer->LineTo(0, theSize.y - 1);
    inRenderer->PopPen();

    // Draw the highlight on the left side of this control
    inRenderer->PushPen(CStudioPreferences::GetButtonHighlightColor());
    inRenderer->MoveTo(CPt(1, 0));
    inRenderer->LineTo(CPt(1, theSize.y - 1));
    inRenderer->PopPen();
}
