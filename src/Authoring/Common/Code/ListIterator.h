/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#ifndef __LISTITERATOR_H__
#define __LISTITERATOR_H__

//==============================================================================
//	Class
//==============================================================================
/**
 *	@class CListIterator
 */
template <class Obj, class List>
class CListIterator
{
    // Typedefs
    // Field members
protected:
    typename List::iterator m_Begin;
    typename List::iterator m_Pos;
    typename List::iterator m_End;

    // Construction/Destruction
public:
    CListIterator(typename List::iterator inBegin, typename List::iterator inEnd);
    ~CListIterator();

    // Access
public:
    bool HasNext();
    Obj GetCurrent();
    CListIterator<Obj, List> operator++();
    CListIterator<Obj, List> operator--();
    CListIterator<Obj, List> operator+=(const long &inNum);
    CListIterator<Obj, List> operator-=(const long &inNum);
    operator typename List::iterator();
    CListIterator<Obj, List> &operator=(const typename List::iterator &inIterator);
};

/**
 *	Constructs a list iterator based given the begin and end iterators for the List.
 */
template <class Obj, class List>
CListIterator<Obj, List>::CListIterator(typename List::iterator inBegin,
                                        typename List::iterator inEnd)
    : m_Begin(inBegin)
    , m_Pos(inBegin)
    , m_End(inEnd)
{
}

/**
 *	Destructor
 */
template <class Obj, class List>
CListIterator<Obj, List>::~CListIterator()
{
}

/**
 *	Returns true if there is another item in the list.
 */
template <class Obj, class List>
bool CListIterator<Obj, List>::HasNext()
{
    return m_Pos != m_End;
}

/**
 *	Obtains the current object in the list.
 */
template <class Obj, class List>
Obj CListIterator<Obj, List>::GetCurrent()
{
    if (m_Pos != m_End)
        return *m_Pos;
    return NULL;
}

/**
 *	Increments the iterator to point to the next object in the list.
 */
template <class Obj, class List>
CListIterator<Obj, List> CListIterator<Obj, List>::operator++()
{
    if (m_Pos != m_End)
        ++m_Pos;
    return *this;
}

/**
 *	Decrements the iterator to point to the previous object in the list.
 */
template <class Obj, class List>
CListIterator<Obj, List> CListIterator<Obj, List>::operator--()
{
    if (m_Pos != m_Begin)
        --m_Pos;
    return *this;
}

/**
 *	Increments the iterator to point to the an object later in the list.
 */
template <class Obj, class List>
CListIterator<Obj, List> CListIterator<Obj, List>::operator+=(const long &inNum)
{
    if ((m_Pos + inNum) >= m_Begin && (m_Pos + inNum) < m_End)
        m_Pos += inNum;
    return *this;
}

/**
 *	Decrements the iterator to point to the an object earlier in the list.
 */
template <class Obj, class List>
CListIterator<Obj, List> CListIterator<Obj, List>::operator-=(const long &inNum)
{
    if ((m_Pos - inNum) >= m_Begin && (m_Pos - inNum) < m_End)
        m_Pos -= inNum;
    return *this;
}

/**
 *	Implicit cast operator to obtain the actual iterator from the container.
 */
template <class Obj, class List>
CListIterator<Obj, List>::operator typename List::iterator()
{
    return m_Pos;
}

/**
 *	Assignment operator for taking in an iterator of the container type.
 */
template <class Obj, class List>
CListIterator<Obj, List> &CListIterator<Obj, List>::
operator=(const typename List::iterator &inIterator)
{
    if (m_Pos != inIterator)
        m_Pos = inIterator;

    return *this;
}

#endif // __LISTITERATOR_H__