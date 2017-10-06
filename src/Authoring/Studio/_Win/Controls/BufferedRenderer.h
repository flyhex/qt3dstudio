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

#ifndef INCLUDED_BUFFERED_RENDERER_H
#define INCLUDED_BUFFERED_RENDERER_H 1

#pragma once

#include "WinRenderer.h"

#include <QPainter>
#include <QPixmap>
#include <QFont>

class CBufferedRenderer : public CWinRenderer
{
public:
    CBufferedRenderer(const QSize &inSize);

    virtual ~CBufferedRenderer();
    QPixmap pixmap() const override { return m_CurrentBitmap;}

    //	void TransparentBltTo( CRenderer* inRenderer, short inAlpha );

protected:
    QPixmap m_OldBitmap;
    QPixmap m_CurrentBitmap;
    QSize m_Size;

    //	bool AlphaBlendExt( HDC inDCDest, short inX, short inY, short inWidth, short inHeight, HDC
    //inDCSrc, short inSourceX, short inSourceY, short inSourceWidth, short inSourceHeight, short
    //inAlpha );
};
#endif // INCLUDED_BUFFERED_RENDERER_H
