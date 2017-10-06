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

#ifndef INCLUDED_SNAPPER_H
#define INCLUDED_SNAPPER_H 1

#pragma once

#include <set>
#include <vector>

class CSnapper
{
    typedef std::set<long> TTimeList;
    typedef std::vector<long> TOffsetList;

public:
    CSnapper(double inTimeRatio = .1);
    virtual ~CSnapper();

    void AddTime(long inTime);
    void AddPixelLocation(long inPixelLoc);

    void SetSnappingKeyframes(bool inIsSnappingKeyframes);
    bool IsSnappingKeyframes();
    void SetSnappingSelectedKeyframes(bool inIsSnappingSelectedKeyframes);
    bool IsSnappingSelectedKeyframes();

    void Clear();

    void SetVisibleArea(long inStartHeight, long inEndHeight);
    void PushOffset(long inHeight);
    void PopOffset();
    bool IsVisible(long inPosition, long inHeight);

    void SetTimeRatio(double inTimeRatio);

    void SetPeriodicInterval(long inInterval);

    bool InterpretTimeEx(long &ioTime, long inFlags);
    long InterpretTime(long inTime, long inFlags);

    void SetSnappingDistance(long inSnapDistance);

    void BeginDrag(long inPosition, long inOffset = 0);
    long ProcessDrag(long inTime, long inPosition, long inFlags);

    void SetSource(void *inSource);
    void *GetSource();

    void SetTimeOffset(long inTimeOffset);
    void SetStartEndTime(long theStartTime, long theEndTime);
    void SetKeyFrameClicked(bool inKeyFrameClicked);

protected:
    bool GetSnapTime(long &inTime, bool inShiftKeyDown);
    long GetClosestPeriodicInterval(long inTime);

    long m_theStartTime;
    long m_theEndTime;

    TTimeList m_Times;
    TOffsetList m_Offsets;

    double m_TimeRatio;

    bool m_IsSnappingKeyframes;
    bool m_IsSnappingSelectedKeyframes;
    long m_Offset;
    long m_StartHeight;
    long m_EndHeight;
    long m_SnapDistance;

    long m_PeriodicInterval;

    long m_InitialOffset;
    long m_ObjectTimeOffset;
    long m_TimeOffset;
    bool m_KeyFrameClicked;
    void *m_Source;
};

// Interface that will provider the info for snapping logic in the timebars and keyframes
class ISnappingListProvider
{
public:
    virtual ~ISnappingListProvider() {}

    virtual void PopulateSnappingList(CSnapper *inSnappingList) = 0;
};

#endif // INCLUDED_SNAPPER_H