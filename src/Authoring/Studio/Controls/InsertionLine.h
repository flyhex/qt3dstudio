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
#ifndef INCLUDED_INSERTION_LINE_H
#define INCLUDED_INSERTION_LINE_H 1

#pragma once

//==============================================================================
// Includes
//==============================================================================
#include "OverlayControl.h"
#include "CColor.h"

#include <QPixmap>
//==============================================================================
// Forwards
//==============================================================================
class CRenderer;

//=============================================================================
/**
 * Class for drawing a horizontal insertion line on top of other controls.
 * Useful for indicating drag-and-drop locations. Example usage: Item is
 * being inserted between Item1 and Item2.
 *		Item1
 *		>----<
 *		Item2
 */
class CInsertionLine : public COverlayControl
{
public:
    CInsertionLine();
    virtual ~CInsertionLine();
    virtual void SetPosition(CPt inPoint);
    void SetLineWidth(long inWidth);
    virtual void Draw(CRenderer *inRenderer);
    virtual bool HitTest(const CPt &inPoint) const;

protected:
    long m_LineWidth;
    long m_LineHeight;
    CColor m_LineColor;
    QPixmap m_InsertLeftImage;
    QPixmap m_InsertRightImage;
};
#endif // INCLUDED_INSERTION_LINE_H
