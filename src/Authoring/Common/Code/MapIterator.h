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
#ifndef __MAPITERATOR_H__
#define __MAPITERATOR_H__

//==============================================================================
//	Class
//==============================================================================
/**
 *	@class CMapIterator
 */
template <class Obj, class Map>
class CMapIterator
{
    // Typedefs
public:
    typedef typename Map::iterator TMapItr;

    // Field members
protected:
    typename Map::iterator m_Begin;
    typename Map::iterator m_Pos;
    typename Map::iterator m_End;

    // Construction/Destruction
public:
    CMapIterator(TMapItr inBegin, TMapItr inEnd);
    ~CMapIterator();

    // Access
public:
    bool HasNext();
    Obj GetCurrent();
    CMapIterator<Obj, Map> operator++();
    CMapIterator<Obj, Map> operator--();
    CMapIterator<Obj, Map> operator+=(const long &inNum);
    CMapIterator<Obj, Map> operator-=(const long &inNum);
    operator typename Map::iterator();
    CMapIterator<Obj, Map> &operator=(const typename Map::iterator &inIterator);
};

/**
 *	Constructs a Map iterator based given the begin and end iterators for the Map.
 */
template <class Obj, class Map>
CMapIterator<Obj, Map>::CMapIterator(typename Map::iterator inBegin, typename Map::iterator inEnd)
    : m_Begin(inBegin)
    , m_Pos(inBegin)
    , m_End(inEnd)
{
}

/**
 *	Destructor
 */
template <class Obj, class Map>
CMapIterator<Obj, Map>::~CMapIterator()
{
}

/**
 *	Returns true if there is another item in the Map.
 */
template <class Obj, class Map>
bool CMapIterator<Obj, Map>::HasNext()
{
    return m_Pos != m_End;
}

/**
 *	Obtains the current object in the Map.
 */
template <class Obj, class Map>
Obj CMapIterator<Obj, Map>::GetCurrent()
{
    return m_Pos->second;
}

/**
 *	Increments the iterator to point to the next object in the Map.
 */
template <class Obj, class Map>
CMapIterator<Obj, Map> CMapIterator<Obj, Map>::operator++()
{
    if (m_Pos != m_End)
        ++m_Pos;
    return *this;
}

/**
 *	Decrements the iterator to point to the previous object in the Map.
 */
template <class Obj, class Map>
CMapIterator<Obj, Map> CMapIterator<Obj, Map>::operator--()
{
    if (m_Pos != m_Begin)
        --m_Pos;
    return *this;
}

/**
 *	Increments the iterator to point to the an object later in the Map.
 */
template <class Obj, class Map>
CMapIterator<Obj, Map> CMapIterator<Obj, Map>::operator+=(const long &inNum)
{
    if ((m_Pos + inNum) >= m_Begin && (m_Pos + inNum) < m_End)
        m_Pos += inNum;
    return *this;
}

/**
 *	Decrements the iterator to point to the an object earlier in the Map.
 */
template <class Obj, class Map>
CMapIterator<Obj, Map> CMapIterator<Obj, Map>::operator-=(const long &inNum)
{
    if ((m_Pos - inNum) >= m_Begin && (m_Pos - inNum) < m_End)
        m_Pos -= inNum;
    return *this;
}

/**
 *	Implicit cast operator to obtain the actual iterator from the container.
 */
template <class Obj, class Map>
CMapIterator<Obj, Map>::operator typename Map::iterator()
{
    return m_Pos;
}

/**
 *	Assignment operator for taking in an iterator of the container type.
 */
template <class Obj, class Map>
CMapIterator<Obj, Map> &CMapIterator<Obj, Map>::operator=(const typename Map::iterator &inIterator)
{
    if (m_Pos != inIterator)
        m_Pos = inIterator;

    return *this;
}

#endif // __MAPITERATOR_H__