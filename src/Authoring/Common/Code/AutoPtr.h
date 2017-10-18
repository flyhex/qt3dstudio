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
#ifndef AMXSMARTPTRH
#define AMXSMARTPTRH
#include <memory>
#include "UICExceptions.h"
#include "PlatformMacros.h"
#ifdef WIN32
#pragma warning(disable : 4284)
#endif

namespace Q3DStudio {
//==========================================================================
/**
  *	@class CAutoPtr
  * Class to arrange aquisition, release, and destruction of different pointer types.
  */
template <class handler, class T>
class CAutoPtr
{
public:
    //=======================================================================
    /**
      *	CTOR-Sets pointer value to null.
      */
    CAutoPtr()
        : m_T(NULL)
    {
    }
    //=======================================================================
    /**
      *	Sets pointer value to the value passed in.
      *	@param inT the value to set the pointer to.
      */
    CAutoPtr(T *inT)
        : m_T(handler::Acquire(inT))
    {
    }
    //=======================================================================
    /**
      *	Passes pointer from source object to this object.
      */
    CAutoPtr(const CAutoPtr<handler, T> &inSource) { m_T = handler::Acquire(inSource.Release()); }
    //=======================================================================
    /**
      *	calls handler::Destroy the m_t member variable.
      */
    ~CAutoPtr()
    {
        if (m_T) {
            handler::Destroy(m_T);
            m_T = NULL;
        }
    }
    //=======================================================================
    /**
      * This objects 1. Destroys old pointer 2. Acquire's new value.
      *	This object takes ownership of the inT parameter.
      *
      *	@param inT The pointer to set the value to.
      *	@return The returned value.
      */
    T *operator=(T *inT)
    {
        if (m_T == inT) {
            return m_T;
        }
        if (m_T != NULL) {
            handler::Destroy(m_T);
            m_T = NULL;
        }
        m_T = handler::Acquire(inT);
        return m_T;
    }
    //=======================================================================
    /**
      *	Passes ownership from the passed in class to this class.
      */
    CAutoPtr<handler, T> &operator=(const CAutoPtr<handler, T> &other)
    {
        if (&other != this) {
            m_T = handler::Acquire(other.Release());
        }

        return *this;
    }
    //=======================================================================
    /**
      *	Returns pointer-to-T.
      */
    operator T *() const { return m_T; }
    //=======================================================================
    /**
      *	Throws if m_t is null, because you are essentially guaranteeing
      *	a dereference if a null parameter.
      *	@return reference-T.
      */
    T &operator*() const
    {
        QT3DS_THROWNULL(m_T, -1);
        return *m_T;
    }
    //=======================================================================
    /**
      *	Returns pointer-to-T, throws if T is null.
      */
    T *operator->() const
    {
        QT3DS_THROWNULL(m_T, -1);
        return m_T;
    }
    //=======================================================================
    /**
      *	@return Address of T, throws if m_t isn't null.
      *
    T** operator&() const
    {
            //This usually indicates an error
            //QT3DS_THROWFALSE( m_T == NULL, AUTO_PTR_ERROR );
            return &m_T;
    }
    CAutoPtr< handler, T >* operator&()
    {
            return this;
    }*/

    //=======================================================================
    /**
      *	Releases the object from this objects control.  This is akin to telling
      *	the auto pointer that it is no longer responsible for the pointer.
      *	@return The value of m_T;
      */
    T *Release() const
    {
        T *theTemp = m_T;
        handler::Release(m_T);
        m_T = NULL;
        return theTemp;
    }

    //=======================================================================
    /**
      *	@return true of other == m_T.
      */
    bool operator==(T *other) const
    {
        if (m_T == other) {
            return true;
        }
        return false;
    }

    //=======================================================================
    /**
      *	@return true if other != m_T.
      */
    bool operator!=(T *other) const
    {
        if (m_T == other) {
            return false;
        }
        return true;
    }
    mutable T *m_T; ///< Pointer to class
};
//==========================================================================
/**
  *	@class CDeleteHandler
  * Handles the aquisition, release and destruction of classes that need to
  * to be 'delete'd.
  */
template <class T>
class CDeleteHandler
{
public:
    //=======================================================================
    /**
      *	Does aquisition action on pointer (in this case, nothing).
      */
    static inline T *Acquire(T *inT) { return inT; }
    //=======================================================================
    /**
      *	Does release action on pointer (in this case, nothing).
      */
    static inline T *Release(T *inT) { return inT; }
    //=======================================================================
    /**
      *	Does destruction action on pointer (in this case, delete's the pointer).
      */
    static inline void Destroy(T *inT) { delete inT; }
};

//==========================================================================
/**
  *	@class CArrayDeleteHandler
  * Handles the aquisition, release and destruction of classes that need to
  * to be 'delete[]'d.
  */
template <class T>
class CArrayDeleteHandler
{
public:
    //=======================================================================
    /**
      *	Does aquisition action on pointer (in this case, nothing).
      */
    static inline T *Acquire(T *inT) { return inT; }
    //=======================================================================
    /**
      *	Does release action on pointer (in this case, nothing).
      */
    static inline T *Release(T *inT) { return inT; }
    //=======================================================================
    /**
      *	Does destruction action on pointer (in this case, delete[]'s the pointer).
      */
    static inline void Destroy(T *inT) { delete[] inT; }
};

template <class T>
class CAutoMemPtr : public CAutoPtr<CDeleteHandler<T>, T>
{
public:
    CAutoMemPtr()
        : CAutoPtr<CDeleteHandler<T>, T>()
    {
    }
    CAutoMemPtr(T *inT)
        : CAutoPtr<CDeleteHandler<T>, T>(inT)
    {
    }
    CAutoMemPtr(const CAutoMemPtr<T> &inSource)
        : CAutoPtr<CDeleteHandler<T>, T>(inSource)
    {
    }
    T *operator=(T *inT) { return CAutoPtr<CDeleteHandler<T>, T>::operator=(inT); }
    T *Release() const { return CAutoPtr<CDeleteHandler<T>, T>::Release(); }
};

template <class T>
class CAutoArrayPtr : public CAutoPtr<CArrayDeleteHandler<T>, T>
{
public:
    CAutoArrayPtr()
        : CAutoPtr<CArrayDeleteHandler<T>, T>()
    {
    }
    CAutoArrayPtr(T *inT)
        : CAutoPtr<CArrayDeleteHandler<T>, T>(inT)
    {
    }
    CAutoArrayPtr(const CAutoArrayPtr<T> &inSource)
        : CAutoPtr<CArrayDeleteHandler<T>, T>(inSource)
    {
    }
    T *operator=(T *inT) { return CAutoPtr<CArrayDeleteHandler<T>, T>::operator=(inT); }
    T *Release() const { return CAutoPtr<CArrayDeleteHandler<T>, T>::Release(); }
};
}

#endif
