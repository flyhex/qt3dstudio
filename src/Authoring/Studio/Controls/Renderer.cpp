/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#include "Qt3DSCommonPrecompile.h"

#include "Renderer.h"

#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>

/**
 * Draws the outline of a rectangle in the specified colors.
 *
 * @param inRect Rectangle to be outlined
 * @param inTop Color of the top line
 * @param inRight Color of the line on the right side
 * @param inBottom Color of the line on the bottom
 * @param inLeft Color of the line on the left side
 */
void CRenderer::DrawRectOutline(const QRect &inRect, const QColor &inTop, const QColor &inRight,
                                const QColor &inBottom, const QColor &inLeft)
{
    QPoint theUpperLeft(inRect.topLeft());
    QPoint theUpperRight(inRect.topRight());
    QPoint theLowerRight(inRect.bottomRight());
    QPoint theLowerLeft(inRect.bottomLeft());

    // Top
    PushPen(inTop, 1);
    MoveTo(theUpperLeft.x(), theUpperLeft.y());
    LineTo(theUpperRight.x(), theUpperRight.y());
    PopPen();

    // Right
    PushPen(inRight, 1);
    LineTo(theLowerRight.x(), theLowerRight.y());
    PopPen();

    // Bottom
    PushPen(inBottom, 1);
    LineTo(theLowerLeft.x(), theLowerLeft.y());
    PopPen();

    // Left
    PushPen(inLeft, 1);
    LineTo(theUpperLeft.x(), theUpperLeft.y());
    PopPen();
}
