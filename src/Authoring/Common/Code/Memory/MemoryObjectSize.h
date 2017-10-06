/****************************************************************************
**
** Copyright (C) 1999-2005 NVIDIA Corporation.
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

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include <map>
#include <set>
#include <list>
#include <vector>

/**
 *	Helper class to assist in calculating the sizes of well known objects.
 */
template <typename T1 = long, typename T2 = long, typename T3 = T1>
class CMemoryObjectSize
{
public:
    static long GetSize(std::vector<T1> &inValue) { return inValue.size() * sizeof(T3); }

    static long GetSize(std::list<T1> &inValue) { return inValue.size() * sizeof(T3); }

    static long GetSizeForEach(std::vector<T1> &inValue)
    {
        long theSize = 0;

        // Calculate the size of each vector in the map
        FOR_EACH(T1 & theItem, inValue) { theSize += sizeof(T1) + theItem.GetMemoryUsage(); }

        return theSize;
    }

    static long GetSizeForEach(std::list<T1> &inValue)
    {
        long theSize = 0;

        // Calculate the size of each vector in the map
        FOR_EACH(T1 & theItem, inValue) { theSize += sizeof(T1) + theItem.GetMemoryUsage(); }

        return theSize;
    }

    static long GetSizeForEach(std::map<T1, T2> &inValue)
    {
        long theSize = 0;

        // Calculate the size of each vector in the map
        typedef std::map<T1, T2> TRef;
        FOR_EACH(TRef::const_reference thePair, inValue)
        {
            theSize += sizeof(T1) + sizeof(T2) + thePair.second->GetMemoryUsage();
        }

        return theSize;
    }

    static long GetSize(std::map<T1, std::vector<T2>> &inValue)
    {
        // Calculate the size of the map
        long theSize = inValue.size() * (sizeof(T1) + sizeof(T2));

        // Calculate the size of each vector in the map
        typedef std::map<T1, std::vector<T2>> TRef;
        FOR_EACH(TRef::const_reference thePair, inValue)
        {
            theSize += thePair.second.size() * sizeof(T2);
        }

        return theSize;
    }

    static long GetSize(std::map<T1, T2> &inValue)
    {
        return inValue.size() * (sizeof(T1) + sizeof(T3));
    }

    static long GetSize(std::multiset<T1, T2> &inValue)
    {
        return inValue.size() * (sizeof(T1) + sizeof(T2));
    }

    static long GetSize(Q3DStudio::CString &inValue)
    {
        long theLength = inValue.Length();
        return (theLength * sizeof(wchar_t)) + // m_WideData
            (theLength * sizeof(char)) + // m_CharData
            (theLength * sizeof(char)) // m_MultiData
            ;
    }
};
