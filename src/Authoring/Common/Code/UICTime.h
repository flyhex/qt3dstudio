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

#pragma once
#ifndef __UICTIME_H__
#define __UICTIME_H__

#include "UICLargeInteger.h"

namespace Q3DStudio {
const long msecsmax = 86400000; // number of milliseconds in one day
const long daysmax = 3652059; // number of days between 01/01/0001 and 12/31/9999
const TLarge datetimemax = TLarge(msecsmax) * daysmax; // max. allowed number for datetime type
const TLarge invdatetime = TLarge(-1);

class CTime
{
protected:
    TLarge m_Time; // Datetime type: 64-bit, number of milliseconds since midnight 01/01/0001.

public:
    // Utilities
    void SetNow(bool inUseUTC = true);
    long StopWatch();

    long GetMilliSecs() const { return static_cast<long>(m_Time % msecsmax); }
    long GetDays() const { return static_cast<long>(m_Time / msecsmax); }
    long DayOfWeek() const { return (GetDays() + 1) % 7; }

    void Encode(long inYear, long inMonth, long inDay, long inHour, long inMin, long inSec,
                long inMSec);
    void DecodeTime(long &outHour, long &outMin, long &outSec, long &inMSec);
    void DecodeDate(long &outYear, long &outMonth, long &outDay);

    // Construction
    CTime(bool inSetNow = false)
        : m_Time(0)
    {
        if (inSetNow)
            SetNow();
    }
    CTime(const CTime &inTime)
        : m_Time(inTime.m_Time)
    {
    }

    // Statics
    static CTime Now() { return CTime(true); }
    static bool IsLeapYear(long inYear);
    static long DaysInMonth(long inYear, long inMonth);
    static long DaysInYear(long inYear, long inMonth);
    static bool IsDateValid(long inYear, long inMonth, long inDay);
    static bool IsTimeValid(long inHour, long inMin, long inSec, long inMSec = 0);

    // Operators
    CTime &operator=(const CTime &inTime)
    {
        m_Time = inTime.m_Time;
        return *this;
    }
    CTime &operator+=(const CTime &inTime)
    {
        m_Time += inTime.m_Time;
        return *this;
    }
    CTime &operator-=(const CTime &inTime)
    {
        m_Time -= inTime.m_Time;
        return *this;
    }
    CTime &operator/=(const CTime &inTime)
    {
        m_Time /= inTime.m_Time;
        return *this;
    }
    CTime &operator*=(const CTime &inTime)
    {
        m_Time *= inTime.m_Time;
        return *this;
    }
    CTime &operator%=(const CTime &inTime)
    {
        m_Time %= inTime.m_Time;
        return *this;
    }

    CTime operator+(const CTime &inTime) const
    {
        CTime theTime(*this);
        return theTime += inTime;
    }
    CTime operator-(const CTime &inTime) const
    {
        CTime theTime(*this);
        return theTime -= inTime;
    }
    CTime operator*(const CTime &inTime) const
    {
        CTime theTime(*this);
        return theTime *= inTime;
    }
    CTime operator/(const CTime &inTime) const
    {
        CTime theTime(*this);
        return theTime /= inTime;
    }
    CTime operator%(const CTime &inTime) const
    {
        CTime theTime(*this);
        return theTime %= inTime;
    }

    bool operator==(const CTime &inTime) const { return m_Time == inTime.m_Time; }
    bool operator!=(const CTime &inTime) const { return m_Time != inTime.m_Time; }
    bool operator>(const CTime &inTime) const { return m_Time > inTime.m_Time; }
    bool operator<(const CTime &inTime) const { return m_Time < inTime.m_Time; }
    bool operator>=(const CTime &inTime) const { return m_Time >= inTime.m_Time; }
    bool operator<=(const CTime &inTime) const { return m_Time <= inTime.m_Time; }
    operator const long() const { return GetMilliSecs(); }

protected:
    bool IsValid(TLarge inTime);
    TLarge GetNow(bool inUseUTC = true);
    TLarge Convert(long inYear, long inMonth, long inDay, long inHour, long inMin, long inSec,
                   long inMSec);
    TLarge GetTimeZoneOffset();
};
}

#endif // __UICTIME_H__
