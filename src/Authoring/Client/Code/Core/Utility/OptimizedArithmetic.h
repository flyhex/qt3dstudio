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
#pragma once

// tiny helper class to provide auto-initialization.
class COptArithInitializer
{
public:
    COptArithInitializer();
};

// Function definitions for the optimized functions. Documentation for them can be found on the
// specific implementations
// though the functionality should be the same between all implementations.
typedef void (*FMatrixMultiply)(const float *inSource1, const float *inSource2, float *outDest);
typedef bool (*FIntersectTriangle)(bool inUseFrontface, bool inUseBackface, const float *inOrig,
                                   const float *inDirection, const float *inTestV0,
                                   const float *inTestV1, const float *inTestV2,
                                   float *outIntersection, float *outIntersectOffset);
typedef void (*FTransformBoundingBox)(float *ioMinBox, float *ioMaxBox, const float *inTransform);

class COptimizedArithmetic
{
public:
    // Public accessors for the optimized functions.
    // useage: COptimizedArithmetic::MatrixMultiply( ... );
    static FMatrixMultiply MatrixMultiply;
    static FIntersectTriangle IntersectTriangle;
    static FTransformBoundingBox TransformBoundingBox;

protected:
    // static member variable used only for auto-initialization of this class.
    static COptArithInitializer m_Initializer;

private:
    COptimizedArithmetic();
    virtual ~COptimizedArithmetic();

public:
    static void Initialize();

    // getters for checking what capabilities are available on the platform.
    static bool HasMMX();
    static bool HasSSE();
    static bool HasSSE2();
    static bool Has3DNow();
};
