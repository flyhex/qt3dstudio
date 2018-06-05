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

#ifndef INCLUDED_WIN_RENDERER_H
#define INCLUDED_WIN_RENDERER_H 1

#pragma once

#include <vector>
#include <map>

#include "Pt.h"
#include "Rct.h"
#include "Renderer.h"

#include <QPainter>
#include <QRegion>

QT_BEGIN_NAMESPACE
class QPixmap;
QT_END_NAMESPACE

class CWinRenderer : public CRenderer
{
    typedef std::vector<QPen> TPenList;
    typedef std::vector<QRegion> TClippingRegions;

protected:
    CWinRenderer(); ///< Leaves CRenderer in an invalid state, only for subclasses
    CWinRenderer(QPainter *inDC); ///< Leaves CRenderer in an invalid state, only for subclasses

public:
    CWinRenderer(QPainter *inDC, const QRect &inClippingRect);
    virtual ~CWinRenderer();

    void FillSolidRect(const QRect &inCoordinates, const QColor &inColor) override;
    void FillRoundedRect(const QRect &inCoordinates, const QColor &inColor,
                         bool vertical) override;

    void MoveTo(const QPoint &inPoint) override;
    void MoveTo(long inX, long inY) override;
    void LineTo(const QPoint &inPoint) override;
    void LineTo(long inX, long inY) override;

    void PushPen(const QColor &inColor, int inWidth = 1) override;
    void PopPen() override;
    void BitBltFrom(const QRect &inRect, CRenderer *inRenderer, long inXSrc, long inYSrc) override;

    void DrawText(float inX, float inY, const QString &inText) override;
    void DrawText(float inX, float inY, const QString &inText,
                          const QRect &inBoundingRect, const QColor &inColor = Qt::black) override;
    void DrawBoldText(float inX, float inY, const QString &inText,
                              const QRect &inBoundingRect, const QColor &inColor = Qt::black) override;

    QSize GetTextSize(const QString &inText) override;

    void DrawBitmap(const QPoint &inPos, const QPixmap &inImage) override;
    void Draw3dRect(const QRect &inRect, const QColor &inTopLeftColor,
                    const QColor &inBottomRightColor) override;
    void DrawGradientBitmap(const QRect &inRct, const QColor &inBeginColor, bool inInverted,
                                    double inScalingFactor = .99) override;

    void DrawGradient(const QRect &inRect, const QColor &inBeginColor, const QColor &inEndColor) override;

    void PushTranslation(const QPoint &inTranslation) override;
    void PopTranslation() override;
    QPoint GetTranslation() override;

    QRect GetClippingRect() override;
    void PushClippingRect(const QRect &inRect) override;
    void PopClippingRect() override;
    void PushAbsoluteClippingRect(const QRect &) override {}
    void FillHashed(const QRect &inRect, const QColor &inForeGroundColor) override;
    QPainter *GetPainter() override;
    QPen GetPen(const QColor &inColor, int inWidth, Qt::PenStyle inStyle);

    QPixmap pixmap() const override;

protected:
    TPenList m_PenList;
    TClippingRegions m_ClippingRegions;
    QPainter *m_painter;
    QPoint m_currentPos;

protected:
    struct SPenInfo
    {
        QColor Color;
        long Width;
        Qt::PenStyle Style;
    };

    class CPenInfoLessThan : public std::binary_function<const SPenInfo &, const SPenInfo &, bool>
    {
    public:
        inline bool operator()(const SPenInfo &inValL, const SPenInfo &inValR) const
        {
            return memcmp(&inValL, &inValR, sizeof(SPenInfo)) < 0;
        }
    };

    typedef std::map<SPenInfo, QPen, CPenInfoLessThan> TPenMap;
    TPenMap m_Pens;
};
#endif // INCLUDED_WIN_RENDERER_H
