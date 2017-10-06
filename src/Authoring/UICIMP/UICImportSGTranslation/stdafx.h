/****************************************************************************
**
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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <QtGlobal>

#ifdef WIN32

#ifndef _WIN32_WINNT // Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT                                                                               \
    0x0501 // Change this to the appropriate value to target other versions of Windows.
#endif

#endif

#ifndef _WIN32
typedef void *HANDLE;
#endif

#include <assert.h>
#ifndef ASSERT
#define ASSERT(a) assert(a)
#endif

#ifdef WIN32
#pragma warning(disable : 4819)
#endif
#include "UICImportLibPrecompile.h"
#include "PlatformTypes.h"

#define SAFE_DELETE(ptr)                                                                           \
    if ((ptr) != NULL) {                                                                           \
        delete (ptr);                                                                              \
        (ptr) = NULL;                                                                              \
    }
// TODO: reference additional headers your program requires here
