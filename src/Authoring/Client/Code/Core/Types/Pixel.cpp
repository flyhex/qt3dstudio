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
//	Includes
//==============================================================================
#include "stdafx.h"
#include "Pixel.h"

//==============================================================================
/**
 *	Constructs a CPixel
 */
CPixel::CPixel()
    : r(0)
    , g(0)
    , b(0)
    , a(0)
{
}
//==============================================================================
/**
 *	Constructs a CPixel
 */
CPixel::CPixel(const unsigned char &inRed, const unsigned char &inGreen,
               const unsigned char &inBlue, const unsigned char &inAlpha)
    : r(inRed)
    , g(inGreen)
    , b(inBlue)
    , a(inAlpha)
{
}

//==============================================================================
/**
 *	Destroys a CPixel
 */
CPixel::~CPixel()
{
}

//==============================================================================
/**
 *	Constructs a copy of the class.
 */
CPixel::CPixel(const CPixel &inPixel)
    : r(inPixel.r)
    , g(inPixel.g)
    , b(inPixel.b)
    , a(inPixel.a)
{
}

//==============================================================================
/**
 *	Handles the assigment of two CPixel's.
 */
CPixel &CPixel::operator=(const CPixel &inPixel)
{
    if (&inPixel != this) {
        r = inPixel.r;
        g = inPixel.g;
        b = inPixel.b;
        a = inPixel.a;
    }

    return *this;
}

//==============================================================================
/**
 *	Sets the RGBA components of the color.
 *
 *	Each color component is a floating point value between 0.0f (not present/
 *	transparent) and 1.0f (full saturation/opaque).
 *
 *	@param inRed The red component of the color.
 *	@param inGreen The green component of the color.
 *	@param inBlue The blue component of the color.
 *	@param inAlpha The alpha component of the color.
 */
void CPixel::Set(const unsigned char &inRed, const unsigned char &inGreen,
                 const unsigned char &inBlue, const unsigned char &inAlpha)
{
    r = inRed;
    g = inGreen;
    b = inBlue;
    a = inAlpha;
}

//==============================================================================
/**
 *	Gets the RGBA components of the color.
 *
 *	Each color component is a floating point value between 0.0f (not present/
 *	transparent) and 1.0f (full saturation/opaque).
 *
 *	@param outRed The red component of the color.
 *	@param outGreen The green component of the color.
 *	@param outBlue The blue component of the color.
 *	@param outAlpha The alpha component of the color.
 */
void CPixel::Get(unsigned char &outRed, unsigned char &outGreen, unsigned char &outBlue,
                 unsigned char &outAlpha)
{
    outRed = r;
    outGreen = g;
    outBlue = b;
    outAlpha = a;
}