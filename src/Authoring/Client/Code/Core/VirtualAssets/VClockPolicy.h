/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef INCLUDED_VCLOCKPOLICY
#define INCLUDED_VCLOCKPOLICY

//==============================================================================
//	Includes
//==============================================================================

class CVClockPolicy
{
public:
    enum EPolicy { POLICY_NORMAL = 1, POLICY_ROUNDTRIP = 2, POLICY_UNKNOWN = 3 };

private:
    long m_UpperBound;
    long m_LowerBound;

    bool m_Looping;
    long m_Type;

public:
    CVClockPolicy(const CVClockPolicy &inSource);
    CVClockPolicy();

    bool AdjustTime(long &ioVirtualTime, bool &outPonged);
    long ReverseTime(long inVirtualTime);

    void SetUpperBound(long inUpperBound) { m_UpperBound = inUpperBound; }
    long GetUpperBound() { return m_UpperBound; }
    void SetLowerBound(long inLowerBound) { m_LowerBound = inLowerBound; }
    long GetLowerBound() { return m_LowerBound; }
    void SetClockType(long inClockType) { m_Type = inClockType; }
    long GetClockType() { return m_Type; }
    void SetLooping(bool inLooping) { m_Looping = inLooping; }
    bool GetLooping() { return m_Looping; }
private:
    long HandleRoundTrip(long inVirtualTime, long inRange, bool &outLooped, bool &outPonged);
    long HandleNormal(long inVirtualTime, long inRange, bool &outLooped);
};

#endif // INCLUDED_VCLOCKPOLICY
