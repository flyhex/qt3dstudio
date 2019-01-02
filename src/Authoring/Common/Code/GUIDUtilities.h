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
#pragma once

//==============================================================================
//	Includes
//==============================================================================
//#include "Qt3DSTime.h"
#include "Qt3DSString.h"
#include "Qt3DSId.h"
#include <stdio.h>

/*
        CId has been revamped to use the more robust OS-dependent GUID generator.
        Because this utility file is heavily used all over the place,
        the best option is to wrap it around CId.
*/

const GUID UNASSIGNED_GUID = { 0, 0, 0, "" };
const GUID SCENE_GUID = {
    0xffffffff, 0xffff, 0xffff, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
};

//==============================================================================
/**
 *	less()	Compared two GUIDs
 *
 *	A compares two GUIDs
 *
 *	@param	<GUID>	inGUID1	The GUID to compare to the second
 *	@param	<GUID>	inGUID2 The GUID to compare to the first
 *
 *	@return	<bool>	true if GUID1 and GUID2 are equal, otherwise false.
 */
inline bool lessGUID(const GUID &inValL, const GUID &inValR)
{
    return ::memcmp(&inValL, &inValR, sizeof(inValR)) < 0;
}

//==============================================================================
/**
 *	equal()	Compared two GUIDs
 *
 *	A compares two GUIDs
 *
 *	@param	<GUID>	inGUID1	The GUID to compare to the second
 *	@param	<GUID>	inGUID2 The GUID to compare to the first
 *
 *	@return	<bool>	true if GUID1 and GUID2 are equal, otherwise false.
 */
inline bool equalGUID(const GUID &inValL, const GUID &inValR)
{
    return ::memcmp(&inValL, &inValR, sizeof(inValR)) == 0;
}

//==============================================================================
/**
 *	@class	CGUIDCompare
 *	@brief	A class that enacapsulates the compare operator for two GUIDs.
 */
class CGUIDCompare
{
public:
    //==============================================================================
    /*
    *	operator()	Compared two GUIDs
    *
    *	A compares two GUIDs
    *
    *	@param	<GUID>	inGUID1	The GUID to compare to the second
    *	@param	<GUID>	inGUID2 The GUID to compare to the first
    *
    *	@return	<bool>	true if GUID1 and GUID2 are equal, otherwise false.
    */
    bool operator()(const GUID &inValL, const GUID &inValR) const
    {
        return lessGUID(inValL, inValR);
    }
};

class CGUIDTools
{
public:
    //==============================================================================
    /**
    *	Prepeare to generate GUIDs.  If this method is not called the GUIDs will
    *	but evil GUIDs that are the same evertime the program runs.
    *	@param	outGuid Returns the generated GUID
    */
    static void Initialize()
    {
        // Q3DStudio::CTime theTime( true );
        //::srand( static_cast<unsigned int>( theTime.GetMilliSecs( ) ) );
    }

    //==============================================================================
    /**
    *	GenerateGuid
    *	@note	This is not a GUID.  Correct creation involves spatial uniqueness and more:
    *	http://hegel.ittc.ukans.edu/topics/internet/internet-drafts/draft-l/draft-leach-uuids-guids-00.txt
    *	@param	outGuid Returns the generated GUID
    */
    static void GenerateGuid(GUID &outGuid)
    {
        // Debug - non-random GUIDs
        /*		static long		theGuidCount = 1;
                        GUID			theGuid = {0};

                        theGuid.Data1 = theGuidCount;

                        ++theGuidCount;
                        outGuid = theGuid;*/

        // ::rand is 16 bits, GUID is 128 bits, smear random bits over the whole GUID
        // for ( long theSlot = 0; theSlot < sizeof(GUID)/sizeof(short); ++theSlot )
        //	reinterpret_cast<short*>( &outGuid )[theSlot] = static_cast<short>( ::rand() );

        Q3DStudio::CId theID;
        theID.Generate();
        outGuid = theID.Convert();
    }

    static GUID FromString(const QString &inGuidStr)
    {
        GUID theGUID = { 0, 0, 0, "" };
        // unsigned long theData[8];
        // const char* theBuffer = inGuidStr.GetCharStar( );
        //
        //// All of the %02X formats get scaned into a 32bit long so we have to convert down
        ///below....
        //::sscanf( theBuffer, "{%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x}",
        //																	&theGUID.Data1,
        //																	&theGUID.Data2,
        //																	&theGUID.Data3,
        //																	&theData[0],
        //																	&theData[1],
        //																	&theData[2],
        //																	&theData[3],
        //																	&theData[4],
        //																	&theData[5],
        //																	&theData[6],
        //																	&theData[7]);

        //// This is where we convert the 32 values into unsigned chars.
        // for ( long theIndex = 0; theIndex < 8; ++theIndex )
        //	theGUID.Data4[theIndex] = static_cast<unsigned char>( theData[theIndex] );

        Q3DStudio::CId theID;
        theID.FromString(inGuidStr);
        theGUID = theID.Convert();

        return theGUID;
    }

    static QString ToString(REFGUID inGuid)
    {
        QString theGuidStr;

        // theGuidStr.Format( _LSTR( "{%08X-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}" ),
        //										inGuid.Data1,
        //										inGuid.Data2,
        //										inGuid.Data3,
        //										inGuid.Data4[0],
        //										(unsigned long)
        //inGuid.Data4[1],
        //										(unsigned long)
        //inGuid.Data4[2],
        //										(unsigned long)
        //inGuid.Data4[3],
        //										(unsigned long)
        //inGuid.Data4[4],
        //										(unsigned long)
        //inGuid.Data4[5],
        //										(unsigned long)
        //inGuid.Data4[6],
        //										(unsigned long)
        //inGuid.Data4[7] );

        Q3DStudio::CId theID(inGuid);
        theGuidStr = theID.ToString();

        return theGuidStr;
    }
};
