/****************************************************************************
**
** Copyright (C) 1999-2001 Anark Corporation.
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
#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Mutex.h"
#include "Guard.h"
#include <queue>

//==============================================================================
//	Forwards
//==============================================================================

//==============================================================================
//	Constants
//==============================================================================

//==============================================================================
//	Class
//==============================================================================
//==============================================================================
/**
 *	@class	CSafeQueue:
 *	@brief	A thread safe, template class that wraps std::queue.
 *
 *	This class uses the Bomb Ass Critical Section class to protect every queue
 *	method with a critical section.  It is a template class.
 *
 */
//==============================================================================
template <class T>
class CSafeQueue
{
    //==============================================================================
    //	Fields
    //==============================================================================

private:
    std::queue<T> m_Queue; ///< The std::queue that this class wraps.
    CMutex m_Mutex; ///< The critical section

    //==============================================================================
    //	Methods
    //==============================================================================

    // Construction

public:
    CSafeQueue() {}
    virtual ~CSafeQueue() {}

    // Access

public:
    void Push(const T &inItem);
    void Pop();
    bool Empty();
    size_t Size();
    const T &Front() const;
    T &Front();
    const T &Back() const;
    T &Back();
};

//==============================================================================
/**
 *	Push:	The member function inserts an element with value inItem at the end
 *			of the controlled sequence..
 *
 *	@param inItem The item that is being inserted into the queue.
 *
 *	@return void
 */
//==============================================================================
template <class T>
void CSafeQueue<T>::Push(const T &inItem)
{
    CGuard theThreadGuard(m_Mutex);

    m_Queue.push(inItem);
}

//==============================================================================
/**
 *	Pop:	The member function removes the last element of the controlled
 *			sequence, which must be non-empty.
 *
 *	@return void
 */
//==============================================================================
template <class T>
void CSafeQueue<T>::Pop()
{
    CGuard theThreadGuard(m_Mutex);

    m_Queue.pop();
}

//==============================================================================
/**
 *	Empty:	The member function tells if the queue is empty or not.
 *
 *	@return bool Returns true if the queue is empty, and false if it's not.
 */
//=============================================================================
template <class T>
bool CSafeQueue<T>::Empty()
{
    CGuard theThreadGuard(m_Mutex);

    return m_Queue.empty();
}

//==============================================================================
/**
 *	Size:	The member function returns the length of the controlled sequence.
 *
 *	@return size_t The number of items in the queue.
 */
//=============================================================================
template <class T>
size_t CSafeQueue<T>::Size()
{
    CGuard theThreadGuard(m_Mutex);

    return m_Queue.size();
}

//==============================================================================
/**
 *	Front:	The member function returns a reference to the first element of the
 *			controlled sequence, which must be non-empty.
 *
 *	@return T& A reference to the first element.
 */
//=============================================================================
template <class T>
T &CSafeQueue<T>::Front()
{
    CGuard theThreadGuard(m_Mutex);

    return m_Queue.front();
}

//==============================================================================
/**
 *	Front:	The member function returns a reference to the first element of the
 *			controlled sequence, which must be non-empty.
 *
 *	@return const T& A reference to the first element.
 */
//=============================================================================
template <class T>
const T &CSafeQueue<T>::Front() const
{
    CGuard theThreadGuard(m_Mutex);

    return m_Queue.front();
}

//==============================================================================
/**
 *	Back:	The member function returns a reference to the last element of the
 *			controlled sequence, which must be non-empty.
 *
 *	@return const T& A reference to the first element.
 */
//=============================================================================
template <class T>
T &CSafeQueue<T>::Back()
{
    CGuard theThreadGuard(m_Mutex);

    return m_Queue.back();
}

//==============================================================================
/**
 *	Back:	The member function returns a reference to the last element of the
 *			controlled sequence, which must be non-empty.
 *
 *	@return const T& A reference to the first element.
 */
//=============================================================================
template <class T>
const T &CSafeQueue<T>::Back() const
{
    CGuard theThreadGuard(m_Mutex);

    return m_Queue.back();
}