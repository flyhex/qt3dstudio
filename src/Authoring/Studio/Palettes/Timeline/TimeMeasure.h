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

#ifndef INCLUDED_TIME_MEASURE_H
#define INCLUDED_TIME_MEASURE_H 1

#pragma once

#include "BaseMeasure.h"
#include "ITickTock.h"

class CTimelineTimelineLayout;
class CSnapper;

class CTimeMeasure : public CBaseMeasure
{
public:
    CTimeMeasure(CTimelineTimelineLayout *inLayout, double inTimeRatio,
                 bool inFillBackground = true);
    virtual ~CTimeMeasure();

    void SetTimeRatio(double inTimeRatio);
    void SetTimeOffset(long inTimeOffset);

    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseRUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;

    void PopulateSnappingList(CSnapper *inSnapper);

    virtual void OnTimer();
    void OnLoseFocus() override;

protected:
    // CBaseMeasure
    void DrawMeasureText(CRenderer *inRenderer, long inPosition, long inMeasure) override;
    long CalculatePos(double inNewValue) override;

    Q3DStudio::CString FormatTime(long inTime);

    long m_ScrollDir;
    double m_TimePerPixel;
    bool m_IsMouseDown;
    CTimelineTimelineLayout *m_TimelineLayout;
    std::shared_ptr<qt3dsdm::ISignalConnection> m_TimerConnection;
};
#endif // INCLUDED_TIME_MEASURE_H
