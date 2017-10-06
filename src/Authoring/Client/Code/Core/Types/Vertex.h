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
#ifndef __VERTEX_H_
#define __VERTEX_H_

//==============================================================================
//	Includes
//==============================================================================
#include "Vector2.h"
#include "Vector3.h"

//==============================================================================
//	Class
//==============================================================================
//==============================================================================
/**
 *		@class CVertex: A simple, agnostic vertex data type (V3, N3, T2).
 */
class CVertex
{
public:
    Q3DStudio::CVector3 m_Position; ///< x, y, and z position values
    Q3DStudio::CVector3 m_Normal; ///< nx, ny, and nz components of the vertex normal
    Q3DStudio::CVector3 m_Tangent; ///< tangent
    Q3DStudio::CVector2 m_Texture; ///< u and v texture coordinates

    // Construction
public:
    CVertex(const CVertex &inVertex)
        : m_Position(inVertex.m_Position)
        , m_Normal(inVertex.m_Normal)
        , m_Tangent(inVertex.m_Tangent)
        , m_Texture(inVertex.m_Texture)
    {
    }
    CVertex(float inX, float inY, float inZ, float inU = 0.0f, float inV = 0.0f, float inNX = 0.0f,
            float inNY = 0.0f, float inNZ = 0.0f, float inTX = 0.0f, float inTY = 0.0f,
            float inTZ = 0.0f)
        : m_Position(inX, inY, inZ)
        , m_Normal(inNX, inNY, inNZ)
        , m_Tangent(inTX, inTY, inTZ)
        , m_Texture(inU, inV)
    {
    }
    CVertex(){}

    // Opertators
public:
    CVertex &operator=(const CVertex &inVertex)
    {
        // Check for self assignment
        if (&inVertex != this) {
            m_Position = inVertex.m_Position;
            m_Normal = inVertex.m_Normal;
            m_Texture = inVertex.m_Texture;
            m_Tangent = inVertex.m_Tangent;
        }

        return *this;
    }

    bool operator==(const CVertex &inVertex) const
    {
        return m_Position == inVertex.m_Position && m_Normal == inVertex.m_Normal
            && m_Tangent == inVertex.m_Tangent && m_Texture == inVertex.m_Texture;
    }
};

#endif //__VERTEX_H_