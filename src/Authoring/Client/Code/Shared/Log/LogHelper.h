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

#ifndef __QT3DS_LOG_HELPER_H_
#define __QT3DS_LOG_HELPER_H_

#ifdef WIN32
#include "LogProject.h"
#else
typedef long IQt3DSLog2;
#endif

//==============================================================================
/**
 *		@class	CLogHelper
 *		@brief	This class provides helper functions to assist with logging.
 */
//==============================================================================
class CLogHelper
{
public:
    static void Log(long inLogType, const wchar_t *inFileName, long inLineNumber,
                    long inCategoryType, const wchar_t *inLogString, ...);
    static void Log(long inLogType, const wchar_t *inFileName, long inLineNumber,
                    long inCategoryType, long inStringId, ...);
    static void Trace(long inCategoryType, const wchar_t *inLogString, ...);
    static void AddEntry(long inType, const wchar_t *inMsg);
    static void Start();
    static void Stop();

    static IQt3DSLog2 *s_qt3dsLog2;
};

#endif // __QT3DS_LOG_HELPER_H_
