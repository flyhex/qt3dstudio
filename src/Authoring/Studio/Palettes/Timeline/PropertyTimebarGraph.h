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

#ifndef INCLUDED_PROPERTY_TIMEBAR_GRAPH_H
#define INCLUDED_PROPERTY_TIMEBAR_GRAPH_H 1

#pragma once

#include "Control.h"
#include "CColor.h"

class CPropertyGraphKeyframe;
class ITimelineItemProperty;

class CPropertyTimebarGraph : public CControl
{
    typedef std::vector<CPropertyGraphKeyframe *> TKeyframeList;

public:
    CPropertyTimebarGraph(ITimelineItemProperty *inProperty);
    virtual ~CPropertyTimebarGraph();
    void SetVisible(bool inIsVisible) override;
    void Draw(CRenderer *inRenderer) override;
    void SetTimeRatio(double inTimeRatio);
    void Invalidate(bool inIsInvalidated = true) override;
    void RefreshKeyframes();

protected:
    void DrawDetailedChannel(CRenderer *inRenderer, long inChannelIndex, ::CColor inColor);
    void AddKeyframes();
    void RemoveKeyframes();

    CPropertyGraphKeyframe *GetKeyframe(long inTime, long inChannelIndex);

protected:
    ITimelineItemProperty *m_Property;
    double m_TimeRatio;
    TKeyframeList m_Keyframes;
    float m_MaxVal;
    float m_MinVal;
    long m_MinY;
    long m_MaxY;
};
#endif // INCLUDED_PROPERTY_TIMEBAR_GRAPH_H
