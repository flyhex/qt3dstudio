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
#include "UICTime.h"

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#elif defined(__MACH__)
#include <sys/time.h>
#endif

namespace Q3DStudio {

//====================================================================
/**
 * Sets this time object to reflect the current time.
 * @param inUseUTC is true if we should use Coordinated Universal Time
 *			instead of local time zone.
 */
void CTime::SetNow(bool inUseUTC)
{
    m_Time = GetNow(inUseUTC);
}

//====================================================================
/**
 * Get the number of miliseconds since year 0.
 * @param inUseUTC is true if we should use Coordinated Universal Time instead of local time zone.
 * @return the number of miliseconds
 */
TLarge CTime::GetNow(bool inUseUTC)
{
#ifdef WIN32 // Windows

    SYSTEMTIME t;

    if (inUseUTC)
        ::GetSystemTime(&t);
    else
        ::GetLocalTime(&t);

    return Convert(t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

#elif defined(__MACH__) // MACH-O
    // we can't use localtime() and gmtime() here as they don't return
    // milliseconds which are needed for our datetime format. instead,
    // we call gettimeofday() which have microsecond precision, and then
    // adjust the time according to timzone info returned by localtime()
    // on BSD and Linux, and global variables altzone/timezone on SunOS.
    // MacOS X: localtime() is always reentrant (?)

    // NOTE: at the moment of passing the DST adjustment (twice a year)
    // the local time value returned by now() may be incorrect.
    // the application should call tzupdate() from time to time if it
    // is supposed to be running infinitely, e.g. if it's a daemon.

    // always rely on UTC time inside your application whenever possible.
    timeval tv;
    ::gettimeofday(&tv, NULL);

    long theDays = tv.tv_sec / 86400 // days since Unix "Epoch", i.e. 01/01/1970
        + 719162; // plus days between 01/01/0001 and Unix Epoch
    long theSecs = tv.tv_sec % 86400; // the remainder, i.e. seconds since midnight
    TLarge theMSecs =
        static_cast<TLarge>(theDays) * msecsmax + (theSecs * 1000 + tv.tv_usec / 1000);

    if (!inUseUTC) {
        theMSecs += GetTimeZoneOffset() * 60 * 1000;
    }
    return theMSecs;
#else
    return 0;
#endif
}

//====================================================================
/**
 * GetTimeZoneOffset.
 * @return the number of miliseconds
 */
TLarge CTime::GetTimeZoneOffset()
{
    return 0;
    /*
#if defined(WIN32)
    TIME_ZONE_INFORMATION tz;
    GetTimeZoneInformation(&tz);
    if (tz.DaylightDate.wMonth != 0)
            return - (tz.Bias +  tz.DaylightBias);
    else
            return - tz.Bias;

#elif defined(__sun__)
    if (timezone == 0)
            tzset();
    if (daylight != 0)
            return - altzone / 60;
    else
            return - timezone / 60;

#elif defined(__DARWIN__)
    time_t u;
    time(&u);
    tm* t = localtime(&u);
    return t->tm_gmtoff / 60;

#else
    time_t u;
    time(&u);
    tm t;
    localtime_r(&u, &t);
    return t.tm_gmtoff / 60;
#endif*/
}

//====================================================================
/**
 * Get the difference in miliseconds between now and the time stored.
 * @return the number of miliseconds
 */
long CTime::StopWatch()
{
    TLarge theDifference = GetNow() - m_Time;
    return static_cast<long>(theDifference);
}

//====================================================================
/**
 * Validate the given representation of time.
 * @param inTime is the time in question
 * @return true if valid
 */
bool CTime::IsValid(TLarge inTime)
{
    return inTime >= 0 && inTime < datetimemax;
}

//====================================================================
/**
 * Examine if the given year is a leap year.
 * @param inYear is the year in question
 * @return true if leap year
 */
bool CTime::IsLeapYear(long inYear)
{
    return inYear > 0 && inYear % 4 == 0 && (inYear % 100 != 0 || inYear % 400 == 0);
}

//====================================================================
/**
 * Query the number of days in a month.
 * @param inYear is the year in question
 * @param inMonth is the month in question
 * @return the days in the given month
 */
long CTime::DaysInMonth(long inYear, long inMonth)
{
    static const long sMonthDays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    long theDays = sMonthDays[inMonth - 1];

    if (inMonth < 1 || inMonth > 12)
        return 0;

    if (inMonth == 2 && IsLeapYear(inYear)) {
        ++theDays;
    }

    return theDays;
}

//====================================================================
/**
 * Query the number of days in a year up to and including a month.
 * @param inYear is the year in question
 * @param inMonth is the month in question
 * @return the days in the given year
 */
long CTime::DaysInYear(long inYear, long inMonth)
{
    static const long sTotalMonthDays[12] = { 31,  59,  90,  120, 151, 181,
                                              212, 243, 273, 304, 334, 365 };
    long theDays = sTotalMonthDays[inMonth - 1];

    if (inMonth < 1 || inMonth > 12)
        return 0;

    if (inMonth > 1 && IsLeapYear(inYear)) {
        ++theDays;
    }

    return theDays;
}

//====================================================================
/**
 * Validates the given date.
 * @param inYear is the year in question
 * @param inMonth is the month in question
 * @param inDay is the day in question
 * @return true if date is valid
 */
bool CTime::IsDateValid(long inYear, long inMonth, long inDay)
{
    return inYear >= 1 && inYear <= 9999 && inMonth >= 1 && inMonth <= 12 && inDay >= 1
        && inDay <= DaysInMonth(inYear, inMonth);
}

//====================================================================
/**
 * Validates the given time.
 * @param inHour is the hour in question
 * @param inMin is the minute in question
 * @param inSec is the second in question
 * @param inMSec is the millisecond in question
 * @return true if time is valid
 */
bool CTime::IsTimeValid(long inHour, long inMin, long inSec, long inMSec)
{
    return inHour >= 0 && inHour < 24 && inMin >= 0 && inMin < 60 && inSec >= 0 && inSec < 60
        && inMSec >= 0 && inMSec < 1000;
}

//====================================================================
/**
 * Set the obejct time to the given date and time.
 * @param inYear are the given years
 * @param inMonth are the given minutes
 * @param inDay are the given seconds
 * @param inHour are the given hours
 * @param inMin are the given minutes
 * @param inSec are the given seconds
 * @param inMSec are the given milliseconds
 */
void CTime::Encode(long inYear, long inMonth, long inDay, long inHour, long inMin, long inSec,
                   long inMSec)
{
    TLarge theTime = Convert(inYear, inMonth, inDay, inHour, inMin, inSec, inMSec);

    if (IsValid(theTime)) {
        m_Time = theTime;
    }
}

//====================================================================
/**
 * Convert the given date and time to milliseconds.
 * @param inYear are the given years
 * @param inMonth are the given minutes
 * @param inDay are the given seconds
 * @param inHour are the given hours
 * @param inMin are the given minutes
 * @param inSec are the given seconds
 * @param inMSec are the given milliseconds
 * @return the total number of milliseonds since year 0
 */
TLarge CTime::Convert(long inYear, long inMonth, long inDay, long inHour, long inMin, long inSec,
                      long inMSec)
{
    TLarge theTime = static_cast<TLarge>(inHour) * 3600000 + static_cast<TLarge>(inMin) * 60000
        + static_cast<TLarge>(inSec) * 1000 + inMSec;

    long theYear = inYear - 1;
    TLarge theDays = inDay // days in this month
        + DaysInYear(inYear, inMonth - 1) // plus days since the beginning of the year
        + theYear * 365 // plus "pure" days
        + theYear / 4 - theYear / 100 + theYear / 400 // plus leap year correction
        - 1; // ... minus one (guess why :)

    theTime += static_cast<TLarge>(theDays) * msecsmax;

    return theTime;
}

//====================================================================
/**
 * Decode the stored time into hours, minutes, seconds and milliseonds.
 * @param outHour are the total hours
 * @param outMin are the total minutes
 * @param outSec are the total seconds
 * @param outMSec are the total milliseconds
 */
void CTime::DecodeTime(long &outHour, long &outMin, long &outSec, long &outMSec)
{
    long theMSecs = GetMilliSecs();

    outHour = theMSecs / 3600000;
    theMSecs %= 3600000;
    outMin = theMSecs / 60000;
    theMSecs %= 60000;
    outSec = theMSecs / 1000;
    outMSec = theMSecs % 1000;
}

//====================================================================
/**
 * Decode the stored time into years, months and days.
 * @param outYear are the total years
 * @param outMonth are the total months
 * @param outDay are the total days
 */
void CTime::DecodeDate(long &outYear, long &outMonth, long &outDay)
{
    long theDays = GetDays();
    const long the1Year = 365; // number of days in 1 year
    const long the4Years = the1Year * 4 + 1; // ... in 4 year period
    const long the100Years = the4Years * 25 - 1; // ... in 100 year period
    const long the400Years = the100Years * 4 + 1; // ... in 400 year period

    outYear = (theDays / the400Years) * 400 + 1;
    theDays %= the400Years;

    long theCenturies = theDays / the100Years;
    theDays %= the100Years;
    if (4 == theCenturies) {
        --theCenturies;
        theDays += the100Years;
    }
    outYear += theCenturies * 100;

    outYear += (theDays / the4Years) * 4;
    theDays %= the4Years;

    long theYears = theDays / the1Year;
    theDays %= the1Year;
    if (4 == theYears) {
        --theYears;
        theDays += the1Year;
    }
    outYear += theYears;

    outMonth = theDays / 29; // approximate month no. (to avoid loops)
    if (theDays < DaysInYear(outYear, outMonth)) // month no. correction
    {
        --outMonth;
    }

    outDay = theDays - DaysInYear(outYear, outMonth) + 1;
    ++outMonth;
}
}
