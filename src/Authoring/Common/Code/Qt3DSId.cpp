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
    : m_Key(Q3DStudio::UUID_ZERO)
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
    Q3DStudio::CString theString = inStringId;
    FromString(theString);
}

//====================================================================
/**
 * Constructor.
 * @param inId is an ID to initialize from.
 */
CId::CId(const CId &inId)
{
    m_Key.GuidPacked.Data1 = inId.m_Key.GuidPacked.Data1;
    m_Key.GuidPacked.Data2 = inId.m_Key.GuidPacked.Data2;
    m_Key.GuidPacked.Data3 = inId.m_Key.GuidPacked.Data3;
    m_Key.GuidPacked.Data4 = inId.m_Key.GuidPacked.Data4;
}

CId::CId(const GUID &inGUID)
{
    m_Key.GuidStd.Data1 = inGUID.Data1;
    m_Key.GuidStd.Data2 = inGUID.Data2;
    m_Key.GuidStd.Data3 = inGUID.Data3;
    ::memcpy(m_Key.GuidStd.Data4, inGUID.Data4, sizeof(inGUID.Data4));
}

CId::CId(long in1, long in2, long in3, long in4)
{
    m_Key.GuidPacked.Data1 = in1;
    m_Key.GuidPacked.Data2 = in2;
    m_Key.GuidPacked.Data3 = in3;
    m_Key.GuidPacked.Data4 = in4;
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
    m_Key.GuidPacked.Data1  = ::rand();
    m_Key.GuidPacked.Data1 <<= 16;
    m_Key.GuidPacked.Data1  |= ::rand();

    m_Key.GuidPacked.Data2  = ::rand();
    m_Key.GuidPacked.Data2 <<= 16;
    m_Key.GuidPacked.Data2  |= ::rand();

    m_Key.GuidPacked.Data3  = ::rand();
    m_Key.GuidPacked.Data3 <<= 16;
    m_Key.GuidPacked.Data3  |= ::rand();

    m_Key.GuidPacked.Data4  = ::rand();
    m_Key.GuidPacked.Data4 <<= 16;
    m_Key.GuidPacked.Data4  |= ::rand();
}

//====================================================================
/**
 * Convert this CId to a GUID.
 * @return a GUID struct.
 */
GUID CId::Convert() const
{
    GUID theThis;

    theThis.Data1 = m_Key.GuidStd.Data1;
    theThis.Data2 = m_Key.GuidStd.Data2;
    theThis.Data3 = m_Key.GuidStd.Data3;
    ::memcpy(theThis.Data4, m_Key.GuidStd.Data4, sizeof(m_Key.GuidStd.Data4));

    return theThis;
}

//====================================================================
/**
 * Compare two CIds.
 * @param inRVal is the Right side CId.
 * @return True if they were equal.
 */
bool CId::operator==(const CId &inRVal) const
{
    bool theResult = ((m_Key.GuidPacked.Data1 == inRVal.m_Key.GuidPacked.Data1)
                      && (m_Key.GuidPacked.Data2 == inRVal.m_Key.GuidPacked.Data2)
                      && (m_Key.GuidPacked.Data3 == inRVal.m_Key.GuidPacked.Data3)
                      && (m_Key.GuidPacked.Data4 == inRVal.m_Key.GuidPacked.Data4));

    return theResult;
}

//====================================================================
/**
 * Compare two CIds.
 * @param inRVal is the Right side CId.
 * @return True if they were equal.
 */
bool CId::operator==(const Q3DStudio::UUID &inRVal) const
{
    bool theResult = ((m_Key.GuidPacked.Data1 == inRVal.GuidPacked.Data1)
                      && (m_Key.GuidPacked.Data2 == inRVal.GuidPacked.Data2)
                      && (m_Key.GuidPacked.Data3 == inRVal.GuidPacked.Data3)
                      && (m_Key.GuidPacked.Data4 == inRVal.GuidPacked.Data4));

    return theResult;
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
    //	const Q3DStudio::TGUIDPacked& theLVal = m_Key.GuidPacked;
    //	const Q3DStudio::TGUIDPacked& theRVal = inRVal.m_Key.GuidPacked;
    //	return !(	theLVal.Data1 >= theRVal.Data1 &&
    //				theLVal.Data2 >= theRVal.Data2 &&
    //				theLVal.Data3 >= theRVal.Data3 &&
    //				theLVal.Data4 >= theRVal.Data4 );

    bool theResult = false;

    if (m_Key.GuidPacked.Data1 < inRVal.m_Key.GuidPacked.Data1) {
        theResult = true;
    }
    // If it is equal then move on to the next element.
    else if (m_Key.GuidPacked.Data1 == inRVal.m_Key.GuidPacked.Data1) {
        if (m_Key.GuidPacked.Data2 < inRVal.m_Key.GuidPacked.Data2) {
            theResult = true;
        }
        // If it is equal then move on to the next element.
        else if (m_Key.GuidPacked.Data2 == inRVal.m_Key.GuidPacked.Data2) {
            if (m_Key.GuidPacked.Data3 < inRVal.m_Key.GuidPacked.Data3) {
                theResult = true;
            } else if (m_Key.GuidPacked.Data3 == inRVal.m_Key.GuidPacked.Data3) {
                if (m_Key.GuidPacked.Data4 < inRVal.m_Key.GuidPacked.Data4) {
                    theResult = true;
                }
            }
        }
    }
    return theResult;
}

//====================================================================
/**
 * Set this Cid equal to the Right Side CId.
 * @param inRVal is the Right side CId.
 * @return this CId.
 */
CId &CId::operator=(const CId &inRVal)
{
    m_Key.GuidPacked.Data1 = inRVal.m_Key.GuidPacked.Data1;
    m_Key.GuidPacked.Data2 = inRVal.m_Key.GuidPacked.Data2;
    m_Key.GuidPacked.Data3 = inRVal.m_Key.GuidPacked.Data3;
    m_Key.GuidPacked.Data4 = inRVal.m_Key.GuidPacked.Data4;

    return *this;
}
//====================================================================
/**
 * Convert this CId into a String.
 * @return The String version of this CId.
 */
Q3DStudio::CString CId::ToString() const
{
    Q3DStudio::CString theGuidStr;
    theGuidStr.Format(_LSTR("{%08X-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
                      m_Key.GuidStd.Data1, m_Key.GuidStd.Data2, m_Key.GuidStd.Data3,
                      m_Key.GuidStd.Data4[0], (unsigned long)m_Key.GuidStd.Data4[1],
                      (unsigned long)m_Key.GuidStd.Data4[2], (unsigned long)m_Key.GuidStd.Data4[3],
                      (unsigned long)m_Key.GuidStd.Data4[4], (unsigned long)m_Key.GuidStd.Data4[5],
                      (unsigned long)m_Key.GuidStd.Data4[6], (unsigned long)m_Key.GuidStd.Data4[7]);

    return theGuidStr;
}

//====================================================================
/**
 * Convert this CId into the Incomming String.
 * @param inStringId is a CId represented as a String.
 * @return This CId.
 */
CId &CId::FromString(const Q3DStudio::CString &inStringId)
{
    unsigned long theData[8];
    const wchar_t *theBuffer = (const wchar_t *)inStringId;

    // All of the %02X formats get scaned into a 32bit long so we have to convert down below....
    long theFields = ::swscanf(theBuffer, L"{%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                               &m_Key.GuidStd.Data1, &m_Key.GuidStd.Data2, &m_Key.GuidStd.Data3,
                               &theData[0], &theData[1], &theData[2], &theData[3], &theData[4],
                               &theData[5], &theData[6], &theData[7]);

    if (theFields == 11) // magic number
    {
        // This is where we convert the 32 values into unsigned chars.
        for (long theIndex = 0; theIndex < 8; ++theIndex) {
            m_Key.GuidStd.Data4[theIndex] = static_cast<unsigned char>(theData[theIndex]);
        }
    } else {
        // If we didn't read in all the fields, zero out the key
        m_Key = Q3DStudio::UUID_ZERO;
    }

    return *this;
}

bool CId::IsZero() const
{
    return (*this == Q3DStudio::UUID_ZERO);
}

CId::operator GUID() const
{
    return Convert();
}
};
