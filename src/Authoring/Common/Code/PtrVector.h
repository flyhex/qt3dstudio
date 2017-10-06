/****************************************************************************
**
** Copyright (C) 1999-2004 NVIDIA Corporation.
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
#ifndef __PTRVECTOR_H__
#define __PTRVECTOR_H__

namespace Q3DStudio {

/**
 *	@class CPtrVector: This is wrapper class for an std::vector of pointers.
 *	The reason for it's existance is because it will provide proper cleanup of
 *  stored pointers that an std::vector<CAutoMemPtrs> sometimes do not do properly.
 */
template <typename TPtr>
class CPtrVector
{
public:
    typedef std::vector<TPtr *> TPtrVector;
    typedef typename TPtrVector::const_iterator
        iterator; // protection against accidentally using non-const iterators
    typedef typename TPtrVector::const_iterator const_iterator;
    typedef typename TPtrVector::iterator TNonConstIterator;

protected:
    TPtrVector m_PtrVector;

public:
    CPtrVector(){}

    ~CPtrVector() { clear(); }
    long size() const { return static_cast<long>(m_PtrVector.size()); }

    TPtr *at(long inIndex) const { return m_PtrVector.at(inIndex); }

    TPtr *operator[](long inIndex) { return m_PtrVector.at(inIndex); }

    void ReplaceAtIndex(long inIndex, TPtr *theNewPtr)
    {
        TPtr *theOldPtr = m_PtrVector.at(inIndex);
        delete theOldPtr;
        m_PtrVector.at(inIndex) = theNewPtr;
    }

    void DeleteAtIndex(long inIndex)
    {
        std::vector<TPtr *>::iterator theItr = m_PtrVector.begin() + inIndex;
        TPtr *thePtr = theItr;
        m_PtrVector.erase(theItr);
        delete thePtr;
    }

    void pop_back()
    {
        delete m_PtrVector.back();
        m_PtrVector.pop_back();
    }

    void push_back(TPtr *inNewPtr) { m_PtrVector.push_back(inNewPtr); }

    void insert(TNonConstIterator inItr, TPtr *inNewPtr) { m_PtrVector.insert(inItr, inNewPtr); }

    iterator begin() const { return m_PtrVector.begin(); }

    TNonConstIterator NonConstBegin() { return m_PtrVector.begin(); }

    iterator end() const { return m_PtrVector.end(); }

    void erase(TNonConstIterator inItr, bool inDoDelete = true)
    {
        TPtr *thePtr = (*inItr);
        m_PtrVector.erase(inItr);
        if (inDoDelete)
            delete thePtr;
    }

    void clear()
    {
        CPtrVector::iterator theItr = begin();
        CPtrVector::iterator theEnd = end();

        TPtr *thePtr = NULL;
        for (; theItr != theEnd; ++theItr) {
            thePtr = (*theItr);
            delete thePtr;
        }

        m_PtrVector.clear();
    }

    bool empty() { return m_PtrVector.empty(); }

    long IndexOf(const TPtr *inElement)
    {
        iterator theIter = m_PtrVector.begin();
        const_iterator theEnd = m_PtrVector.end();
        long theCount = 0;
        bool theFound = false;

        for (; theIter != theEnd; ++theIter) {
            if (*theIter == inElement) {
                theFound = true;
                break;
            }

            ++theCount;
        }

        if (theFound)
            return theCount;
        else
            return -1;
    }
};
}

#endif __PTRVECTOR_H__