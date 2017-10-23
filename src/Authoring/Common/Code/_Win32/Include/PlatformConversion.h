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

#ifndef __PLATFORMCONVERSION_H__
#define __PLATFORMCONVERSION_H__

//==============================================================================
//	This is the WIN32 file for converting platform-specific types to
//	native Studio types and visa-versa.
//==============================================================================

#ifdef WIN32

//==============================================================================
//	Includes
//==============================================================================

#include "Qt3DSRectBase.h"
#include "Qt3DSPointBase.h"

#ifndef __PLATFORMTYPES_H__
#include "PlatformTypes.h"
#endif

//==============================================================================
//	Class
//==============================================================================

//==============================================================================
/**
 *	@class	CPlatformConversion
 *	This utility class handles conversions between platform specific data types.
 */
//==============================================================================
class CPlatformConversion
{
public:
    //==============================================================================
    /**
     *	Convert an Qt3DSRect to a Q3DStudio::CRect
     */
    //==============================================================================
    static void Qt3DSRectToCRect(const Qt3DSRect &inPlatform, Q3DStudio::CRectBase &outRect)
    {
        outRect.left = inPlatform.left;
        outRect.top = inPlatform.top;
        outRect.right = inPlatform.right;
        outRect.bottom = inPlatform.bottom;
    }

    //==============================================================================
    /**
     *	Convert a Q3DStudio::CRect to an Qt3DSRect
     */
    //==============================================================================
    static void CRectToQt3DSRect(const Q3DStudio::CRectBase &inRect, Qt3DSRect &outPlatform)
    {
        outPlatform.left = inRect.left;
        outPlatform.top = inRect.top;
        outPlatform.right = inRect.right;
        outPlatform.bottom = inRect.bottom;
    }

    //==============================================================================
    /**
     *	Convert an Qt3DSPoint to an Q3DStudio::CPoint
     */
    //==============================================================================
    static void Qt3DSPointToCPoint(const Qt3DSPoint &inPlatform, Q3DStudio::CPointBase &outPoint)
    {
        outPoint.x = inPlatform.x;
        outPoint.y = inPlatform.y;
    }

    //==============================================================================
    /**
     *	Convert a Q3DStudio::CPoint to an Qt3DSPoint
     */
    //==============================================================================
    static void CPointToQt3DSPoint(const Q3DStudio::CPointBase &inPoint, Qt3DSPoint &outPlatform)
    {
        outPlatform.x = inPoint.x;
        outPlatform.y = inPoint.y;
    }
};

#endif // WIN32

#endif // __PLATFORMCONVERSION_H__
