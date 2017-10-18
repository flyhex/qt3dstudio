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

//==============================================================================
//						CoreLib Precompiled Header
//==============================================================================
#ifdef __cplusplus
#pragma once
#include "UICMacros.h"

#ifdef _WIN32
//==============================================================================
//	Disable certain warnings since warnings are errors
//==============================================================================
#pragma warning(disable : 4702) // Unreachable code
#pragma warning(disable : 4290) // C++ Exception Specification ignored
#pragma warning(disable : 4514) // unreferenced inline function
#pragma warning(disable : 4819)
#endif

//==============================================================================
//	Common Includes
//==============================================================================
#include <stdio.h> // Standard includes MUST come first
#include <stdlib.h>
#include <float.h>
#include <math.h>

#ifdef _WIN32
//==============================================================================
//	Windows Includes
//==============================================================================
#include <winsock2.h> //must include before windows.h to avoid winsock.h defines.
#define QT3DS_LITTLE_ENDIAN
#include <windows.h>
//==============================================================================
//	ATL Includes
//==============================================================================
#include <atlbase.h>
// You may derive a class from CComModule and use it if you want to override
// something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>
#include <atlctl.h>
#include <atlhost.h>

#include <COMMCTRL.H>
#include <LIMITS.H>
#include <CONTROL.H>
#endif

#include <assert.h>
#ifndef ASSERT
#define ASSERT(a) assert(a)
#endif

//==============================================================================
//	STL Includes
//==============================================================================
#ifdef _WIN32
#pragma warning(push, 3) // Temporarily pop to warning level 3 while including standard headers
#pragma warning(disable : 4018) // Disable mismatched < STL warning
#endif
#include <vector>
#include <map>
#include <deque>
#include <string>
#include <stack>
#include <set>
#include <list>
#include <utility>
#include <algorithm>
#include <stdexcept>
#include <limits>
#ifdef _WIN32
#pragma warning(pop) // Pop out to previous warning level (probably level 4)
#endif

//==============================================================================
//	Memory Manager Includes
//==============================================================================
#ifdef USE_MEMORY_MANAGER
#include <mmgr.h>
#endif

//==============================================================================
//	Common Player Includes
//==============================================================================
#include "UICMath.h"
#include "UICPoint.h"
#include "UICRect.h"
#include "UICString.h"
#include "UICMessageBox.h"
#include "UICObjectCounter.h"
#include "PlatformTypes.h"
#include "PlatformMacros.h"
#include "PlatformStrings.h"
#include "PlatformConversion.h"
#include "CommonConstants.h"
#include "UICExceptions.h"
#include "UICExceptionConstants.h"
#include "UICLog.h"
#include "Mutex.h"
#include "Guard.h"
#include "Thread.h"
#include "Conditional.h"
#include "STLHelpers.h"
#include "GenericFunctor.h"
#include "Qt3DSRender.h"

#include <QtGlobal>

//==============================================================================
//	Global Variables
//==============================================================================
QT3DS_LOGEXTERN; // Enables access to the logger for all files.  See UICLog for more info.

// I put these in to help with some of the data model compile times.
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4100)
#pragma warning(disable : 4512)
#pragma warning(disable : 4702)
#pragma warning(disable : 4996)
#endif
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#ifdef _WIN32
#pragma warning(pop)

#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4189) // local variable is initialized but not referenced
#endif
#endif
