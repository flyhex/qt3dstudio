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

//==============================================================================
//	Prefix
//==============================================================================

#ifndef _UICLOG_H__
#define _UICLOG_H__

#pragma once

//==============================================================================
//	Includes
//==============================================================================

#include "UICExceptions.h" // for UIC_THIS_FILE

//==============================================================================
//	Constants
//==============================================================================

#ifdef UIC_NO_LOGGING

#define UIC_LOGRAW(inType, inMsg)
#define UIC_LOGMSG(inMsg)
#define UIC_LOGSTART
#define UIC_LOGSTOP
#define UIC_LOGGLOBAL
#define UIC_LOGEXTERN
#define UIC_LOG(inTypeId, inMessage)
#define UIC_LOG1(inTypeId, inMessage, inParam1)
#define UIC_LOG2(inTypeId, inMessage, inParam1, inParam2)
#define UIC_LOGNOTYPE(inMessage)
#define UIC_TRACE

#else // #ifdef UIC_NO_LOGGING

#include "LogTypes.h"
#include "LogHelper.h"

//==============================================================================
//	Macros
//==============================================================================

// NOTE: Use UIC_LOG, UIC_LOG1 and UIC_LOG2 as defined in UICExceptions.h instead of these.

// Simple log entry with type
#define UIC_LOGRAW(inType, inMsg) CLogHelper::AddEntry(inType, inMsg);

// Simple log entry
#define UIC_LOGMSG(inMsg) UIC_LOGRAW(LOGMODE_LOG, inMsg)

// Create or link to singleton logger.  Include this once in the startup code for each module
#define UIC_LOGSTART CLogHelper::Start();

// Release COM singleton. Include this once in the startup code for each module
#define UIC_LOGSTOP CLogHelper::Stop();

// Publish global log variable.  Include this once OUTSIDE of a method to define the global
// variable.
#define UIC_LOGGLOBAL

/*
#define UIC_LOGGLOBAL					\
        IUICLog2*	CLogHelper::s_UICLog2 = NULL;	\
        long		CLogHelper::s_UICLogCount = 0;
*/

// Publish global log variable.  Add this to every file, preferably in stdafx.h
#define UIC_LOGEXTERN

// A log entry with no extra parameters
#define UIC_LOG(inTypeId, inMessage)                                                               \
    CLogHelper::Log(LOGMODE_LOG, UIC_THIS_FILE, __LINE__, inTypeId, L#inMessage);

// A log entry with one extra parameter
#define UIC_LOG1(inTypeId, inMessage, inParam1)                                                    \
    CLogHelper::Log(LOGMODE_LOG, UIC_THIS_FILE, __LINE__, inTypeId, L#inMessage, inParam1);

// A log entry with two extra parameters
#define UIC_LOG2(inTypeId, inMessage, inParam1, inParam2)                                          \
    CLogHelper::Log(LOGMODE_LOG, UIC_THIS_FILE, __LINE__, inTypeId, L#inMessage, inParam1,         \
                    inParam2);

// A log entry with no extra parameters
#define UIC_LOGNOTYPE(inMessage)                                                                   \
    CLogHelper::Log(LOGMODE_LOG, UIC_THIS_FILE, __LINE__, 0, L#inMessage);

#define UIC_TRACE CLogHelper::Trace

#endif // #ifndef UIC_NO_LOGGING
#endif // #ifndef _UICLOG_H__
