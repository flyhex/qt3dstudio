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

#ifndef INCLUDED_BASE_MEASURE_H
#define INCLUDED_BASE_MEASURE_H 1

#pragma once

#include "Control.h"
#include "CColor.h"
#include "CoreUtils.h"
#include <vector>

class CBaseMarker;
class CSnapper;

class CBaseMeasure : public CControl
{
public:
    CBaseMeasure(double inRatio, bool inFillBackground = true);
    virtual ~CBaseMeasure();

    virtual void OnDraw(CRenderer *inRenderer, CRct &inDirtyRect, bool inIgnoreValidation = false);

    virtual void Draw(CRenderer *inRenderer);

    // inline these
    long GetLargeHashInterval() const { return ::dtol(m_LargeHashInterval); }
    long GetMediumHashInterval() const { return ::dtol(m_MediumHashInterval); }
    long GetSmallHashInterval() const { return ::dtol(m_SmallHashInterval); }

protected:
    virtual void DrawMeasureText(CRenderer *inRenderer, long inPosition, long inMeasure) = 0;
    virtual long CalculatePos(double inNewValue) = 0;
    virtual long GetDisplayLength();
    virtual long GetDisplayHeight();
    virtual void DrawLine(CRenderer *inRenderer, long inPos, long inStart, long inEnd);
    virtual void DrawOutline(CRenderer *inRenderer, long inPos, long inStart, long inEnd);

    double m_SmallHashInterval; ///< the measurement represented by a small hash
    double m_MediumHashInterval; ///< the measurement represented by a medium hash
    double m_LargeHashInterval; ///< the measurement represented by a large hash
    double m_Ratio; ///< Ratio that is used to calculate the size of the large hash
    long m_Offset; ///< Offset of the point 0 from left or top
    ::CColor m_BackgroundColor; ///< Background of this control

    // Tickmarks
    long m_EdgeMargin; ///< Margin from the edge, so that no line draws all the way through
    long m_LargeHashOffset; ///< Offset from the margin for the large hash
    long m_MediumHashOffset; ///< Offset from the margin for the medium hash
    long m_SmallHashOffset; ///< Offset from the margin for the small hash

    bool m_FillBackground; ///< true to fill the background (i.e. its not transparent)
};
#endif // INCLUDED_BASE_MEASURE_H
