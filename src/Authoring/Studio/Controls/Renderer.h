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

#ifndef INCLUDED_RENDERER_H
#define INCLUDED_RENDERER_H 1

#pragma once

#include <QColor>
#include <QPoint>

#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE
class QPainter;
class QPixmap;
class QRect;
class QSize;
class QString;
QT_END_NAMESPACE

class CRenderer
{
    typedef std::vector<QPoint> TTranslationList;

public:
    virtual ~CRenderer() {}

    virtual QPainter* GetPainter() = 0;
    virtual void FillSolidRect(const QRect &inCoordinates, const QColor &inColor) = 0;
    virtual void FillRoundedRect(const QRect &inCoordinates, const QColor &inColor,
                                 bool vertical) = 0;
    virtual void MoveTo(const QPoint &inPoint) = 0;
    virtual void MoveTo(long inX, long inY) = 0;
    virtual void LineTo(const QPoint &inPoint) = 0;
    virtual void LineTo(long inX, long inY) = 0;

    virtual void PushPen(const QColor &inColor,
                         int inWidth = 1) = 0; //, UINT inStyle = PS_SOLID );
    virtual void PopPen() = 0;
    virtual void BitBltFrom(const QRect &inRect, CRenderer *inRenderer, long inXSrc, long inYSrc) = 0;

    virtual void DrawText(float inX, float inY, const QString &inText) = 0;
    virtual void DrawText(float inX, float inY, const QString &inText,
                          const QRect &inBoundingRect, const QColor &inColor = Qt::black) = 0;
    virtual void DrawBoldText(float inX, float inY, const QString &inText,
                              const QRect &inBoundingRect, const QColor &inColor = Qt::black) = 0;

    virtual QSize GetTextSize(const QString &inText) = 0;

    virtual void DrawBitmap(const QPoint &inPos, const QPixmap &inImage) = 0;
    virtual void Draw3dRect(const QRect &inRect, const QColor &inTopLeftColor,
                            const QColor &inBottomRightColor) = 0;
    virtual void DrawRectOutline(const QRect &inRect, const QColor &inTop, const QColor &inRight,
                                 const QColor &inBottom, const QColor &inLeft);
    virtual void DrawGradientBitmap(const QRect &inRct, const QColor &inBeginColor, bool inInverted,
                                    double inScalingFactor = .99) = 0;

    virtual void DrawGradient(const QRect &inRect, const QColor &inBeginColor,
                              const QColor &inEndColor) = 0;

    virtual void PushTranslation(const QPoint &inTranslation) = 0;
    virtual void PopTranslation() = 0;
    virtual QPoint GetTranslation() = 0;

    virtual QRect GetClippingRect() = 0;
    virtual void PushClippingRect(const QRect &inRect) = 0;
    virtual void PushAbsoluteClippingRect(const QRect &inRect) = 0;
    virtual void PopClippingRect() = 0;

    virtual void FillHashed(const QRect &inRect, const QColor &inForeGroundColor) = 0;

    virtual QPixmap pixmap() const = 0;

protected:
    QPoint m_Translation;
    TTranslationList m_Translations;
};
#endif // INCLUDED_RENDERER_H
