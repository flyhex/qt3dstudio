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
#include "BoundingBox.h"
#include "OptimizedArithmetic.h"

namespace Q3DStudio {

/**
 *	Empty Constructor.  An empty box is a maximized inverted box which
 *	ensures that an expand with an empty box is a no-op.  If an empty box was
 *	a point at 0,0,0 an expand would expand a box to always include 0,0,0 which
 *	would be incorrect if the box was offset and did not previously include 0,0,0.
 */
CBoundingBox::CBoundingBox()
    : m_MinCorner(FLT_MAX, FLT_MAX, FLT_MAX)
    , m_MaxCorner(-FLT_MAX, -FLT_MAX, -FLT_MAX)
    , m_Orthographic(false)
{
}

/**
 *	Constructor using two points in space.  The created box is guaranteed to
 *	have correct min/max corners even if the given corners are not correct.
 *
 *	@param inCorner1 is one spanning corner of the box
 *	@param inCorner2 is the other spanning corner of the box
 */
CBoundingBox::CBoundingBox(const CVector3 &inCorner1, const CVector3 &inCorner2)
    : m_MinCorner(inCorner1)
    , m_MaxCorner(inCorner2)
    , m_Orthographic(false)
{
    m_MinCorner.Minimize(inCorner2);
    m_MaxCorner.Maximize(inCorner1);
}

/**
 *	Copy Constructor.
 *
 *	@param inBox is the source box
 */
CBoundingBox::CBoundingBox(const CBoundingBox &inBox)
    : m_MinCorner(inBox.m_MinCorner)
    , m_MaxCorner(inBox.m_MaxCorner)
    , m_Orthographic(inBox.m_Orthographic)
{
}

/**
 *	Reset bounding box to be empty which is different than the box having
 *	both corners at 0,0,0.
 */
void CBoundingBox::Clear()
{
    m_MinCorner.Set(FLT_MAX, FLT_MAX, FLT_MAX);
    m_MaxCorner.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    m_Orthographic = false;
}

/**
 *	Query box for emptyness.  Empty means not having any points at all and
 *	is different from a box with both corners at 0,0,0.  An empty box is
 *	guaranteed not ever to expand another box.
 */
bool CBoundingBox::IsEmpty() const
{
    return m_MinCorner == CVector3(FLT_MAX, FLT_MAX, FLT_MAX)
        && m_MaxCorner == CVector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}

/**
 *	Expands this box to represent a volume capable of holding both the old
 *	box and the box given as an argument.
 *
 *	@param inBox is the box that may expand the current box
 *	@return this box which is the resulting expanded box
 */
CBoundingBox &CBoundingBox::Expand(const CBoundingBox &inBox)
{
    m_MinCorner.Minimize(inBox.m_MinCorner);
    m_MaxCorner.Maximize(inBox.m_MaxCorner);

    return *this;
}

/**
 *	Expands this box to represent a volume capable of including the incoming point.
 *
 *	@param inPoint is the vertex which needs to be contained within this box
 *	@return this box which is the resulting expanded box
 */
CBoundingBox &CBoundingBox::Expand(const CVector3 &inPoint)
{
    m_MinCorner.Minimize(inPoint);
    m_MaxCorner.Maximize(inPoint);

    return *this;
}

/**
 *	Get the max corner of the box.
 *
 *	@return the max point of the box
 */
const CVector3 &CBoundingBox::MaxPoint() const
{
    return m_MaxCorner;
}

/**
 *	Get the min corner of the box.
 *
 *	@return the min point of the box
 */
const CVector3 &CBoundingBox::MinPoint() const
{
    return m_MinCorner;
}

/**
 *	Transforms the bounding box using the given matrix.  The resulting box
 *	is guaranteed to keep the min/max corners min/maxed even of the
 *	applied matrix results in a rotation that flips one or more corners.
 *	Note that each corner has to be translated individually and all
 *	eight corners have to be processed to spen the new box.
 *
 *	@param inTransform is the matrix that will apply the transform
 *	@return this box which is rotated, scaled or modified by the given matrix
 */
CBoundingBox &CBoundingBox::Transform(const CMatrix &inTransform)
{
    // Don't mess with an empty box since it may make it not empty
    if (!IsEmpty())
        COptimizedArithmetic::TransformBoundingBox(&m_MinCorner.x, &m_MaxCorner.x,
                                                   inTransform.m[0]);

    return *this;
}

/**
 *	Get the min corner of the box.
 *
 *	@return the min point of the box
 */
void CBoundingBox::GetCorners(CVector3 outCorners[8]) const
{
    outCorners[0] = m_MinCorner;

    outCorners[1].x = m_MaxCorner.x;
    outCorners[1].y = m_MinCorner.y;
    outCorners[1].z = m_MinCorner.z;

    outCorners[2].x = m_MinCorner.x;
    outCorners[2].y = m_MaxCorner.y;
    outCorners[2].z = m_MinCorner.z;

    outCorners[3].x = m_MaxCorner.x;
    outCorners[3].y = m_MaxCorner.y;
    outCorners[3].z = m_MinCorner.z;

    outCorners[4].x = m_MinCorner.x;
    outCorners[4].y = m_MinCorner.y;
    outCorners[4].z = m_MaxCorner.z;

    outCorners[5].x = m_MaxCorner.x;
    outCorners[5].y = m_MinCorner.y;
    outCorners[5].z = m_MaxCorner.z;

    outCorners[6].x = m_MinCorner.x;
    outCorners[6].y = m_MaxCorner.y;
    outCorners[6].z = m_MaxCorner.z;

    outCorners[7] = m_MaxCorner;
}

/**
 *	Get whether or not this bounding box should be rendered with an
 *	orthographic camera.
 *	@return	true if the box should be rendered orthographically.
 */
bool CBoundingBox::GetOrthographic() const
{
    return m_Orthographic;
}

/**
 *	Set whether or not this bounding box should be rendered with an
 *	orthographic camera.
 *	@param	inOrthographic	true by default
 */
void CBoundingBox::SetOrthographic(const bool &inOrthographic)
{
    m_Orthographic = inOrthographic;
}

} // namespace Q3DStudio
