/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#pragma once
#ifndef UIC_RENDER_CLIPPING_PLANE_H
#define UIC_RENDER_CLIPPING_PLANE_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSPlane.h"
#include "foundation/Qt3DSFlags.h"
#include "foundation/Qt3DSBounds3.h"

namespace qt3ds {
namespace render {

    struct BoxEdgeFlagValues
    {
        enum Enum {
            xMax = 1,
            yMax = 1 << 1,
            zMax = 1 << 2,
        };
    };

    typedef NVFlags<BoxEdgeFlagValues::Enum, QT3DSU8> TRenderBoxEdge;

    // For an intesection test, we only need two points of the bounding box.
    // There will be a point nearest to the plane, and a point furthest from the plane.
    // We can derive these points from the plane normal equation.
    struct SPlaneBoxEdge
    {
        TRenderBoxEdge lowerEdge;
        TRenderBoxEdge upperEdge;
    };

    struct SClipPlane
    {
        QT3DSVec3 normal;
        QT3DSF32 d;
        SPlaneBoxEdge mEdges;

        // For intersection tests, we only need to know if the numerator is greater than, equal to,
        // or less than zero.
        inline QT3DSF32 distance(const QT3DSVec3 &pt) const { return normal.dot(pt) + d; }

        // Only works if p0 is above the line and p1 is below the plane.
        inline QT3DSVec3 intersectWithLine(const QT3DSVec3 &p0, const QT3DSVec3 &p1) const
        {
            QT3DSVec3 dir = p1 - p0;
            QT3DSVec3 pointOnPlane = normal * (-d);
#ifdef _DEBUG
            QT3DSF32 distanceOfPoint = distance(pointOnPlane);
            QT3DS_ASSERT(NVAbs(distanceOfPoint) < 0.0001f);
#endif
            QT3DSF32 numerator = (pointOnPlane - p0).dot(normal);
            QT3DSF32 denominator = dir.dot(normal);

            QT3DS_ASSERT(NVAbs(denominator) > .0001f);
            QT3DSF32 t = (numerator / denominator);
            QT3DSVec3 retval = p0 + dir * t;
#ifdef _DEBUG
            QT3DSF32 retvalDistance = distance(retval);
            QT3DS_ASSERT(NVAbs(retvalDistance) < .0001f);
#endif
            return retval;
        }

        static inline QT3DSVec3 corner(const NVBounds3 &bounds, TRenderBoxEdge edge)
        {
            return QT3DSVec3((edge & BoxEdgeFlagValues::xMax) ? bounds.maximum[0] : bounds.minimum[0],
                          (edge & BoxEdgeFlagValues::yMax) ? bounds.maximum[1] : bounds.minimum[1],
                          (edge & BoxEdgeFlagValues::zMax) ? bounds.maximum[2] : bounds.minimum[2]);
        }

        // dividing the distance numerator

        // I got this code from osg, but it is in graphics gems
        // as well.
        /** intersection test between plane and bounding sphere.
        return 1 if the bs is completely above plane,
        return 0 if the bs intersects the plane,
        return -1 if the bs is completely below the plane.*/
        inline int intersect(const NVBounds3 &bounds) const
        {
            // if lowest point above plane than all above.
            if (distance(corner(bounds, mEdges.lowerEdge)) > 0.0f)
                return 1;

            // if highest point is below plane then all below.
            if (distance(corner(bounds, mEdges.upperEdge)) < 0.0f)
                return -1;

            // d_lower<=0.0f && d_upper>=0.0f
            // therefore must be crossing plane.
            return 0;
        }

        inline void calculateBBoxEdges()
        {
            mEdges.upperEdge = TRenderBoxEdge(
                static_cast<QT3DSU8>((normal[0] >= 0.0f ? BoxEdgeFlagValues::xMax : 0)
                                  | (normal[1] >= 0.0f ? BoxEdgeFlagValues::yMax : 0)
                                  | (normal[2] >= 0.0f ? BoxEdgeFlagValues::zMax : 0)));

            mEdges.lowerEdge = TRenderBoxEdge((~((QT3DSU8)mEdges.upperEdge)) & 7);
        }
    };

    struct SClippingFrustum
    {
        SClipPlane mPlanes[6];

        SClippingFrustum() {}

        SClippingFrustum(const QT3DSMat44 &modelviewprojection, SClipPlane nearPlane);

        bool intersectsWith(const NVBounds3 &bounds) const
        {
            for (QT3DSU32 idx = 0; idx < 6; ++idx)
                if (mPlanes[idx].intersect(bounds) < 0)
                    return false;
            return true;
        }

        bool intersectsWith(const QT3DSVec3 &point, QT3DSF32 radius = 0.0f) const
        {
            for (QT3DSU32 idx = 0; idx < 6; ++idx)
                if (mPlanes[idx].distance(point) < radius)
                    return false;
            return true;
        }
    };
}
}

#endif