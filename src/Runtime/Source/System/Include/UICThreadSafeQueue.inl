/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Safely push an object onto the queue
 */
template <typename T>
inline void CThreadSafeQueue<T>::Push(const T &inItem)
{
    CSyncGuard theGuard(m_Primitive);
    m_QueueData.Push(inItem);
}

//==============================================================================
/**
*	Safely pop an object off the queue
*/
template <typename T>
inline bool CThreadSafeQueue<T>::Pop(T &outItem)
{
    CSyncGuard theGuard(m_Primitive);
    if (m_QueueData.GetCount()) {
        outItem = m_QueueData[0];
        m_QueueData.Remove(0);
        return true;
    }
    return false;
}

//==============================================================================
/**
 *	Get the count queue count
 */
template <typename T>
INT32 CThreadSafeQueue<T>::GetCount()
{
    CSyncGuard theGuard(m_Primitive);
    return m_QueueData.GetCount();
}

template <typename T>
template <typename TPredFunc>
inline void CThreadSafeQueue<T>::Remove(TPredFunc predicate, CArray<T> &ioOutRemovedItems)
{
    CSyncGuard theGuard(m_Primitive);
    for (INT32 index = m_QueueData.GetCount() - 1; index >= 0; --index) {
        const T &item = m_QueueData[index];
        if (predicate(item)) {
            ioOutRemovedItems.Push(item);
            m_QueueData.Remove(index);
        }
    }
}

} // namespace Q3DStudio
