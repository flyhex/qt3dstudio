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
#ifndef __CCOLOR_H_
#define __CCOLOR_H_

//==============================================================================
//	Includes
//==============================================================================
#include "UICString.h"
#include "UICObjectCounter.h"

namespace Q3DStudio {

//==============================================================================
//	Class
//==============================================================================
//==============================================================================
/**
 *	@class	CColor
 *	@brief	A color class, giving you control over RGB and A.
 */
class CColor
{
    //==============================================================================
    //	Constants
    //==============================================================================

protected:
    const float COLORNORMALIZER;

    //==============================================================================
    //	Field Members
    //==============================================================================
public:
    float r; ///< Red component of color
    float g; ///< Green componenct of color
    float b; ///< Blue componenct of color
    float a; ///< Alpha componenct of color

    DEFINE_OBJECT_COUNTER(CColor)

    //==============================================================================
    //	Member Fucntions
    //==============================================================================

    // Construction
public:
    CColor();
    CColor(float inRed, float inGreen, float inBlue, float inAlpha = 1.0f);
    CColor(const CColor &inColor);
    CColor(long inColorLong);
    ~CColor();

    // Operators
public:
    CColor &operator=(const CColor &inColor);

    // Data access
public:
    // Values between 1,0
    void SetR(float inR);
    void SetG(float inG);
    void SetB(float inB);
    void SetA(float inA);

    float GetR() const;
    float GetG() const;
    float GetB() const;
    float GetA() const;

    // Values between 255, 0
    void SetR255(long inR);
    void SetG255(long inG);
    void SetB255(long inB);
    void SetA255(long inA);

    long GetR255() const;
    long GetG255() const;
    long GetB255() const;
    long GetA255() const;

    void Set(float inRed, float inGreen, float inBlue, float inAlpha = 1.0f);
    void Set255(long inRed, long inGreen, long inBlue, long inAlpha = 255.0f);
    void Get255(long &outRed, long &outGreen, long &outBlue, long &outAlpha);
    void Get(float &outRed, float &outGreen, float &outBlue, float &outAlpha);
    void SetRGB(long inRGB);
    void GetRGB(long &outRGB);
    void SetRGB(unsigned char inRed, unsigned char inGreen, unsigned char inBlue,
                unsigned char inAlpha = 255);
    void GetRGB(unsigned char &outRed, unsigned char &outGreen, unsigned char &outBlue,
                unsigned char &outAlpha);
    void ToFloat(float *outColor)
    {
        outColor[0] = r;
        outColor[1] = g;
        outColor[2] = b;
        outColor[3] = a;
    }

    Q3DStudio::CString toString();
    void fromString(const Q3DStudio::CString &inStringValue);

    // Operations
public:
    void InterploateLinear(const CColor &inColor, float inWeight);
    void Blend(const CColor &inColor);
    void Subtract(const CColor &inColor);
    void Add(const CColor &inColor);
    void AdjustSaturation(float inSaturation);
    void AdjustContrast(float inContrast);
    void Scale(float inScaleFactor);
    void Invert();
    bool operator==(const CColor &inOther) const;
};

} // namespace Q3DStudio

#endif // __CCOLOR_H_
