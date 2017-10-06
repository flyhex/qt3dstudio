/****************************************************************************
**
** Copyright (C) 1999-2001 Anark Corporation.
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

#ifndef __CLIENTERRORIDS_H__
#define __CLIENTERRORIDS_H__

#include "UICExceptions.h"

//==============================================================================
//	Defines
//==============================================================================

///< Base = 0x80040000

const unsigned long GLOBALCLIENT_START_ERROR_RANGE = 10000; ///< 0x80042710
const unsigned long GLOBALCLIENT_END_ERROR_RANGE = 10499; ///< 0x80042903
const unsigned long CONTROLLER_START_ERROR_RANGE = 10500; ///< 0x80042904
const unsigned long EVENT_START_ERROR_RANGE = 11000; ///< 0x80042AF8
const unsigned long GIT_START_ERROR_RANGE = 11500; ///< 0x80042CEC
const unsigned long ICE_START_ERROR_RANGE = 12000; ///< 0x80042EE0
const unsigned long IOLIBRARY_START_ERROR_RANGE = 12500; ///< 0x800430D4
const unsigned long LIBRARY_START_ERROR_RANGE = 13000; ///< 0x800432C8
const unsigned long OBJECTMODEL_START_ERROR_RANGE = 13500; ///< 0x800434BC
const unsigned long PLAYERCP_START_ERROR_RANGE = 14000; ///< 0x800436B0
const unsigned long PLAYER_START_ERROR_RANGE = 14500; ///< 0x800438A4
const unsigned long RENDER_START_ERROR_RANGE = 15000; ///< 0x80043A98
const unsigned long COMPRESSION_START_ERROR_RANGE = 15400; ///< 0x80043C28
const unsigned long SCRIPTENGINE_START_ERROR_RANGE = 15500; ///< 0x80043C8C
const unsigned long UNUSED1 = 16000; ///< 0x80043E80
const unsigned long TIMER_START_ERROR_RANGE = 16500; ///< 0x80044074
const unsigned long TRANSM_START_ERROR_RANGE = 17000; ///< 0x80044268
const unsigned long TYPES_START_ERROR_RANGE = 17500; ///< 0x8004445C
const unsigned long UTILITY_START_ERROR_RANGE = 18000; ///< 0x80044650

const unsigned long PLUGIN3DS_START_ERROR_RANGE = 18400; ///< 0x800447E0
const unsigned long PLUGINUICA_START_ERROR_RANGE = 18450; ///< 0x80044812
const unsigned long PLUGINUICB_START_ERROR_RANGE = 18500; ///< 0x
const unsigned long PLUGINUICI_START_ERROR_RANGE = 18550; ///< 0x
const unsigned long PLUGINUICM_START_ERROR_RANGE = 18600; ///< 0x
const unsigned long PLUGINBVS_START_ERROR_RANGE = 18650; ///< 0x
const unsigned long PLUGINAKS_START_ERROR_RANGE = 18700; ///< 0x
const unsigned long PLUGINIMG_START_ERROR_RANGE = 18750; ///< 0x
const unsigned long PLUGINMP3_START_ERROR_RANGE = 18800; ///< 0x
const unsigned long PLUGINUICTM_START_ERROR_RANGE = 18850; ///< 0x
const unsigned long PLUGINWMM_START_ERROR_RANGE = 19900; ///< 0x

const unsigned long UICCLIENT_START_ERROR_RANGE = 20000; ///< 0x80044e20

const unsigned long INSTALLER_START_ERROR_RANGE = 21000; ///< 0x80045208
const unsigned long INSTANCEDATA_START_ERROR_RANGE = 21500; ///< 0x
const unsigned long SHARED_START_ERROR_RANGE = 21600; ///< 0x
const unsigned long GLRENDER_START_ERROR_RANGE = 21700; ///< 0x

// NOTE: The Component project has many, many objects, each of which may
// have many many errors. Leave a large block of numbers reserved for the
// Component errors. Each element should also leave room at the end of
// its sub-allocation so the list can grow with the least amount of trauma.
const unsigned long COMPONENT_START_ERROR_RANGE = 22000; ///< 0x
const unsigned long UNUSED5 = 27000; ///< 0x

#ifndef FACILITY_ITF
#define FACILITY_ITF 0x0004
#endif

#ifndef MAKE_HRESULT
#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )
#endif

//==============================================================================
//	HRESULTS
//==============================================================================
#define GLOBALCLIENT_ERROR(inID, inName)                                                           \
    const HRESULT inName =                                                                         \
        MAKE_HRESULT(LEVEL_ERROR, FACILITY_ITF, GLOBALCLIENT_START_ERROR_RANGE + inID);

GLOBALCLIENT_ERROR(
    0, GLOBALCLIENT_E_FATALERROR) ///< 0x80042710 - Sell the farm jenny, the cows have all gone home
GLOBALCLIENT_ERROR(1, GLOBALCLIENT_E_FAILEDTORENDER) ///< 0x80042711

const HRESULT GLOBALCLIENT_E_STARTRANGE =
    MAKE_HRESULT(LEVEL_ERROR, FACILITY_ITF, GLOBALCLIENT_START_ERROR_RANGE);
const HRESULT GLOBALCLIENT_E_ENDRANGE =
    MAKE_HRESULT(LEVEL_ERROR, FACILITY_ITF, GLOBALCLIENT_END_ERROR_RANGE);

//==============================================================================
//	Logging bits for seperate modules
//==============================================================================

const unsigned long LOGTYPE_GLOBALCLIENT = 0x00000001;
const unsigned long LOGTYPE_STUDIOCLIENT = 0x00000002;
const unsigned long LOGTYPE_CONTROLLER = 0x00000004;
const unsigned long LOGTYPE_COMPONENT = 0x00000008;
const unsigned long LOGTYPE_PLAYERCP = 0x00000010;
const unsigned long LOGTYPE_PLAYER = 0x00000020;
const unsigned long LOGTYPE_RENDER = 0x00000040;
const unsigned long LOGTYPE_SCRIPTENGINE = 0x00000080;
const unsigned long LOGTYPE_LIBRARY = 0x00000100;
const unsigned long LOGTYPE_BVS = 0x00000200;
const unsigned long LOGTYPE_TRANSM = 0x00000400;
const unsigned long LOGTYPE_UTILITY = 0x00000800;
const unsigned long LOGTYPE_TYPES = 0x00001000;
const unsigned long LOGTYPE_REFCOUNTS = 0x00002000;
const unsigned long LOGTYPE_OBJECTMODEL = 0x00004000;
const unsigned long LOGTYPE_ICELIB = 0x00008000;
const unsigned long LOGTYPE_THREAD = 0x00008001;

// Save 0x80000000 to use as a flag to identify
// the next range of bit masks
// const unsigned long UNUSED_ID	= 0x80000001;

typedef struct _TSLogTypeEntry
{
    long m_LogTypeBitMask;
    char m_LogTypeName[15];

} TSLogTypeEntry;

const TSLogTypeEntry g_LogTypeNameMap[] = {
    { LOGTYPE_GLOBALCLIENT, "GLOBAL" },
    { LOGTYPE_STUDIOCLIENT, "CLIENT" },
    { LOGTYPE_CONTROLLER, "CONTROLLER" },
    { LOGTYPE_COMPONENT, "COMPONENT" },
    { LOGTYPE_PLAYERCP, "CONNECTPOINT" },
    { LOGTYPE_PLAYER, "PLAYER" },
    { LOGTYPE_RENDER, "RENDER" },
    { LOGTYPE_SCRIPTENGINE, "SCRIPT" },
    { LOGTYPE_LIBRARY, "LIBRARY" },
    { LOGTYPE_BVS, "BVS" },
    { LOGTYPE_TRANSM, "TRANSM" },
    { LOGTYPE_UTILITY, "UTILITY" },
    { LOGTYPE_TYPES, "TYPES" },
    { LOGTYPE_REFCOUNTS, "REFCOUNTS" },
    { LOGTYPE_OBJECTMODEL, "OBJECTMODEL" },
    { LOGTYPE_ICELIB, "ICELIB" },
    { LOGTYPE_THREAD, "THREAD" },
};

const long g_LogTypeNameMapSize = sizeof(g_LogTypeNameMap) / sizeof(TSLogTypeEntry);

#endif // #ifndef __CLIENTERRORIDS_H__
