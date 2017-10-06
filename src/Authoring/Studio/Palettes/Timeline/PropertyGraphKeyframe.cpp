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

#include "PropertyGraphKeyframe.h"
#include "Renderer.h"
#include "Bindings/ITimelineItemProperty.h"
#include "StudioUtils.h"
#include "CColor.h"

CPropertyGraphKeyframe::CPropertyGraphKeyframe(ITimelineItemProperty *inProperty,
                                               long inChannelIndex, long inKeyframeTime,
                                               double inTimeRatio, long inMinY, long inMaxY,
                                               float inMinVal, float inMaxVal)
    : m_Property(inProperty)
    , m_ChannelIndex(inChannelIndex)
    , m_KeyframeTime(inKeyframeTime)
    , m_TimeRatio(inTimeRatio)
    , m_MinVal(inMinVal)
    , m_MaxVal(inMaxVal)
    , m_MinY(inMinY)
    , m_MaxY(inMaxY)
    , m_IsMouseDown(false)
{
    SetSize(CPt(4, 4));
    PositionKeyframe();
}

CPropertyGraphKeyframe::~CPropertyGraphKeyframe()
{
}

long CPropertyGraphKeyframe::GetTime()
{
    return m_KeyframeTime;
}

long CPropertyGraphKeyframe::GetChannelIndex() const
{
    return m_ChannelIndex;
}

void CPropertyGraphKeyframe::PositionKeyframe()
{
    long theXPos = ::TimeToPos(m_KeyframeTime, m_TimeRatio);
    float theValue = GetKeyframeValue();
    long theYPos = (long)((1.0 - (theValue - m_MinVal) / (m_MaxVal - m_MinVal)) * (m_MaxY - m_MinY)
                          + m_MinY - GetSize().y / 2 + .5);

    if (theYPos < m_MinY)
        theYPos = m_MinY;
    else if (theYPos > m_MaxY)
        theYPos = m_MaxY;

    SetPosition(CPt(theXPos, theYPos));
}

void CPropertyGraphKeyframe::SetTimeRatio(double inTimeRatio)
{
    m_TimeRatio = inTimeRatio;
    PositionKeyframe();
}

float CPropertyGraphKeyframe::GetKeyframeValue()
{
    return m_Property->GetChannelValueAtTime(m_ChannelIndex, m_KeyframeTime);
}

void CPropertyGraphKeyframe::SetKeyframeValue(float inValue)
{
    m_Property->SetChannelValueAtTime(m_ChannelIndex, m_KeyframeTime, inValue);
}

void CPropertyGraphKeyframe::Draw(CRenderer *inRenderer)
{
    CPt mySize = GetSize();
    inRenderer->MoveTo(0, 0);
    inRenderer->LineTo(mySize.x, 0);
    inRenderer->LineTo(mySize.x, mySize.y);
    inRenderer->LineTo(0, mySize.y);
    inRenderer->LineTo(0, 0);

    if (m_IsMouseDown) {
        inRenderer->FillSolidRect(CRct(GetSize()), CColor::black);
    }
}

bool CPropertyGraphKeyframe::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (!CControl::OnMouseDown(inPoint, inFlags)) {
        m_IsMouseDown = true;
        m_MouseDownLoc = inPoint;
    }
    return true;
}

void CPropertyGraphKeyframe::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseUp(inPoint, inFlags);
    m_IsMouseDown = false;
    m_Property->CommitChangedKeyframes();
    GetParent()->Invalidate();
}

void CPropertyGraphKeyframe::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseMove(inPoint, inFlags);

    if (m_IsMouseDown) {
        long theDiff = inPoint.y - m_MouseDownLoc.y;
        long theYPos = GetPosition().y + theDiff;
        if (theYPos < m_MinY)
            theYPos = m_MinY;
        if (theYPos > m_MaxY)
            theYPos = m_MaxY;
        SetPosition(GetPosition().x, theYPos);

        float theValue = (m_MaxVal - m_MinVal)
                * (((m_MaxY + m_MinY) - theYPos - GetSize().y / 2) - m_MinY) / (m_MaxY - m_MinY)
            + m_MinVal;
        SetKeyframeValue(theValue);

        GetParent()->Invalidate();
    }
}
