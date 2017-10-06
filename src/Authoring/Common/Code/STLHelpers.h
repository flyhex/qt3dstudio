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
#ifndef __STLHELPERS_H__
#define __STLHELPERS_H__

namespace Q3DStudio {

/**
 *	@class CForEachDelete
 *	@brief Used in the std::for_each to delete object of type T.
 */
template <class T>
class CForEachDelete
{
public: // Operators
    void operator()(T *&inMember)
    {
        delete inMember;
        inMember = NULL;
    }
};

/**
 *	@class CForEachDeleteMapFirst
 *	@brief Used in the std::for_each to delete the Itr->first object from a std::map.
 */
template <class T1, class T2>
class CForEachDeleteMapFirst
{
public: // Operators
    void operator()(std::pair<T1, T2> inMember)
    {
        delete inMember.first;
        inMember.first = NULL;
    }
};

/**
 *	@class CForEachDeleteMapSecond
 *	@brief Used in the std::for_each to delete the Itr->second object from a std::map.
 */
template <class T1, class T2>
class CForEachDeleteMapSecond
{
public: // Operators
    void operator()(std::pair<T1, T2> inMember)
    {
        delete inMember.second;
        inMember.second = NULL;
    }
};

/**
 *	@class CForEachDeleteMapFirstAndSecond
 *	@brief Used in the std::for_each to delete the Itr->first and Itr->second object from a
 *std::map.
 */
template <class T1, class T2>
class CForEachDeleteMapFirstAndSecond
{
public: // Operators
    void operator()(std::pair<T1, T2> inMember)
    {
        delete inMember.first;
        inMember.first = NULL;

        delete inMember.second;
        inMember.second = NULL;
    }
};
} // namespace Q3DStudio
#endif // __STLHELPERS_H__