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

#ifndef __PLATFORMSTRINGS_H__
#define __PLATFORMSTRINGS_H__

//==============================================================================
//	This is the WIN32 file that maps string functions with dissimilar
//	cross-platform names (thanks Microsoft) to Studio specific names (_a).
//==============================================================================

#ifdef WIN32

// ASCII string functions
#define _asnprintf ::_snprintf
#define _avsnprintf ::_vsnprintf

// UNICODE string functions
#define _aswprintf ::_snwprintf
#define _avswprintf ::_vsnwprintf
#define _awcstok(a, b, c)                                                                          \
    ::wcstok(a, b);                                                                                \
    c;

#endif // WIN32

#endif // __PLATFORMSTRINGS_H__
