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

#ifndef __MATRIXSTACK_H_
#define __MATRIXSTACK_H_

#include <stack>
#include "Matrix.h"

class CMatrixStack : public std::stack<Q3DStudio::CMatrix>
{
public:
    //==============================================================================
    /**
    *	Constructor initializes the stack with an identity matrix up top.
    */
    CMatrixStack() { std::stack<Q3DStudio::CMatrix>::push(Q3DStudio::CMatrix()); }

    /**
    *	Push a matrix onto the stack and autmatically multiply this new matrix
    *	at the top of the stack with the old top matrix.  This is when you
    *	continually push local matrices onto this stack since this auto
    *	multiplication esures that the top matrix is the global matrix.
    *
    *	@param	inMatrix	matrix to be pushed and muliplied onto the stack
    */
    void push(const Q3DStudio::CMatrix &inMatrix)
    {
        std::stack<Q3DStudio::CMatrix>::push(inMatrix * top());
    }
};

#endif //__MATRIXSTACK_H_
