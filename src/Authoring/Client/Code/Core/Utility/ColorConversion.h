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
#ifndef __CCOLORCONVERSION_H_
#define __CCOLORCONVERSION_H_

class CColorConversion
{
public:
    // Operations
    static unsigned char GetRed(long inRGBA);
    static unsigned char GetGreen(long inRGBA);
    static unsigned char GetBlue(long inRGBA);
    static unsigned char GetAlpha(long inRGBA);
    static long MakeRGBA(float inRed, float inGreen, float inBlue, float inAlpha = 1.0f);
    static long MakeRGBA(long inRed, long inGreen, long inBlue, long inAlpha = 255);
};

#endif // __CCOLORCONVERSION_H_