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

#include "Qt3DSCommonPrecompile.h"
#include "ColorConversion.h"

/**
*	Returns the red portion of the color
*
*	@param	inRGBA	A long composed of RGBA values.
*/
unsigned char CColorConversion::GetRed(long inRGBA)
{
    return static_cast<unsigned char>(0xFF & inRGBA);
}

//==============================================================================
/**
*	Returns the green portion of the color
*
*	@param	inRGBA	A long composed of RGBA values.
*/
unsigned char CColorConversion::GetGreen(long inRGBA)
{
    return static_cast<unsigned char>(0xFF & inRGBA >> 8);
}

//==============================================================================
/**
*	Returns the blue portion of the color
*
*	@param	inRGBA	A long composed of RGBA values.
*/
unsigned char CColorConversion::GetBlue(long inRGBA)
{
    return static_cast<unsigned char>(0xFF & inRGBA >> 16);
}

//==============================================================================
/**
*	Returns the alpha portion of the color
*
*	@param	inRGBA	A long composed of RGBA values.
*/
unsigned char CColorConversion::GetAlpha(long inRGBA)
{
    return static_cast<unsigned char>(0xFF & inRGBA >> 24);
}

//==============================================================================
/**
*	MakeRGB: Creates an RGBA long from float portions (0.0-1.0)
*
*	@param	inRed		Red portion
*	@param	inGreen		Green portion
*	@param	inBlue		Blue portion
*	@param	inAlpha		Alpha portion
*/
long CColorConversion::MakeRGBA(float inRed, float inGreen, float inBlue, float inAlpha)
{
    return MakeRGBA(static_cast<long>(inRed * 255), static_cast<long>(inGreen * 255),
                    static_cast<long>(inBlue * 255), static_cast<long>(inAlpha * 255));
}

//==============================================================================
/**
*	MakeRGBA: Creates an RGBA long from long portions (0-255)
*
*	@param	inRed		Red portion
*	@param	inGreen		Green portion
*	@param	inBlue		Blue portion
*	@param	inAlpha		Alpha portion
*/
long CColorConversion::MakeRGBA(long inRed, long inGreen, long inBlue, long inAlpha)
{
    return ((inAlpha << 24) | (inBlue << 16) | (inGreen << 8) | inRed);
}
