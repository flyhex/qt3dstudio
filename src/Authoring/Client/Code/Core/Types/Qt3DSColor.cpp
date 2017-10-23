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
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSColor.h"
#include <stdio.h>
#include "ColorConversion.h"

#define CLAMP_COLOR(x, y, z)                                                                       \
    if (x > y)                                                                                     \
        x = y;                                                                                     \
    if (x < z)                                                                                     \
        x = z;

namespace Q3DStudio {

IMPLEMENT_OBJECT_COUNTER(CColor)

//==============================================================================
/**
 *	Constructs a CColor
 */
CColor::CColor()
    : COLORNORMALIZER(255.0f)
    , r(0.0f)
    , g(0.0f)
    , b(0.0f)
    , a(1.0f)
{
    ADDTO_OBJECT_COUNTER(CColor)
}

//==============================================================================
/**
 *	Constructs a CColor
 */
CColor::CColor(float inRed, float inGreen, float inBlue, float inAlpha)
    : COLORNORMALIZER(255.0f)
    , r(inRed)
    , g(inGreen)
    , b(inBlue)
    , a(inAlpha)
{
    ADDTO_OBJECT_COUNTER(CColor)
}

//==============================================================================
/**
 *	Constructs a copy of the class.
 */
CColor::CColor(const CColor &inColor)
    : COLORNORMALIZER(255.0f)
    , r(inColor.r)
    , g(inColor.g)
    , b(inColor.b)
    , a(inColor.a)
{
    ADDTO_OBJECT_COUNTER(CColor)
}

//==============================================================================
/**
 *	Constructs based on a long that represent the color in RGB
 */
CColor::CColor(long inColorLong)
    : COLORNORMALIZER(255.0f)
    , a(1.0f)
{
    ADDTO_OBJECT_COUNTER(CColor)
    SetRGB(inColorLong);
}

//==============================================================================
/**
 *	Destroys a CColor
 */
CColor::~CColor()
{
    REMOVEFROM_OBJECT_COUNTER(CColor)
}

//==============================================================================
/**
 *	Handles the assigment of two CColor's.
 */
CColor &CColor::operator=(const CColor &inColor)
{
    if (&inColor != this) {
        r = inColor.r;
        g = inColor.g;
        b = inColor.b;
        a = inColor.a;
    }

    return *this;
}

void CColor::SetR(float inR)
{
    r = inR;
}
void CColor::SetG(float inG)
{
    g = inG;
}
void CColor::SetB(float inB)
{
    b = inB;
}
void CColor::SetA(float inA)
{
    a = inA;
}

float CColor::GetR() const
{
    return r;
}
float CColor::GetG() const
{
    return g;
}
float CColor::GetB() const
{
    return b;
}
float CColor::GetA() const
{
    return a;
}

void CColor::SetR255(long inR)
{
    r = inR / COLORNORMALIZER;
    CLAMP_COLOR(r, COLORNORMALIZER, 0);
}

void CColor::SetG255(long inG)
{
    g = inG / COLORNORMALIZER;
    CLAMP_COLOR(g, COLORNORMALIZER, 0);
}

void CColor::SetB255(long inB)
{
    b = inB / COLORNORMALIZER;
    CLAMP_COLOR(b, COLORNORMALIZER, 0);
}

void CColor::SetA255(long inA)
{
    a = inA / COLORNORMALIZER;
    CLAMP_COLOR(a, COLORNORMALIZER, 0);
}

long CColor::GetR255() const
{
    // Add .5 to combat the rounding off issue
    long temp = (long)((r * COLORNORMALIZER) + .5);
    CLAMP_COLOR(temp, (long)COLORNORMALIZER, 0);
    return temp;
}

long CColor::GetG255() const
{
    // Add .5 to combat the rounding off issue
    long temp = (long)((g * COLORNORMALIZER) + .5);
    CLAMP_COLOR(temp, (long)COLORNORMALIZER, 0);
    return temp;
}

long CColor::GetB255() const
{
    // Add .5 to combat the rounding off issue
    long temp = (long)((b * COLORNORMALIZER) + .5);
    CLAMP_COLOR(temp, (long)COLORNORMALIZER, 0);
    return temp;
}

long CColor::GetA255() const
{
    // Add .5 to combat the rounding off issue
    long temp = (long)((a * COLORNORMALIZER) + .5);
    CLAMP_COLOR(temp, (long)COLORNORMALIZER, 0);
    return temp;
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
void CColor::Set(float inRed, float inGreen, float inBlue, float inAlpha)
{
    r = inRed;
    CLAMP_COLOR(r, 1.f, 0.f);

    g = inGreen;
    CLAMP_COLOR(g, 1.f, 0.f);

    b = inBlue;
    CLAMP_COLOR(b, 1.f, 0.f);

    a = inAlpha;
    CLAMP_COLOR(a, 1.f, 0.f);
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
void CColor::Set255(long inRed, long inGreen, long inBlue, long inAlpha)
{
    SetR255(inRed);
    SetG255(inGreen);
    SetB255(inBlue);
    SetA255(inAlpha);
}

//==============================================================================
/**
 *	Gets the RGBA components of the color.
 *
 *	@param outRed The red component of the color.
 *	@param outGreen The green component of the color.
 *	@param outBlue The blue component of the color.
 *	@param outAlpha The alpha component of the color.
 */
void CColor::Get255(long &outRed, long &outGreen, long &outBlue, long &outAlpha)
{
    outRed = GetR255();
    outGreen = GetG255();
    outBlue = GetB255();
    outAlpha = GetA255();
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
void CColor::Get(float &outRed, float &outGreen, float &outBlue, float &outAlpha)
{
    outRed = r;
    outGreen = g;
    outBlue = b;
    outAlpha = a;
}

//==============================================================================
/**
 *	Sets the color value formatted as a D3DRGB type.
 *
 *	@param	inRGB	A long composed of RGB values.
 */
void CColor::SetRGB(long inRGB)
{
    r = static_cast<float>(CColorConversion::GetRed(inRGB)) / COLORNORMALIZER;
    CLAMP_COLOR(r, 1.f, 0.f);

    g = static_cast<float>(CColorConversion::GetGreen(inRGB)) / COLORNORMALIZER;
    CLAMP_COLOR(g, 1.f, 0.f);

    b = static_cast<float>(CColorConversion::GetBlue(inRGB)) / COLORNORMALIZER;
    CLAMP_COLOR(b, 1.f, 0.f);
}

//==============================================================================
/**
 *	GetRGB: Returns the color value formatted as a D3DRGB type.
 *
 *	@param	inRGB	A long composed of RGB values.
 */
void CColor::GetRGB(long &outRGB)
{
    outRGB = CColorConversion::MakeRGBA(r, g, b, a);
}

//==============================================================================
/**
 *	SetRGB:	Set the color from short char between 0 and 255
 */
void CColor::SetRGB(unsigned char inRed, unsigned char inGreen, unsigned char inBlue,
                    unsigned char inAlpha)
{
    r = inRed / COLORNORMALIZER;
    g = inGreen / COLORNORMALIZER;
    b = inBlue / COLORNORMALIZER;
    a = inAlpha / COLORNORMALIZER;
}

//==============================================================================
/**
 *	GetRGB:	Returns the color as char values between 0 and 255.
 */
void CColor::GetRGB(unsigned char &outRed, unsigned char &outGreen, unsigned char &outBlue,
                    unsigned char &outAlpha)
{
    outRed = static_cast<unsigned char>(r * COLORNORMALIZER);
    outGreen = static_cast<unsigned char>(g * COLORNORMALIZER);
    outBlue = static_cast<unsigned char>(b * COLORNORMALIZER);
    outAlpha = static_cast<unsigned char>(a * COLORNORMALIZER);
}

//==============================================================================
/**
 *	Changes the color value to be its negative.
 *
 *	Each color component is a modified by subtracting 1.0f from its value
 *	thus producing the color negative.
 */
void CColor::Invert()
{
    r = static_cast<float>(1.0f - r);
    g = static_cast<float>(1.0f - g);
    b = static_cast<float>(1.0f - b);
    a = static_cast<float>(1.0f - a);
}

//==============================================================================
/**
 *	Scales the color value by the specifed factor.
 *
 *	Each color component is a multiplied by the given factor thus producing
 *	a scaled version of the original color. If the scale factor is 1.0f, then
 *	the result is the original color.
 *
 *	@param inScaleFactor The scale factor by which each color component is
 *	multiplied. If the value is 1.0f, the color will not be changed.
 *
 *	@return HRESULT Always returns S_OK.
 */
void CColor::Scale(float inScaleFactor)
{
    r *= inScaleFactor;
    CLAMP_COLOR(r, 1.f, 0.f);

    g *= inScaleFactor;
    CLAMP_COLOR(g, 1.f, 0.f);

    b *= inScaleFactor;
    CLAMP_COLOR(b, 1.f, 0.f);

    a *= inScaleFactor;
    CLAMP_COLOR(a, 1.f, 0.f);
}

//==============================================================================
/**
 *	Changes the contrast of the color value.
 *
 *	The RGB components of the color are modified by linearly interpolating
 *	the component between 50% gray and its current value using the following
 *	formula:
 *
 *	new = 0.5f + inContrast * (current - 0.5f)
 *
 *	Where 'inContrast' is the weighting factor of the interpolation. If
 *	'inContrast' is 0.0f, then the color becomes 50% gray. If it is 1.0f,
 *	then the new color is the same as the original. If it is greater than
 *	1.0f, then the contrast is increased.
 *
 *	@param inContrast The weighing factor for altering the color's contrast.
 */
void CColor::AdjustContrast(float inContrast)
{
    r = static_cast<float>(0.5f + inContrast * (r - 0.5f));
    g = static_cast<float>(0.5f + inContrast * (g - 0.5f));
    b = static_cast<float>(0.5f + inContrast * (b - 0.5f));
}

//==============================================================================
/**
 *	AdjustSaturation: Changes the saturation of the color value.
 *
 *	The RGB components of the color are modified by linearly interpolating
 *	the component between an unsaturated value and its current value using
 *	the following formula:
 *
 *	 Approximate values for each component's contribution to luminance.<BR>
 *	 Based upon the NTSC standard described in ITU-R Recommendation BT.709.<BR>
 *	grey = current.red * 0.2125f + current.green * 0.7154f + current.blue * 0.0721f
 *	new = grey + inSaturation * (current - grey)
 *
 *	Where 'inSaturation' is the weighting factor of the interpolation. If
 *	'inSaturation' is 0.0f, then the color becomes grayscale. If it is 1.0f,
 *	then the new color is the same as the original. If it is greater than 1.0f,
 *	then the saturation is increased.
 *
 *	@param inSaturation The weighting factor for altering the color's saturation.
 */
void CColor::AdjustSaturation(float inSaturation)
{
    float theGrey = static_cast<float>(r * 0.2125f + g * 0.7154f + b * 0.0721f);

    r = theGrey + inSaturation * (r - theGrey);
    CLAMP_COLOR(r, 1.f, 0.f);

    g = theGrey + inSaturation * (g - theGrey);
    CLAMP_COLOR(g, 1.f, 0.f);

    b = theGrey + inSaturation * (b - theGrey);
    CLAMP_COLOR(b, 1.f, 0.f);
}

//==============================================================================
/**
 *	Adds the specified color to the current color.
 *
 *	The RGBA components of 'inColor' are added to the components of the
 *	current color.
 *
 *	@param inColor The color to add to the current color.
 */
void CColor::Add(const CColor &inColor)
{
    r += inColor.r;
    CLAMP_COLOR(r, 1.f, 0.f);

    g += inColor.g;
    CLAMP_COLOR(g, 1.f, 0.f);

    b += inColor.b;
    CLAMP_COLOR(b, 1.f, 0.f);

    a += inColor.a;
    CLAMP_COLOR(a, 1.f, 0.f);
}

//==============================================================================
/**
 *	Subtracts the specified color from the current color.
 *
 *	The RGBA components of 'inColor' are subtracted from the components of the
 *	current color.
 *
 *	@param inColor The color to subtract from the current color.
 */
void CColor::Subtract(const CColor &inColor)
{
    r -= inColor.r;
    CLAMP_COLOR(r, 1.f, 0.f);

    g -= inColor.g;
    CLAMP_COLOR(g, 1.f, 0.f);

    b -= inColor.b;
    CLAMP_COLOR(b, 1.f, 0.f);

    a -= inColor.a;
    CLAMP_COLOR(a, 1.f, 0.f);
}

//==============================================================================
/**
 *	Blends the specified color into the current color.
 *
 *	The RGBA components of 'inColor' are blended into the components of the
 *	current color by multiplying each component together.
 *
 *	@param inColor The color to blend into the current color.
 */
void CColor::Blend(const CColor &inColor)
{
    r *= inColor.r;
    CLAMP_COLOR(r, 1.f, 0.f);

    g *= inColor.g;
    CLAMP_COLOR(g, 1.f, 0.f);

    b *= inColor.b;
    CLAMP_COLOR(b, 1.f, 0.f);

    a *= inColor.a;
    CLAMP_COLOR(a, 1.f, 0.f);
}

//==============================================================================
/**
 *	Performs a linear interpolation between the current
 *	color and the specified	color, treating both as 4-D vectors.
 *
 *	The current color acts as the source of the interpolation and 'inColor'
 *	acts as the target. The interpolation of each component uses the following
 *	formula:
 *
 *	new = current + inWeight * (inColor - current)
 *
 *	Where 'inWeight' is the weighting factor for the interpolation. If 'inWeight'
 *	is 0.0f, then the color remains the current color. If it is 1.0f, then the
 *	new color is the same as 'inColor'.
 *
 *	@param inColor The target color of the interpolation.
 *	@param inWeight The weighting factor for the interpolation.
 */
void CColor::InterploateLinear(const CColor &inColor, float inWeight)
{
    r = inColor.r + inWeight * (r - inColor.r);
    CLAMP_COLOR(r, 1.f, 0.f);

    g = inColor.g + inWeight * (g - inColor.g);
    CLAMP_COLOR(g, 1.f, 0.f);

    b = inColor.b + inWeight * (b - inColor.b);
    CLAMP_COLOR(b, 1.f, 0.f);

    a = inColor.a + inWeight * (a - inColor.a);
    CLAMP_COLOR(a, 1.f, 0.f);
}

//==============================================================================
/**
 *	Transforms the current Color into a string
 *
 *	This method takes the current values of the Color and returns them
 *	in a string in the format R G B A
 */
Q3DStudio::CString CColor::toString()
{
    Q3DStudio::CString theStringBuffer;
    unsigned char theRed;
    unsigned char theGreen;
    unsigned char theBlue;
    unsigned char theAlpha;

    GetRGB(theRed, theGreen, theBlue, theAlpha);
    theStringBuffer.Format(_LSTR("%.3hd %.3hd %.3hd %.3hd"), theRed, theGreen, theBlue, theAlpha);

    return theStringBuffer;
}

bool CColor::operator==(const CColor &inOther) const
{
    return r == inOther.r && g == inOther.g && b == inOther.b && a == inOther.a;
}

//==============================================================================
/**
 *		Takes the values of the string and sets the Color to those	values.
 *
 *		This method takes the values of the string and stores them in this
 *		Color.  The string must be in the format of R G B A
 */
void CColor::fromString(const Q3DStudio::CString &inStringValue)
{
    short theRed;
    short theGreen;
    short theBlue;
    short theAlpha;

    ::sscanf(inStringValue.GetCharStar(), "%hd %hd %hd %hd", &theRed, &theGreen, &theBlue,
             &theAlpha);
    SetRGB(static_cast<unsigned char>(theRed), static_cast<unsigned char>(theGreen),
           static_cast<unsigned char>(theBlue), static_cast<unsigned char>(theAlpha));
}

} // namespace Q3DStudio
