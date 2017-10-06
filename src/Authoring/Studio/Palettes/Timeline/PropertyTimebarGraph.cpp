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

#include "stdafx.h"

#include "PropertyTimebarGraph.h"
#include "IKeyframe.h"
#include "Renderer.h"
#include "PropertyGraphKeyframe.h"
#include "Bindings/ITimelineItemProperty.h"
#include "StudioUtils.h"

//=============================================================================
/**
 * Create a graph for the specified property.
 * @param inProperty the property this is graphing.
 */
CPropertyTimebarGraph::CPropertyTimebarGraph(ITimelineItemProperty *inProperty)
    : m_Property(inProperty)
    , m_TimeRatio(0.0f)
    , m_MaxVal(0.0f)
    , m_MinVal(0.0f)
    , m_MinY(0)
    , m_MaxY(0)
{
}

CPropertyTimebarGraph::~CPropertyTimebarGraph()
{
    TKeyframeList::iterator thePos = m_Keyframes.begin();
    for (; thePos != m_Keyframes.end(); ++thePos)
        delete (*thePos);
}

//=============================================================================
/**
 * Toggle whether this is visible or not.
 * Overrides CControl::SetVisible. If this is not visible then it removes
 * a bunch of the refresh logic to speed stuff up.
 * @param inIsVisible true if this control is to be visible.
 */
void CPropertyTimebarGraph::SetVisible(bool inIsVisible)
{
    CControl::SetVisible(inIsVisible);

    // If this is visible then add the appropriate keyframes and listeners.
    if (inIsVisible) {
        m_MaxVal = m_Property->GetMaximumValue();
        m_MinVal = m_Property->GetMinimumValue();

        m_MinY = 10;
        m_MaxY = GetSize().y - m_MinY;

        RefreshKeyframes();
    } else
        RemoveKeyframes();
}

//=============================================================================
/**
 * Set the time ratio for this display.
 * The time ratio controls how much time is displayed in how much space.
 * @param inTimeRatio the time ratio.
 */
void CPropertyTimebarGraph::SetTimeRatio(double inTimeRatio)
{
    m_TimeRatio = inTimeRatio;
}

//=============================================================================
/**
 * Draw this to the specified renderer.
 * This will perform the graphing of all the channels on the property.
 * @param inRenderer the renderer to draw to.
 */
void CPropertyTimebarGraph::Draw(CRenderer *inRenderer)
{
    CRct theRect(CPt(0, 0), GetSize());
    inRenderer->PushClippingRect(theRect);

    // the available line colors, tried to use Rainbow.bvs code to do it dynamically but didn't
    // quite work.
    ::CColor theColors[6] = { ::CColor(255, 0, 0),   ::CColor(0, 255, 0),   ::CColor(0, 0, 255),
                              ::CColor(255, 255, 0), ::CColor(255, 0, 255), ::CColor(0, 255, 255) };

    long theChannelCount = m_Property->GetChannelCount();
    // Don't want to overflow the color array
    if (theChannelCount <= 6) {
        // For each channel graph it.
        for (long theIndex = 0; theIndex < theChannelCount; ++theIndex)
            DrawDetailedChannel(inRenderer, theIndex, theColors[theIndex]);
    }

    inRenderer->PopClippingRect();
}

void CPropertyTimebarGraph::DrawDetailedChannel(CRenderer *inRenderer, long inChannelIndex,
                                                ::CColor inColor)
{
    inRenderer->PushPen(inColor, 2);

    CRct theClipRect = inRenderer->GetClippingRect();
    float theValue = m_Property->GetChannelValueAtTime(inChannelIndex, 0);
    long theYPos = (long)((1.0 - (theValue - m_MinVal) / (m_MaxVal - m_MinVal)) * (m_MaxY - m_MinY)
                          + m_MinY + .5);

    inRenderer->MoveTo(CPt(0, theYPos));

    long theInterval = 5;
    long theSize = theClipRect.position.x + theClipRect.size.x + theInterval;

    for (long thePixel = theClipRect.position.x; thePixel < theSize; thePixel += theInterval) {
        long theTime = ::PosToTime(thePixel, m_TimeRatio); //(long)( thePixel / m_TimeRatio + .5 );
        theValue = m_Property->GetChannelValueAtTime(inChannelIndex, theTime);
        theYPos = (long)((1.0 - (theValue - m_MinVal) / (m_MaxVal - m_MinVal)) * (m_MaxY - m_MinY)
                         + m_MinY + .5);

        inRenderer->LineTo(CPt(thePixel, theYPos));
    }

    inRenderer->PopPen();
}

void CPropertyTimebarGraph::AddKeyframes()
{
    long theChannelCount = m_Property->GetChannelCount();
    long theKeyframeCount =
        m_Property->GetKeyframeCount(); // the way it works now (and hence the assumption is), the
                                        // number of keyframes for all the channels is the same.
    for (long theIndex = 0; theIndex < theChannelCount; ++theIndex) {
        for (long theKeyIndex = 0; theKeyIndex < theKeyframeCount; ++theKeyIndex) {
            CPropertyGraphKeyframe *theGraphKeyframe = new CPropertyGraphKeyframe(
                m_Property, theIndex, m_Property->GetKeyframeByIndex(theKeyIndex)->GetTime(),
                m_TimeRatio, m_MinY, m_MaxY, m_MinVal, m_MaxVal);
            AddChild(theGraphKeyframe);
            m_Keyframes.push_back(theGraphKeyframe);
        }
    }
}

void CPropertyTimebarGraph::Invalidate(bool inIsInvalidated)
{
    CControl::Invalidate(inIsInvalidated);

    if (inIsInvalidated && GetParent() != nullptr) {
        GetParent()->Invalidate(inIsInvalidated);
    }
}

void CPropertyTimebarGraph::RemoveKeyframes()
{
    TKeyframeList::iterator thePos = m_Keyframes.begin();
    for (; thePos != m_Keyframes.end(); ++thePos) {
        CPropertyGraphKeyframe *theKeyframe = (*thePos);
        RemoveChild(theKeyframe);
        delete theKeyframe;
    }
    m_Keyframes.clear();
}

void CPropertyTimebarGraph::RefreshKeyframes()
{
    TKeyframeList::iterator thePos = m_Keyframes.begin();
    while (thePos != m_Keyframes.end()) {
        CPropertyGraphKeyframe *theKeyframe = (*thePos);
        if (m_Property->GetKeyframeByTime(theKeyframe->GetTime())) {
            theKeyframe->PositionKeyframe();
            ++thePos;
        } else {
            RemoveChild(theKeyframe);
            delete theKeyframe;
            thePos = m_Keyframes.erase(thePos);
        }
    }
    long theChannelCount = m_Property->GetChannelCount();
    long theKeyframeCount =
        m_Property->GetKeyframeCount(); // the way it works now (and hence the assumption is), the
                                        // number of keyframes for all the channels is the same.
    for (long theIndex = 0; theIndex < theChannelCount; ++theIndex) {
        for (long theKeyIndex = 0; theKeyIndex < theKeyframeCount; ++theKeyIndex) {
            IKeyframe *theKeyframe = m_Property->GetKeyframeByIndex(theKeyIndex);
            CPropertyGraphKeyframe *theExistingKeyframe =
                GetKeyframe(theKeyframe->GetTime(), theIndex);
            if (!theExistingKeyframe) {
                CPropertyGraphKeyframe *theGraphKeyframe =
                    new CPropertyGraphKeyframe(m_Property, theIndex, theKeyframe->GetTime(),
                                               m_TimeRatio, m_MinY, m_MaxY, m_MinVal, m_MaxVal);
                AddChild(theGraphKeyframe);

                m_Keyframes.push_back(theGraphKeyframe);
            }
        }
    }

    Invalidate();
}

CPropertyGraphKeyframe *CPropertyTimebarGraph::GetKeyframe(long inTime, long inChannelIndex)
{
    TKeyframeList::iterator thePos = m_Keyframes.begin();
    for (; thePos != m_Keyframes.end(); ++thePos) {
        CPropertyGraphKeyframe *theKeyframe = (*thePos);
        if (theKeyframe->GetChannelIndex() == inChannelIndex && theKeyframe->GetTime() == inTime) {
            return theKeyframe;
        }
    }
    return nullptr;
}
