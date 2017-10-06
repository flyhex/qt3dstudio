/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

//==============================================================================
//	Includes
//==============================================================================
#include "UICSyncPrimitive.h"
#include "UICArray.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Thread safe queue that wraps using a sync primitive when it accesses
 *	it's internals.
 */
template <typename T>
class CThreadSafeQueue
{
    //==============================================================================
    //	Fields
    //==============================================================================
private:
    CSyncPrimitive m_Primitive;
    CArray<T> m_QueueData;

    //==============================================================================
    //	Fields
    //==============================================================================
private:
    CThreadSafeQueue(const CThreadSafeQueue &other);
    CThreadSafeQueue &operator=(const CThreadSafeQueue &other);

public:
    CThreadSafeQueue() {}
    void Push(const T &inItem);
    bool Pop(T &outItem);
    INT32 GetCount();

    //=========================================================================
    // Remove items that correspond to a given predicate.
    // The predicate is a functor that takes a const reference and returns either
    // true or false.
    template <typename TPredFunc>
    void Remove(TPredFunc predicate, CArray<T> &ioOutRemovedItems);
};

} // namespace Q3DStudio

//==============================================================================
//	Template code
//==============================================================================
#include "UICThreadSafeQueue.inl"
