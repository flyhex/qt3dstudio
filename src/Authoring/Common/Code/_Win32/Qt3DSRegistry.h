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

#if !defined(AFX_UICREGISTRY_H__9E9A6CBC_0465_457A_9EFC_40835D4A8CA6__INCLUDED_)
#define AFX_UICREGISTRY_H__9E9A6CBC_0465_457A_9EFC_40835D4A8CA6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Qt3DSString.h"

//==============================================================================
/**
 *		@class	Qt3DSRegistry
 *		@brief	Studio Client Registry Functions.
 *
 *		Registry entries:
 *		-	Stores the latest installed version.
 */
//==============================================================================
class Qt3DSRegistry
{
public:
    Qt3DSRegistry();
    virtual ~Qt3DSRegistry();

#ifdef KDAB_TEMPORARILY_REMOVED
    static BOOL GetValueKey(const Q3DStudio::CString &inKeyName,
                            const Q3DStudio::CString &inValueName,
                            Q3DStudio::CString &outValueString, HKEY inKeyRoot = HKEY_CURRENT_USER);
    static BOOL SetValueKey(const Q3DStudio::CString &inKeyName,
                            const Q3DStudio::CString &inValueName,
                            const Q3DStudio::CString &inValueString,
                            HKEY inKeyRoot = HKEY_CURRENT_USER);

    static BOOL GetValueKey(const Q3DStudio::CString &inKeyName,
                            const Q3DStudio::CString &inValueName, long &outValue,
                            HKEY inKeyRoot = HKEY_CURRENT_USER);
    static BOOL SetValueKey(const Q3DStudio::CString &inKeyName,
                            const Q3DStudio::CString &inValueName, long inValue,
                            HKEY inKeyRoot = HKEY_CURRENT_USER);
#endif
};

#endif // !defined(AFX_UICREGISTRY_H__9E9A6CBC_0465_457A_9EFC_40835D4A8CA6__INCLUDED_)
