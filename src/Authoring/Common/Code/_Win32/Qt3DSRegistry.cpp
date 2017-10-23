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

#include "stdafx.h"
#include "Qt3DSRegistry.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Qt3DSRegistry::Qt3DSRegistry()
{
}

Qt3DSRegistry::~Qt3DSRegistry()
{
}

//==============================================================================
/**
 *		GetValueKey: Query the specified value from the specified key.
 *
 *		@param	inKeyName
 *				The path and name of the key
 *
 *		@param	inValueName
 *				The value name
 *
 *		@param	outValueString
 *				Returns the value string
 *
 *		@param	inKeyRoot
 *				[ optional ] The registry root (defaults to HKEY_CURRENT_USER)
 *
 *		@return HRESULT
 */
#ifdef KDAB_TEMPORARILY_REMOVED
//==============================================================================
BOOL Qt3DSRegistry::GetValueKey(const Q3DStudio::CString &inKeyName,
                               const Q3DStudio::CString &inValueName,
                               Q3DStudio::CString &outValueString, HKEY inKeyRoot)
{
    BOOL theResult = FALSE;
    CRegKey theRegistryKey;

    outValueString = "";

    // If there is an error opening the key
    if (ERROR_SUCCESS == theRegistryKey.Open(inKeyRoot, inKeyName.c_str(), KEY_QUERY_VALUE)) {
        TCHAR theValueString[MAX_PATH];
        DWORD theStringSize = sizeof(theValueString);

        // if ( ERROR_SUCCESS == theRegistryKey.QueryValue( (LPTSTR) theValueString, inValueName,
        // &theStringSize ) )
        if (ERROR_SUCCESS
            == theRegistryKey.QueryStringValue(inValueName.c_str(), (LPTSTR)theValueString,
                                               &theStringSize)) {
            outValueString = theValueString;
            theResult = TRUE;
        }
    }

    return theResult;
}

//==============================================================================
/**
 *		SetValueKey: Query the specified value from the specified key.
 *
 *		@param	inKeyName
 *				The path and name of the key
 *
 *		@param	inValueName
 *				The value name
 *
 *		@param	inValueString
 *				The value string to ser
 *
 *		@param	inKeyRoot
 *				[ optional ] The registry root (defaults to HKEY_CURRENT_USER)
 *
 *		@return HRESULT
 */
//==============================================================================
BOOL Qt3DSRegistry::SetValueKey(const Q3DStudio::CString &inKeyName,
                               const Q3DStudio::CString &inValueName,
                               const Q3DStudio::CString &inValueString, HKEY inKeyRoot)
{
    BOOL theResult = FALSE;
    CRegKey theRegistryKey;

    // If there is an error opening the key
    if (ERROR_SUCCESS == theRegistryKey.Create(inKeyRoot, inKeyName)) {
        // if ( ERROR_SUCCESS == theRegistryKey.SetValue( inValueString, inValueName ) )
        if (ERROR_SUCCESS == theRegistryKey.SetStringValue(inValueName, inValueString)) {
            theResult = TRUE;
        }
    }

    return theResult;
}

//==============================================================================
/**
 *		GetValueKey: Query the specified value from the specified key.
 *
 *		@param	inKeyName
 *				The path and name of the key
 *
 *		@param	inValueName
 *				The value name
 *
 *		@param	outValueString
 *				Returns the value string
 *
 *		@param	inKeyRoot
 *				[ optional ] The registry root (defaults to HKEY_CURRENT_USER)
 *
 *		@return HRESULT
 */
//==============================================================================
BOOL Qt3DSRegistry::GetValueKey(const Q3DStudio::CString &inKeyName,
                               const Q3DStudio::CString &inValueName, long &outValue,
                               HKEY inKeyRoot)
{
    BOOL theResult = FALSE;
    CRegKey theRegistryKey;

    outValue = 0;

    // If there is an error opening the key
    if (ERROR_SUCCESS == theRegistryKey.Open(inKeyRoot, inKeyName, KEY_QUERY_VALUE)) {
        // if ( ERROR_SUCCESS == theRegistryKey.QueryValue( (DWORD&) outValue, inValueName ) )
        if (ERROR_SUCCESS == theRegistryKey.QueryDWORDValue(inValueName, (DWORD &)outValue)) {
            theResult = TRUE;
        }
    }

    return theResult;
}

//==============================================================================
/**
 *		SetValueKey: Query the specified value from the specified key.
 *
 *		@param	inKeyName
 *				The path and name of the key
 *
 *		@param	inValueName
 *				The value name
 *
 *		@param	inValueString
 *				The value string to set
 *
 *		@param	inKeyRoot
 *				[ optional ] The registry root (defaults to HKEY_CURRENT_USER)
 *
 *		@return HRESULT
 */
//==============================================================================
BOOL Qt3DSRegistry::SetValueKey(const Q3DStudio::CString &inKeyName,
                               const Q3DStudio::CString &inValueName, long inValue, HKEY inKeyRoot)
{
    BOOL theResult = FALSE;
    CRegKey theRegistryKey;

    // If there is an error opening the key
    if (ERROR_SUCCESS == theRegistryKey.Create(inKeyRoot, inKeyName)) {
        // if ( ERROR_SUCCESS == theRegistryKey.SetValue( inValue, inValueName ) )
        if (ERROR_SUCCESS == theRegistryKey.SetDWORDValue(inValueName, inValue)) {
            theResult = TRUE;
        }
    }

    return theResult;
}
#endif
