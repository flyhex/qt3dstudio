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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef __UICSHAREDPTR_H__
#define __UICSHAREDPTR_H__

#pragma once

#include "AutoPtr.h"
#include "UICExceptions.h"
//==============================================================================
//	Class
//==============================================================================

namespace Q3DStudio {

//==============================================================================
/**
*	@class	CReferenceCount
*	@brief	This class provides base reference counting for the CSharedPtr.
*
*	This class simply maintains a reference count.
*
*	@note No checking is performed for releasing the reference to many times.
*/
class CReferenceCount
{
protected:
    long m_RefCount; ///< Referenced count variable

    // Construction
public:
    CReferenceCount(long inStartRefCount = 0)
        : m_RefCount(inStartRefCount)
    {
    }
    virtual ~CReferenceCount()
    {
#ifdef _DEBUG
        if (m_RefCount != 0)
            throw;
#endif
    }

    // Operations
public:
    // Add a reference
    long Add() { return ++m_RefCount; }
    // Release a reference
    long Release() { return --m_RefCount; }

    // Access
public:
    // Is this object referenced
    bool IsReferenced() { return (m_RefCount > 0); }
    // Is this the only reference on the object
    bool IsUnique() { return (m_RefCount == 1); }
    // Return the count of references on the object
    long Count() { return m_RefCount; }
};

//==============================================================================
/**
*	@class	CSharedPtr
*	@brief	This class provides ref counted sharing of any pointer.
*/
template <class T>
class CSharedPtr
{
protected:
    class SSharedObject
    {
    public:
        SSharedObject(T *inPointer = NULL)
            : m_Pointer(inPointer)
        {
        }

        CAutoMemPtr<T> m_Pointer; ///< Pointer to the object type
        CReferenceCount m_RefCount; ///< Referenced count object
    private:
        SSharedObject(const SSharedObject &inOther);
        SSharedObject &operator=(const SSharedObject &inOther);
    };

    SSharedObject *m_Object;

public:
    // Constructor
    // May use assignment to aquire a shared pointer object
    CSharedPtr()
        : m_Object(NULL)
    {
    }

    // Copy constructor
    // Starts ownership of the pointer
    CSharedPtr(T *inPointer)
        : m_Object(NULL)
    {
        if (inPointer) {
            Assign(new SSharedObject(inPointer));
        }
    }

    // Copy constructor
    // Shares ownership of the pointer
    CSharedPtr(const CSharedPtr<T> &inSource)
        : m_Object(NULL)
    {
        Assign(inSource.m_Object);
    }

    // Destructor
    // Deletes the pointer when the ref count is down to zero
    ~CSharedPtr() { Release(); }

    // Assignment operator
    // Uses the same pointer and increments the ref count
    CSharedPtr &operator=(const CSharedPtr<T> &inSource)
    {
        Assign(inSource.m_Object);
        return *this;
    }

    // Assignment operator
    // Starts ownership of the pointer
    // Do not use this on a pointer that is already shared!!!!
    CSharedPtr &operator=(T *inPointer)
    {
        if (inPointer != NULL) {
            Assign(new SSharedObject(inPointer));
        } else // inPointer == NULL
        {
            Release();
        }

        return *this;
    }

    /**
      *	DANGEROUS!!!  This will simply change all of the ptrs that are shared
      *	from this one instance.  Not recommended, as it changes an object's data from
      *	underneath the object without its knowledge...
      *	@param inNewPtr the new hotness.
      *	@param outOldPtr the old and busted.
      */
    /*
    void ReplacePtr( T* inNewPtr, T*& outOldPtr )
    {
            UIC_THROWNULL( m_Object, E_FAIL );
            outOldPtr = m_Object->m_Pointer;
            m_Object->m_Pointer = inNewPtr;
            //And the damage has been done...
    }
    */
    void SwapPtr(CSharedPtr<T> &inSwapPtr)
    {
        UIC_THROWNULL(m_Object, E_FAIL);
        UIC_THROWNULL(inSwapPtr.m_Object, E_FAIL);

        // If the incoming ptr is not us
        if (&inSwapPtr != this) {
            CAutoMemPtr<T> theHoldPtr;

            // Swap the pointers underneath.
            theHoldPtr = m_Object->m_Pointer;
            m_Object->m_Pointer = inSwapPtr.m_Object->m_Pointer;
            inSwapPtr.m_Object->m_Pointer = theHoldPtr;
        }
    }

    // Operator overloads
    operator T *() const
    {
        if (m_Object) {
            return m_Object->m_Pointer;
        }
        return NULL;
    }

    T *Get() const
    {
        if (m_Object) {
            return m_Object->m_Pointer;
        }
        return NULL;
    }

    T *operator->() const
    {
        if (m_Object) {
            return m_Object->m_Pointer;
        }
        return NULL;
    }

    // Comparison
    bool operator==(const CSharedPtr<T> &inSource) const
    {
        if (m_Object != NULL && inSource.m_Object != NULL) {
            return m_Object->m_Pointer == inSource.m_Object->m_Pointer;
        }
        return m_Object == inSource.m_Object;
    }

    // Comparison
    bool operator==(T *inPointer) const
    {
        if (m_Object) {
            return (m_Object->m_Pointer == inPointer);
        }
        return inPointer == NULL;
    }

    // Comparison
    bool operator!=(const CSharedPtr<T> &inSource) const
    {
        if (m_Object != NULL && inSource.m_Object != NULL) {
            return m_Object->m_Pointer != inSource.m_Object->m_Pointer;
        }
        return m_Object != inSource.m_Object;
    }

    // Comparison
    bool operator!=(T *inPointer) const
    {
        if (m_Object) {
            return (m_Object->m_Pointer != inPointer);
        }
        return inPointer != NULL;
    }

    //==============================================================================
    /**
     *	Performs a deep copy of the contained pointer.
     */
    void Copy(const CSharedPtr<T> &inSource)
    {
        // Throw on NULL
        UIC_THROWFALSE(m_Object != NULL && inSource.m_Object != NULL, E_FAIL);
        if (m_Object != inSource.m_Object) {
            *m_Object->m_Pointer = *inSource.m_Object->m_Pointer;
        }
    }

    //==============================================================================
    /**
    *	Performs a deep copy of the contained pointer.
    *
    *	Assumes the pointer has a method named "Clone" that returns a newly
    *	allocated copy of the pointer to the class.
    *
    *	@return	The cloned pointer.
    */
    T *Clone()
    {
        T *theClone = NULL;
        if (m_Object->m_Pointer)
            theClone = m_Object->m_Pointer->Clone();
        return theClone;
    }

    bool IsNull() { return (m_Object == NULL); }

    // Reference count methods
    bool IsReferenced() { return ((m_Object) ? (m_Object->m_RefCount.IsReferenced()) : false); }
    bool IsUnique() { return ((m_Object) ? (m_Object->m_RefCount.IsUnique()) : true); }
    long Count() { return ((m_Object) ? (m_Object->m_RefCount.Count()) : 0); }

protected:
    // Safe way to add references
    void Add()
    {
        // Throw on NULL
        if (m_Object)
            m_Object->m_RefCount.Add();
    }

    // Safe way to release references
    void Release()
    {
        // Throw on NULL
        if (m_Object) {
            long theRefCount = m_Object->m_RefCount.Release();
            if (theRefCount <= 0) {
                if (theRefCount < 0)
                    throw;

                delete m_Object;
            }
        }
        m_Object = NULL;
    }

    // Assigns the smart pointer and ref counts it
    void Assign(SSharedObject *inObject)
    {
        if (m_Object != inObject) {
            if (m_Object)
                Release();
            // Postcondition: m_Object is NULL

            m_Object = inObject;

            // Use the passed in ref count
            if (m_Object) {
                // Increment the reference count
                Add();
            }
        }
    }
};

} // namespace Q3DStudio

#endif // __UICSHAREDPTR_H__
