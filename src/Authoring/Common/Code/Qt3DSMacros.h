/****************************************************************************
**
** Copyright (C) 2004 NVIDIA Corporation.
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
#pragma once

#include <stddef.h>

// ================ PLATFORM RECOGNITION ======================
// Defines WIN32 when running on Windows
#if defined(__WIN32__) && !defined(WIN32)
#define WIN32
#endif

// ================ DEBUG DETECTION ======================
#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#ifdef DEBUG
#define CHECK_BOUNDS // CHECK_BOUNDS affects the way an item is retrieved from container objects
//#  define USE_MEMORY_MANAGER		// uses the fluid_studios memmgr.h
#ifndef TRACK_OBJECT_COUNTS
#define TRACK_OBJECT_COUNTS
#endif
#endif

// Items to be removed before release
#define PERFORM_PROFILE // include the profiler in the build

// ====================== NULL ===========================
#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

//==============================================================================
//	UICTODO Development Macro
//	Usage:
//
//	"#pragma UICTODO(Message)" yields "__FILE__>(__LINE__) : To Do - <Message>"
//
//	in the Build Pane of the Output Window
//
//	For example:
//		#pragma UICTODO(SDJ, 11/12/00, Clean up this code) might result in
//		C:\Code\Project\File.cpp(20) : [ 11/12/00 ] SDJ - Clean up this code
//
//	Double-clicking this line opens the specified file to the specified line in
//	the VC++ IDE.
//
//	NOTE!!!  Only use when you know you will fix it the next check-in.  The
//	Last TODO macro was abused when people just left comments left and right.
//
//==============================================================================
#define chSTR(x) #x
#define chSTR2(x) chSTR(x)
#define UICTODO(author, date, description)                                                         \
    message(__FILE__ "(" chSTR2(__LINE__) "): [ " #date " ] " #author " - " #description)

template <typename T, size_t N>
char (&_ArraySizeHelper(T (&array)[N]))[N];
#define _arraysize(array) (sizeof(_ArraySizeHelper(array)))

// Old and disabled:
#define TODO(author, date, description)

//==============================================================================
/**
 *	Lambda-type macro that simplifies iteration over collections.
 *	@param	VAR	theVariable iterating across the collection
 *	@param	COL theCollection being iterated on
 *	@note	Copyright (c) 2003 Eric Niebler
 *	@waring	This macro forces us to turn off compiler warnings for:
 *			"warning C4706: assignment within conditional expression".
 *			This is unfortunate but the positive impact in readability
 *			makes this sacrifice worth it.
 *
 *  For iterating over collections. Collections can be arrays, null-terminated
 *	strings, or STL containers including std::string. The loop variable can be
 *	a value or reference.  For example: @code
std::list<long> theLongList( stuff );
FOR_EACH( long& theLong, theLongList )
{
    // *** loop body goes here ***
    // theLong is a read-write reference to each long in theLongList
} @endcode
 *  Alternately, you can declare the loop variable first, so you can access it
 *	after the loop finishes. Obviously, if you do it this way, then the loop
 *	variable cannot be a reference: @code
long theLong;
FOR_EACH( theLong, theLongList )
{ ... } @endcode
 */
#define FOR_EACH(VAR, COL)                                                                         \
    if (::std::for_each::static_any_t _for_each_cur = ::std::for_each::begin(COL, 0)) {        \
    } else if (::std::for_each::static_any_t _for_each_end = ::std::for_each::end(COL, 0)) {   \
    } else                                                                                         \
        for (bool _for_each_continue = true;                                                       \
             _for_each_continue && !::std::for_each::done(_for_each_cur, _for_each_end, COL, 0); \
             _for_each_continue ? ::std::for_each::next(_for_each_cur, COL, 0) : (void)&COL)     \
            if (_for_each_continue = false) {                                                      \
            } else                                                                                 \
                for (VAR = ::std::for_each::extract(_for_each_cur, COL, 0); !_for_each_continue; \
                     _for_each_continue = true)

#ifdef _WIN32
// See warning in method comment above, but not for Mac, Codewarrior fails the build process.
#pragma warning(disable : 4706) // "assignment within conditional expression"
#endif
