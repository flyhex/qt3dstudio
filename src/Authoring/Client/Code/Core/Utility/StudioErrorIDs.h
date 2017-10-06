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
#ifndef _INCLUDED_STUDIO_ERRROR_IDS_H
#define _INCLUDED_STUDIO_ERRROR_IDS_H

//==============================================================================
//	Includes
//==============================================================================
#include "ClientErrorIDs.h"

//==============================================================================
//	HRESULTS
//==============================================================================
#ifdef WIN32
#define STUDIO_ERROR(inID, inName)                                                                 \
    const HRESULT inName =                                                                         \
        MAKE_HRESULT(LEVEL_ERROR, FACILITY_ITF, UICCLIENT_START_ERROR_RANGE + inID);
#else
#ifndef HRESULT
#define HRESULT quint32
#endif
#define STUDIO_ERROR(inID, inName)                                                                 \
    const HRESULT inName =                                                                         \
        MAKE_HRESULT(LEVEL_ERROR, FACILITY_ITF, UICCLIENT_START_ERROR_RANGE + inID);
#endif

// EngineeringTask (SDJ 02/09/05) Clean up this file or get rid of it!!!!

STUDIO_ERROR(0, STUDIO_E_FAIL)
/*STUDIO_ERROR(   1, STUDIO_E_INVALIDARGUMENT			)
STUDIO_ERROR(   2, STUDIO_E_NULLARGUMENT			)
STUDIO_ERROR(   3, STUDIO_E_STATEOUTOFRANGE			)
STUDIO_ERROR(   4, STUDIO_E_OUTOFMEMORY				)
STUDIO_ERROR(   5, STUDIO_E_NOTIMPL					)
*/
STUDIO_ERROR(6, STUDIO_E_ACQUIREINTERFACE)
/*STUDIO_ERROR(   7, STUDIO_E_CREATEASSET				)
STUDIO_ERROR(   8, STUDIO_E_OBJECTMAPLOOKUP			)
STUDIO_ERROR(   9, STUDIO_E_REGISTERSTATE			)
*/
STUDIO_ERROR(10, STUDIO_E_NULLDOCUMENT)
STUDIO_ERROR(11, STUDIO_E_NULLSTUDIOOBJECT)
/*STUDIO_ERROR(  12, STUDIO_E_ILLEGALPROPERTYTYPE		)
STUDIO_ERROR(  13, STUDIO_E_ILLEGALSTATE			)
*/
STUDIO_ERROR(14, STUDIO_E_NULLWINDOW)
/*STUDIO_ERROR(  15, STUDIO_E_CREATEWINDOW			)
*/
STUDIO_ERROR(16, STUDIO_E_NULLVIEW)

STUDIO_ERROR(17, STUDIO_E_INVALIDPROGRAMSTATE)
/*STUDIO_ERROR(  18, STUDIO_E_ASSOCIATIONFAILED		)
STUDIO_ERROR(  19, STUDIO_E_PROPERTYLOOKUPFAILED	)
*/
STUDIO_ERROR(20, STUDIO_E_NULLCTRL)
/*STUDIO_ERROR(  21, STUDIO_E_UNEXPECTEDPARAMETERTYPE	)
STUDIO_ERROR(  22, STUDIO_E_NULLASSET				)
*/
STUDIO_ERROR(23, STUDIO_E_SCRIPTERROR)
/*
STUDIO_ERROR(  24, STUDIO_E_INVALIDURL				)
STUDIO_ERROR(  25, STUDIO_E_FAILHOTSWAP				)
*/
STUDIO_ERROR(25, STUDIO_E_INVALIDEDITOR)
/*
STUDIO_ERROR(  26, STUDIO_E_INVALIDOPCODE			)
STUDIO_ERROR(  27, STUDIO_E_CACHEASSET				)
STUDIO_ERROR(  28, STUDIO_E_EXPORTFAIL				)	/// Exporting a
presentation failed for an unknown reason - fatal error
STUDIO_ERROR(  29, STUDIO_E_NULLHANDLE				)
STUDIO_ERROR(  30, STUDIO_E_NULLDC					)
STUDIO_ERROR(  31, STUDIO_E_NULLKEYFRAME			)
STUDIO_ERROR(  32, STUDIO_E_CANCELEXPORT			)	/// Exporting presentation
failed, but only because the user cancelled the operation
STUDIO_ERROR(  33, STUDIO_E_NULLDRAGOBJECT			)	/// NULL Studio Object
for drag and drop
STUDIO_ERROR(  34, STUDIO_E_NULLCMD					)	/// NULL
undo/redo command (CCmd or subclass)
STUDIO_ERROR(  35, STUDIO_E_GETTIMERANGE			)
STUDIO_ERROR(  36, STUDIO_E_RESTOREASSET			)
STUDIO_ERROR(  37, STUDIO_E_DRAGNDROP				)
STUDIO_ERROR(  38, STUDIO_E_NOTRANSFEROBJECT		)
STUDIO_ERROR(  39, STUDIO_E_INVALIDTRANSFEROBJECT	)
*/
STUDIO_ERROR(42, STUDIO_E_EGLINITIALIZEERROR)

#endif // #ifndef _INCLUDED_STUDIO_ERRROR_IDS_H
