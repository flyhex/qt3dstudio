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

#ifndef __QT3DS_LOG_TYPES_H_
#define __QT3DS_LOG_TYPES_H_

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

#include <QtGlobal>

//==============================================================================
/**
 *	@typedef	TSAddEntryIndirect
 */
//==============================================================================
typedef struct _TSAddEntryIndirect
{
    long m_StructSize; ///< The size of this struct ( for versioning )
    long m_LogStringId; ///< The ID of the log string in the resource DLL
    wchar_t m_LogString[MAX_PATH]; ///< The log string (for quick debugging)
    va_list m_LogParamList; ///< Variable parameter list
    long m_LogType; ///< The log type (log/exception)
    long m_CategoryType; ///< The category (component, render, etc)
    wchar_t m_FileNameString[MAX_PATH]; ///< The file name
    long m_LineNumberLong; ///< The line number
    quint32 m_Mask; ///< The mask that identifies valid members of this struct
    quint32 m_Level; ///< The mask that identifies valid members of this struct

} TSAddEntryIndirect;

//==============================================================================
//	TSAddEntryIndirect mask ids
//==============================================================================

const unsigned long QT3DS_LOG_STRING = 0x00000001; ///<
const unsigned long QT3DS_LOG_ID = 0x00000002; ///<

//==============================================================================
//	Log Modes
//==============================================================================

enum ELogMode ///< Logging mode determined by registry lookup
{ LOGMODE_NONE = 0x00000000, ///< Log nothing
  LOGMODE_LOG = 0x00000001, ///< Log logs
  LOGMODE_EXCEPTION = 0x00000002, ///< Log exceptions
  LOGMODE_ALL = LOGMODE_LOG | LOGMODE_EXCEPTION ///< Log everthing
};

//==============================================================================
//	Log Levels
//==============================================================================

enum ELogLevel {
    LOGLEVEL_UNKNOWN = 0,
    LOGLEVEL_CRITICAL = 1,
    LOGLEVEL_WARNING = 2,
    LOGLEVEL_INFO = 3,
};

#endif // __QT3DS_LOG_TYPES_H_
