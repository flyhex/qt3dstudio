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

#ifndef __PLATFORMTYPES_H__
#define __PLATFORMTYPES_H__

//==============================================================================
//	This is the WIN32 file that defines the cross-platform data types used
//	by Studio. Whereever possible, these types are mapped directly to existing
//	Windows types.
//==============================================================================

#include <QtGlobal>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>

typedef RECT UICRect;
typedef POINT UICPoint;
typedef void *UICWindow;
typedef HWND UICRenderDevice;
typedef HCURSOR UICCursor;

typedef unsigned __int64 __uint64;
typedef unsigned long __uint32;
typedef unsigned char __uint8;

#else

typedef bool BOOL;
typedef int8_t BYTE;
typedef int16_t WORD;
typedef int32_t DWORD;
typedef int32_t LONG;
typedef int64_t __int64;
typedef int64_t LONGLONG;
typedef void* HANDLE;
typedef void* HMODULE;
typedef void* HRESULT;
typedef const char* LPCTSTR;
typedef char* LPCSTR;
typedef char* LPSTR;
#define UNREFERENCED_PARAMETER(s) Q_UNUSED(s)
#define TRUE true
#define FALSE false

typedef DWORD COLORREF;
typedef DWORD* LPCOLORREF;
#define GetRValue(rgb)     ((BYTE)(rgb & 0xFF))
#define GetGValue(rgb)      ((BYTE) ((rgb >> 8) & 0xFF ) )
#define GetBValue(rgb)      ((BYTE) ((rgb>>16) & 0xFF) )
#define RGB(r,g,b)      ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

struct GUID
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
};
typedef const GUID& REFGUID;

typedef void* UICWindow;
typedef void* UICRenderDevice;

typedef union _LARGE_INTEGER {
  struct {
    DWORD LowPart;
    LONG  HighPart;
  };
  struct {
    DWORD LowPart;
    LONG  HighPart;
  } u;
  LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef struct tagPIXELFORMATDESCRIPTOR {
  WORD  nSize;
  WORD  nVersion;
  DWORD dwFlags;
  BYTE  iPixelType;
  BYTE  cColorBits;
  BYTE  cRedBits;
  BYTE  cRedShift;
  BYTE  cGreenBits;
  BYTE  cGreenShift;
  BYTE  cBlueBits;
  BYTE  cBlueShift;
  BYTE  cAlphaBits;
  BYTE  cAlphaShift;
  BYTE  cAccumBits;
  BYTE  cAccumRedBits;
  BYTE  cAccumGreenBits;
  BYTE  cAccumBlueBits;
  BYTE  cAccumAlphaBits;
  BYTE  cDepthBits;
  BYTE  cStencilBits;
  BYTE  cAuxBuffers;
  BYTE  iLayerType;
  BYTE  bReserved;
  DWORD dwLayerMask;
  DWORD dwVisibleMask;
  DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR;

#endif // WIN32

#endif // __PLATFORMTYPES_H__
