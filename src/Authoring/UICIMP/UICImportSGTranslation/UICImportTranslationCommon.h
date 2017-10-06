/****************************************************************************
**
** Copyright (C) 1999-2009 NVIDIA Corporation.
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
#include <vector>
#include <string>

namespace UICIMP {

typedef std::vector<long> TLongsList;
typedef std::vector<float> TFloatsList;
typedef std::vector<std::string> TStringList;

static const char *ST[] = { "S", "T" };
static const char *XY[] = { "X", "Y" };
static const char *XYZ[] = { "X", "Y", "Z" };
static const char *XYZANGLE[] = { "X", "Y", "Z", "ANGLE" };
static const char *RGBA[] = { "R", "G", "B", "A" };
static const char *TIME[] = { "TIME" };
static const char *SINGLEFLOAT[] = { "F" };

static const TStringList g_STIdentifiers(ST, ST + sizeof(ST) / sizeof(*ST));
static const TStringList g_XYIdentifiers(XY, XY + sizeof(XY) / sizeof(*XY));
static const TStringList g_XYZIdentifiers(XYZ, XYZ + sizeof(XYZ) / sizeof(*XYZ));
static const TStringList g_XYZANGLEIdentifiers(XYZANGLE,
                                               XYZANGLE + sizeof(XYZANGLE) / sizeof(*XYZANGLE));
static const TStringList g_RGBAIdentifiers(RGBA, RGBA + sizeof(RGBA) / sizeof(*RGBA));
static const TStringList g_TIMEIdentifiers(TIME, TIME + sizeof(TIME) / sizeof(*TIME));
static const TStringList g_FLOATIdentifiers(SINGLEFLOAT, SINGLEFLOAT
                                                + sizeof(SINGLEFLOAT) / sizeof(*SINGLEFLOAT));
}