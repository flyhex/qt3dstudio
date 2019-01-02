/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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

#include "Qt3DSCommonPrecompile.h"
#include "Qt3DSId.h"
#include "Qt3DSTime.h"

namespace Q3DStudio {

//====================================================================
/**
 * Constructor .
 * Initialize the GUID to Zero.
 */
CId::CId()
{
}

//====================================================================
/**
 * Constructor. The local Key will get initialized to the Value represented by the string.
 * @param inStringId is an ID in the form of a string.
 *
 */
CId::CId(wchar_t *inStringId)
{
    m_Key = QUuid(QString::fromWCharArray(inStringId));
}

//====================================================================
/**
 * Constructor.
 * @param inId is an ID to initialize from.
 */
CId::CId(const CId &inId)
{
    m_Key = inId.m_Key;
}

CId::CId(const GUID &inGUID)
{
#if defined(Q_OS_WIN)
    m_Key = QUuid(inGUID);
#else
    m_Key = QUuid(inGUID.Data1, inGUID.Data2, inGUID.Data3, inGUID.Data4[0], inGUID.Data4[1],
                  inGUID.Data4[2], inGUID.Data4[3], inGUID.Data4[4], inGUID.Data4[5],
                  inGUID.Data4[6], inGUID.Data4[7]);
#endif
}

CId::CId(long in1, long in2, long in3, long in4)
{
    m_Key = QUuid(in1, in2 >> 16, in2 & 0xffff,
                  (in3 >> 24) & 0xff, (in3 >> 16) & 0xff, (in3 >> 8) & 0xff, in3 & 0xff,
                  (in4 >> 24) & 0xff, (in4 >> 16) & 0xff, (in4 >> 8) & 0xff, in4 & 0xff);
}

//====================================================================
/**
 * Generate a new Key
 *
 */
void CId::Generate()
{
    // Generate unique UUID that works the same way on all platforms. We can't use QUuid as the
    // generated UUID on non-Windows platforms many times fills the packed Data4 with 0,
    // which is considered invalid by the rest of the application
    static long s_LastMilliSecs = 0;
    Q3DStudio::CTime theTime = Q3DStudio::CTime::Now( );
    long theCurrentMilliSecs = theTime.GetMilliSecs( );

    // Don't do this more than once in a 10ms period.
    if ( s_LastMilliSecs != theCurrentMilliSecs )
    {
        s_LastMilliSecs = theCurrentMilliSecs;
        ::srand( (unsigned)( theCurrentMilliSecs ) );
    }

    // ::rand is 16 bits, GUID is 128 bits, smear random bits over the whole GUID
    long Data1, Data2, Data3, Data4;
    Data1  = ::rand();
    Data1 <<= 16;
    Data1  |= ::rand();

    Data2  = ::rand();
    Data2 <<= 16;
    Data2  |= ::rand();

    Data3  = ::rand();
    Data3 <<= 16;
    Data3  |= ::rand();

    Data4  = ::rand();
    Data4 <<= 16;
    Data4  |= ::rand();
    *this = CId(Data1, Data2, Data3, Data4);
}

//====================================================================
/**
 * Convert this CId to a GUID.
 * @return a GUID struct.
 */
GUID CId::Convert() const
{
#if defined(Q_OS_WIN)
    return m_Key;
#else
    GUID guid;
    guid.Data1 = m_Key.data1;
    guid.Data2 = m_Key.data2;
    guid.Data3 = m_Key.data3;
    memcpy(guid.Data4, m_Key.data4, 8);
    return guid;
#endif
}

//====================================================================
/**
 * Compare two CIds.
 * @param inRVal is the Right side CId.
 * @return True if they were equal.
 */
bool CId::operator==(const CId &inRVal) const
{
    return m_Key == inRVal.m_Key;
}

//====================================================================
/**
 * Compare two CIds for not equal.
 * @param inRVal is the Right side CId.
 * @return True if they were equal.
 */
bool CId::operator!=(const CId &inRVal) const
{
    return !(*this == inRVal);
}

//====================================================================
/**
 * Compare if this CId is less than the Right Size CId.
 * @param inRVal is the Right side CId.
 * @return True if this CId is Less than.
 */
bool CId::operator<(const CId &inRVal) const
{
    return m_Key < inRVal.m_Key;
}

//====================================================================
/**
 * Set this Cid equal to the Right Side CId.
 * @param inRVal is the Right side CId.
 * @return this CId.
 */
CId &CId::operator=(const CId &inRVal)
{
    m_Key = inRVal.m_Key;

    return *this;
}
//====================================================================
/**
 * Convert this CId into a String.
 * @return The String version of this CId.
 */
QString CId::ToString() const
{
    return m_Key.toString();
}

CId::operator TGUIDPacked () const
{
    TGUIDPacked packed;
    packed.Data1 = m_Key.data1;
    packed.Data2 = long(m_Key.data2) << 16 | long(m_Key.data3);
    packed.Data3 = long(m_Key.data4[0]) << 24 | long(m_Key.data4[1]) << 16
                   | long(m_Key.data4[2]) << 8 | long(m_Key.data4[3]);
    packed.Data4 = long(m_Key.data4[4]) << 24 | long(m_Key.data4[5]) << 16
                   | long(m_Key.data4[6]) << 8 | long(m_Key.data4[7]);
    return packed;
}

//====================================================================
/**
 * Convert this CId into the Incomming String.
 * @param inStringId is a CId represented as a String.
 * @return This CId.
 */
CId &CId::FromString(const QString &inStringId)
{
    m_Key = QUuid(inStringId);

    return *this;
}

bool CId::IsZero() const
{
    return m_Key.isNull();
}

CId::operator GUID() const
{
    return Convert();
}
};
