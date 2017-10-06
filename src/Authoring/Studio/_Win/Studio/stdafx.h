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

#ifdef __cplusplus
#pragma once

#pragma warning(disable : 4819)

#include "UICMacros.h"

//#define WIN32_LEAN_AND_MEAN	// Exclude rarely-used stuff from Windows headers
#define VC_EXTRALEAN // Exclude morerarely-used stuff from Windows headers

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS // some CString constructors will be explicit
#define _AFX_ALL_WARNINGS // turns off MFC's hiding of some common and often safely ignored warning
                          // messages

#ifdef DEBUG
#define CHECK_BOUNDS // CHECK_BOUNDS affects the way an item is retrieved from container objects
#endif

#if _MSC_VER >= 1400
#if defined _M_IX86
#pragma comment(                                                                                   \
    linker,                                                                                        \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(                                                                                   \
    linker,                                                                                        \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(                                                                                   \
    linker,                                                                                        \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(                                                                                   \
    linker,                                                                                        \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

//==============================================================================
//	Standard Includes
//==============================================================================
#include <stdio.h> // Standard includes MUST come first
#include <stdlib.h>
#include <float.h>
#include <math.h>

//==============================================================================
//	STL Includes
//==============================================================================
#pragma warning(push, 3) // Temporarily pop to warning level 3 while including standard headers
#pragma warning(disable : 4018) // Disable mismatched < STL warning
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
#pragma warning(pop) // Pop out to previous warning level (probably level 4)

//==============================================================================
//	MFC Includes
//==============================================================================
#pragma warning(push, 1)
#include <afxwin.h> // MFC core and standard components
#include <afxext.h> // MFC extensions
#include <afxdisp.h> // MFC Automation classes
#include <afxpriv.h> // AFX Windows Messages (WM_INITIALUPDATE for example)
#include <afxdtctl.h> // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h> // MFC support for Windows Common Controls
#include <atlbase.h>
#include <atlcom.h>
#endif // _AFX_NO_AFXCMN_SUPPORT

//==============================================================================
//	ATL Includes
//==============================================================================
#include "atlbase.h" // For CrystalEdit
#include "afxtempl.h" // For CrystalEdit
#include "afxole.h" // For CrystalEdit
#pragma warning(pop)

// I put these in to help with some of the data model compile times.
#pragma warning(push)
#pragma warning(disable : 4100)
#pragma warning(disable : 4512)
#pragma warning(disable : 4702)
#pragma warning(disable : 4996)
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/signals.hpp>
#include <boost/signals/connection.hpp>
#pragma warning(pop)

//==============================================================================
//	Disable certain warnings since warnings are errors
//==============================================================================
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4702) // Unreachable code
#pragma warning(disable : 4290) // C++ Exception Specification ignored
#pragma warning(disable : 4514) // Unreferenced inline function
#pragma warning(disable : 4121) // Alignment of member sensative to packing
#pragma warning(disable : 4512) // Assignment not generated
#pragma warning(disable : 4355) // This used in member initializer list.
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4189) // local variable is initialized but not referenced

//==============================================================================
//	Common Includes
//==============================================================================
#include "GUIDUtilities.h"
#include "UICId.h"

//==============================================================================
//	Application Includes (Aug '08 accumulated most popular include files)
//==============================================================================
#include "UICString.h"
#include "UICMath.h"
#include "StringLoader.h"
#include "StudioException.h"
#include "UICExceptions.h"
#include "Exceptions.h"
#include "UICObjectCounter.h"
#include "STLHelpers.h"
#include "Pt.h"
#include "Rct.h"
#include "SafeArray.h"
#include "PlatformMacros.h"
#include "PlatformTypes.h"
#include "Multicaster.h"
#include "GenericFunctor.h"
#include "CColor.h"
#include "InputStream.h"
#include "OutputStream.h"
#include "StudioUtils.h"
#include "StudioDefs.h"
#include "StudioErrorIDs.h"
#include "Strings.h"
#include "StudioPreferences.h"
#include "Renderer.h"
#include "StudioObjectTypes.h"
#include "HotKeys.h"
#include "Dispatch.h"
#include "Cmd.h"
#include "MasterP.h"
#include "Dialogs.h"
#include "Views.h"
#include "ResourceCache.h"
#include "CmdBatch.h"
#include "DispatchListeners.h"
#include "UICFile.h"
#include "StudioConst.h"
#include "resource.h"
#include "StudioProjectSettings.h"
#include "Preferences.h"
#include "StudioClipboard.h"
#include "DropTarget.h"
#include "ContextMenu.h"
#include "TextButton.h"

#include "StudioPrefixWin32.h"

//==============================================================================
//	Common Includes
//==============================================================================
#include "Resource.h"

//==============================================================================
//	OpenGL Includes
//==============================================================================
#include <GL/gl.h>

#define UIC_LITTLE_ENDIAN

#define SAFE_DELETE(ptr)                                                                           \
    if ((ptr) != nullptr) {                                                                           \
        delete (ptr);                                                                              \
        (ptr) = nullptr;                                                                              \
    }
#endif
