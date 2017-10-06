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

#ifndef INCLUDED_PROPERTY_GRAPH_KEYFRAME_H
#define INCLUDED_PROPERTY_GRAPH_KEYFRAME_H 1

#pragma once

#include "Control.h"

class ITimelineItemProperty;

class CPropertyGraphKeyframe : public CControl
{
public:
    CPropertyGraphKeyframe(ITimelineItemProperty *inProperty, long inChannelIndex,
                           long inKeyframeTime, double inTimeRatio, long inHeight, long inOffset,
                           float inMinVal, float inMaxVal);
    virtual ~CPropertyGraphKeyframe();

    void Draw(CRenderer *inRenderer) override;

    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;

    long GetTime();
    long GetChannelIndex() const;
    void SetTimeRatio(double inTimeRatio);
    void PositionKeyframe();

protected:
    float GetKeyframeValue();
    void SetKeyframeValue(float inValue);

    ITimelineItemProperty *m_Property;
    long m_ChannelIndex; // index that identifies the channel for a animated property
    long m_KeyframeTime;
    double m_TimeRatio;
    float m_MinVal;
    float m_MaxVal;
    long m_MinY;
    long m_MaxY;
    CPt m_MouseDownLoc;
    bool m_IsMouseDown;
};
#endif // INCLUDED_PROPERTY_GRAPH_KEYFRAME_H
