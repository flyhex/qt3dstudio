/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#ifndef INCLUDED_SAFE_ARRAY_H
#define INCLUDED_SAFE_ARRAY_H 1

#pragma once

#include <vector>
#include <assert.h>
#include "SIterator.h"

#ifndef ASSERT
#ifdef _DEBUG
#define ASSERT(a) assert(a)
#else
#define ASSERT(a)
#endif
#endif

template <class TObject>
class CSafeArray;

template <class TObject>
class CSafeArray
{
public:
    class CSafeIterator;
    class CIterator;
    class CReverseIterator;

    typedef std::vector<TObject> TVector;
    typedef std::vector<CSafeIterator *> TIteratorList;

    CSafeArray();
    virtual ~CSafeArray();

    void Add(const TObject &inObject);
    void Add(const TObject &inObject, const CSafeIterator &inBeforePos);
    void Remove(const CSafeIterator &inIterator);

    TObject Get(long inIndex);

    long GetCount();

    // These functions were moved into the header to eliminate the CodeWarrior warnings that are
    // produced
    // when the functions are outside of the header file.
    inline CIterator Begin() { return CIterator(this); }
    CReverseIterator RBegin() { return CReverseIterator(this); }
    //=============================================================================
    /**
     * Get a pointer iterating through this array forwards.
     * @return an iterator iterating forwards through this array.
     */
    inline CIterator GetIterator() { return CIterator(this); }

    //=============================================================================
    /**
     * Get a pointer iterating through this array backwards.
     * @return an iterator iterating bakcwards through this array.
     */
    CReverseIterator GetReverseIterator() { return CReverseIterator(this); }

    //=============================================================================
    /**
     * Register an iterator as being an iterator on this array.
     * This will provide the iterator with access to the underlying vector.
     * @param inIterator the iterator that is being registered.
     * @return the underlying vector, for fast access.
     */
    TVector *Register(CSafeIterator *inIterator)
    {
        m_Iterators.push_back(inIterator);

        return &m_Array;
    }
    void Deregister(CSafeIterator *inIterator);

public:
    class CSafeIterator : public CSIterator<TObject>
    {
        //		friend CSafeArray<TObject>;
    public:
        CSafeIterator() {}
        virtual ~CSafeIterator() {}

        virtual void ArrayIndexAdded(long inIndex) = 0;
        virtual void ArrayIndexRemoved(long inIndex) = 0;
        virtual long GetArrayIndex() const = 0;
        virtual typename std::vector<TObject>::iterator GetIterator() const = 0;
        virtual void ArrayGoingAway() = 0;
    };

    //==========================================================================
    /**
     * Forward iterator class.
     */
    class CIterator : public CSafeIterator
    {
        typedef std::vector<TObject> TVector;

    public:
        //=======================================================================
        /**
         * Create a forward iterator iterating over inArray.
         * @param inArray the array this is iterating over.
         */
        CIterator(CSafeArray<TObject> *inArray)
        {
            m_Array = inArray;
            m_Vector = inArray->Register(this);
            m_Position = m_Vector->begin();
            m_Index = 0;
            m_IsInvalid = false;
        }

        //=======================================================================
        /**
         * Copy constructor for a forward iterator.
         * @param inIterator the iterator being copied.
         */
        CIterator(const CIterator &inIterator)
        {
            m_Array = inIterator.m_Array;
            if (m_Array) {
                m_Vector = m_Array->Register(this);

                m_Position = inIterator.m_Position;
                m_Index = inIterator.m_Index;
                m_IsInvalid = inIterator.m_IsInvalid;
            }
        }

        //=======================================================================
        /**
         * Destructor, cool eh?
         */
        virtual ~CIterator()
        {
            if (m_Array)
                m_Array->Deregister(this);
        }

        //=======================================================================
        /**
         * Assignment operator, Copies inIterator.
         * @param inIterator the iterator being copied.
         */
        CIterator &operator=(const CIterator &inIterator)
        {
            if (&inIterator == this)
                return *this;

            // Deregister from previous array
            if (m_Array)
                m_Array->Deregister(this);

            m_Array = inIterator.m_Array;
            if (m_Array) {
                m_Position = inIterator.m_Position;
                m_Index = inIterator.m_Index;
                m_IsInvalid = inIterator.m_IsInvalid;

                m_Vector = m_Array->Register(this);
            }

            return *this;
        }

        //=======================================================================
        /**
         * Increment the index of the object in the array.
         * GetCurrent will now refer to the next object.
         */
        void operator++() override
        {
            if (m_Array) {
                ++m_Index;

                m_Position = m_Vector->begin();
                if (m_Index > 0 && m_Index < (long)m_Vector->size())
                    m_Position += m_Index;

                m_IsInvalid = false;
            }
        }

        //=======================================================================
        /**
         * Increment the index of the object in the array.
         * GetCurrent will now point to index + inNumToInc'th object.
         * @param inNumToInc the amount to increment the index by.
         */
        void operator+=(const long inNumToInc) override
        {
            if (m_Array) {
                m_Index += inNumToInc;

                m_Position = m_Vector->begin();
                if (m_Index > 0 && m_Index < (long)m_Vector->size())
                    m_Position += m_Index;

                m_IsInvalid = false;
            }
        }

        //=======================================================================
        /**
         * Checks to see if there's any more elements in the array.
         * @return true if GetCurrent can be called again.
         */
        bool IsDone() override
        {
            if (m_Array)
                return m_Index >= (long)m_Vector->size();
            else
                return true;
        }

        bool HasNext()
        {
            if (m_Array)
                return m_Index < (long)m_Vector->size();
            else
                return false;
        }

        //=======================================================================
        /**
         * Get the element at the current index.
         * @return the element at the current index.
         */
        TObject GetCurrent() override
        {
            if (m_Array) {
                // assert( !m_IsInvalid );

                return (*m_Position);
            }
            // assert( false );

            return (*m_Position);
        }

        TObject operator*() override
        {
            if (m_Array) {
                // assert( !m_IsInvalid );
                return (*m_Position);
            }
            // assert( false );
            return (*m_Position);
        }

    protected:
        //=======================================================================
        /**
         * Notification from the Array that an item was removed.
         * @param inIndex the index of the object that was removed.
         */
        void ArrayIndexRemoved(long inIndex) override
        {
            if (m_Array) {
                if (inIndex <= m_Index)
                    --m_Index;

                m_Position = m_Vector->begin();

                if (m_Index > 0 && m_Index < (long)m_Vector->size())
                    m_Position += m_Index;

                m_IsInvalid = true;
            }
        }

        //=======================================================================
        /**
         * Notification from the array that an item was added.
         * @param inIndex the index of the object that was added.
         */
        void ArrayIndexAdded(long inIndex) override
        {
            if (m_Array) {
                if (inIndex <= m_Index)
                    ++m_Index;

                m_Position = m_Vector->begin();

                if (m_Index > 0 && m_Index < (long)m_Vector->size())
                    m_Position += m_Index;

                m_IsInvalid = true;
            }
        }

        //=======================================================================
        /**
         * Notification from the array that it is being deleted.
         * This will cause all current iterating to end.
         */
        void ArrayGoingAway() override { m_Array = NULL; }

        //=======================================================================
        /**
         * Get the index of the item that this is referring to.
         * @return the index of the item this is referring to.
         */
        long GetArrayIndex() const override { return m_Index; }

        //=======================================================================
        /**
         * Get the underlying control of this iterator.
         * @return the underlying iterator.
         */
        typename TVector::iterator GetIterator() const override { return m_Position; }

        CSafeArray<TObject> *m_Array;
        TVector *m_Vector;
        typename TVector::iterator m_Position;
        long m_Index;
        bool m_IsInvalid;
    };

    class CReverseIterator : public CSafeIterator
    {
        typedef std::vector<TObject> TVector;

    public:
        //=======================================================================
        /**
         * Creates a reverse iterator iterating over inArray.
         * @param inArray the array this is iterating over.
         */
        CReverseIterator(CSafeArray<TObject> *inArray)
        {
            m_Array = inArray;
            m_Vector = inArray->Register(this);
            m_RPosition = m_Vector->rbegin();

            m_Index = 0;
            m_IsInvalid = false;
        }

        //=======================================================================
        /**
         * Copies a reverse iterator.
         */
        CReverseIterator(const CReverseIterator &inIterator)
        {
            m_Array = inIterator.m_Array;
            if (m_Array) {
                m_Vector = m_Array->Register(this);
                m_RPosition = inIterator.m_RPosition;
                m_Index = inIterator.m_Index;
                m_IsInvalid = inIterator.m_IsInvalid;
            }
        }

        virtual ~CReverseIterator()
        {
            // Remove this from the list of iterators in m_Array.
            if (m_Array)
                m_Array->Deregister(this);
        }

        //=======================================================================
        /**
         * Set this iterator equal to inIterator.
         * @param inIterator the iterator being copied.
         */
        CReverseIterator &operator=(const CReverseIterator &inIterator)
        {
            if (&inIterator == this)
                return *this;

            // Remove registration from previous array
            if (m_Array)
                m_Array->Deregister(this);

            m_Array = inIterator.m_Array;
            m_RPosition = inIterator.m_RPosition;
            m_Index = inIterator.m_Index;
            m_IsInvalid = inIterator.m_IsInvalid;

            // Add registration to new array.
            if (m_Array)
                m_Vector = m_Array->Register(this);

            return *this;
        }

        //=======================================================================
        /**
         * Increment the location of this iterator.
         * This will advance to the next object.
         */
        virtual void operator++()
        {
            if (m_Array) {
                ++m_RPosition;
                ++m_Index;
            }

            m_IsInvalid = false;
        }

        //=======================================================================
        /**
         * Increment the location of this iterator inNumToInc places.
         * This will advance to the next inNumToInc object.
         * @param inNumToInc the number of places to advance.
         */
        virtual void operator+=(const long inNumToInc)
        {
            if (m_Array) {
                m_RPosition += inNumToInc;
                m_Index += inNumToInc;
            }
            m_IsInvalid = false;
        }

        //=======================================================================
        /**
         * Checks to see if there are any more objects in this iterator.
         * @return true if GetCurrent can be called.
         */
        virtual bool IsDone()
        {
            if (m_Array)
                return m_RPosition == m_Vector->rend();
            return true;
        }

        bool HasNext()
        {
            if (m_Array)
                return m_RPosition != m_Vector->rend();
            return false;
        }

        //=======================================================================
        /**
         * Get the current object in this iterator.
         * @return the object at the current position this iterator is pointing to.
         */
        virtual TObject GetCurrent()
        {
            ASSERT(!m_IsInvalid);
            ASSERT(m_Array);

            return (*m_RPosition);
        }

        TObject operator*()
        {
            ASSERT(!m_IsInvalid);
            ASSERT(m_Array);

            return (*m_RPosition);
        }

    protected:
        //=======================================================================
        /**
         * Notification from the Array that the item at inIndex was removed.
         * @param inIndex the index of the object that was removed.
         */
        void ArrayIndexRemoved(long inIndex)
        {
            ASSERT(m_Array);

            if ((long)(m_Vector->size() - inIndex) <= m_Index) {
                --m_Index;
            }

            m_RPosition = m_Vector->rbegin();

            if (m_Index > 0 && m_Index < (long)m_Vector->size())
                m_RPosition += m_Index;

            m_IsInvalid = true;
        }

        //=======================================================================
        /**
         * Notification from the Array that an item was added at inIndex.
         * @param inIndex the index of the object that was added.
         */
        void ArrayIndexAdded(long inIndex)
        {
            ASSERT(m_Array);

            if ((long)(m_Vector->size() - inIndex) <= m_Index) {
                ++m_Index;
            }
            m_RPosition = m_Vector->rbegin();
            if (m_Index > 0)
                m_RPosition += m_Index;

            m_IsInvalid = true;
        }

        //=======================================================================
        /**
         * Notification from the Array that it is being deleted.
         * This will cause all iteration to end.
         */
        void ArrayGoingAway()
        {
            ASSERT(m_Array);

            m_Array = NULL;
        }

        //=======================================================================
        /**
         * Get the index of the item that this array is pointing to.
         * @return the index of the item that this array is pointing to.
         */
        long GetArrayIndex() const
        {
            ASSERT(m_Array);

            return (long)m_Vector->size() - m_Index;
        }

        //=======================================================================
        /**
         * Get the underlying iterator that this is using.
         * @return the underlying iterator that this is using.
         */
        typename TVector::iterator GetIterator() const
        {
            ASSERT(m_Array);

            return m_RPosition.base();
        }

        CSafeArray<TObject> *m_Array; ///< The Array that this is iterating over.
        TVector *m_Vector; ///< The fast-access vector of the array this is iterating over.
        typename TVector::reverse_iterator
            m_RPosition; ///< The position of this iterator, in terms of m_Vector.
        long m_Index; ///< The index of this iterator in the vector.
        bool m_IsInvalid; ///< True if the GetCurrent is invalid, the ++ operator must be called
                          ///first.
    };

protected:
    TVector m_Array;
    TIteratorList m_Iterators;
};

//=============================================================================
/**
 * Creates an empty SafeArray.
 */
template <typename T>
inline CSafeArray<T>::CSafeArray()
{
}

//=============================================================================
/**
 * Destructor.
 */
template <typename T>
inline CSafeArray<T>::~CSafeArray()
{
    typename TIteratorList::iterator theIterPos = m_Iterators.begin();
    for (; theIterPos != m_Iterators.end(); ++theIterPos) {
        (*theIterPos)->ArrayGoingAway();
    }
}

//=============================================================================
/**
 * Add an element to the back of this array.
 * @param inElement the element to be added.
 */
template <typename T>
inline void CSafeArray<T>::Add(const T &inElement)
{
    long theIndex = long(m_Array.size());
    m_Array.push_back(inElement);

    typename TIteratorList::iterator theIterPos = m_Iterators.begin();
    for (; theIterPos != m_Iterators.end(); ++theIterPos) {
        (*theIterPos)->ArrayIndexAdded(theIndex);
    }
}

//=============================================================================
/**
 * Add an element to this array at a specified index.
 * @param inElement the element to be added to this array.
 * @param inBeforePos the position this should be added in, pushing the existing
 *		element up one.
 */
template <typename T>
inline void CSafeArray<T>::Add(const T &inElement,
                               const typename CSafeArray<T>::CSafeIterator &inBeforePos)
{
    long theIndex = inBeforePos.GetArrayIndex();
    m_Array.insert(inBeforePos.GetIterator(), inElement);

    typename TIteratorList::iterator theIterPos = m_Iterators.begin();
    for (; theIterPos != m_Iterators.end(); ++theIterPos) {
        (*theIterPos)->ArrayIndexAdded(theIndex);
    }
}

//=============================================================================
/**
 * Remove an element from this array.
 * @param inIterator the index of this item to be removed.
 */
template <typename T>
inline void CSafeArray<T>::Remove(const typename CSafeArray<T>::CSafeIterator &inIterator)
{
    long theErasedIndex = 0;
    theErasedIndex = inIterator.GetArrayIndex();
    m_Array.erase(inIterator.GetIterator());

    typename TIteratorList::iterator theIterPos = m_Iterators.begin();
    for (; theIterPos != m_Iterators.end(); ++theIterPos) {
        (*theIterPos)->ArrayIndexRemoved(theErasedIndex);
    }
}

//=============================================================================
/**
 * Get the item at index inIndex.
 * @param inIndex the index of the item to fetch.
 * @return the item at index inIndex.
 */
template <typename T>
inline T CSafeArray<T>::Get(long inIndex)
{
    return m_Array.at(inIndex);
}

//=============================================================================
/**
 * Get the number of elements in this array.
 */
template <typename T>
inline long CSafeArray<T>::GetCount()
{
    return long(m_Array.size());
}

//=============================================================================
/**
 * Remove an iterator from the list of iterators iterating on this object.
 * @param inIterator the location of the iterator to be removed.
 */
template <typename T>
inline void CSafeArray<T>::Deregister(typename CSafeArray<T>::CSafeIterator *inIterator)
{
    typename TIteratorList::iterator thePos = m_Iterators.begin();
    for (; thePos != m_Iterators.end(); ++thePos) {
        if ((*thePos) == inIterator) {
            m_Iterators.erase(thePos);
            break;
        }
    }
}
#endif // INCLUDED_SAFE_ARRAY_H
