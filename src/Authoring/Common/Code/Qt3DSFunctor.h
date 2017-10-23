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

// EngineeringTask (SDJ 08.09.05) This file is a huge mess!
// It really should be cleaned up and consolidated to be a useful tool.

//==============================================================================
//	Prefix
//==============================================================================
#ifndef _QT3DS_FUNCTOR_
#define _QT3DS_FUNCTOR_

#include <typeinfo>

//==============================================================================
//	Includes
//==============================================================================

#include "SafeQueue.h"

namespace Q3DStudio {

//==============================================================================
//	Class
//==============================================================================

//==========================================================================
/**
 *		@class CFunctorObjectBase
 *		Abstract base class, used as the base storage pointer for the
 *		Functor Queue.
 */
class CFunctorObjectBase
{
public:
    const std::type_info *m_TypeInfo;

public:
    virtual bool Execute() { return false; }
    virtual bool operator==(CFunctorObjectBase &) = 0;
};

//==========================================================================
/**
 *		@class CFunctorObjectBase
 *		Abstract base class, used as the base storage pointer for the
 *		Functor Queue.
 */
template <typename TFirstArg>
class CFunctorObjectParam1Base
{
public:
    virtual bool Execute(TFirstArg) { return false; }
};

//==========================================================================
/**
 *		@class CFunctorObjectReturnBase
 */
template <typename TReturnType>
class CFunctorObjectReturnBase : public CFunctorObjectBase
{
public:
    virtual TReturnType ExecuteReturn() = 0;
};

//==========================================================================
/**
 *		@class CFunctorObjectReturnBase1
 */
template <typename TReturnType, typename TFirstArg>
class CFunctorObjectReturnBase1 : public CFunctorObjectBase
{
public:
    virtual TReturnType ExecuteReturn(TFirstArg) = 0;
};

//==========================================================================
/**
 *		@class CFunctorObject0
 *		Template class that stores the data needed to execute a functor
 *		with a single parameter.
 */
template <typename TObjectType>
class CFunctorObject0 : public CFunctorObjectBase
{
protected:
    typedef void (TObjectType::*TMethod)();

    TObjectType *m_Object; ///< The object on which to execute the methof
    TMethod m_Method; ///< The single parameter method to execute

public:
    // Constructor
    CFunctorObject0(TObjectType *inObject, TMethod inMethod)
        : m_Object(inObject)
        , m_Method(inMethod)
    {
        m_TypeInfo = &typeid(TObjectType);
    }

    //=======================================================================
    /**
     *		Insert a functor object into the queue.
     *
     *		@param	inObjectBase
     *				The fucntor object to insert into the queue.
     */
    virtual bool Execute()
    {
        bool theReturn = true;

        // Try / Catch the execution since param objects could go
        // out of scope if not managed properly

        try {
            if (m_Object && m_Method) {
                (m_Object->*(m_Method))();
            }
        } catch (...) {
            theReturn = false;
        }

        return theReturn;
    }

    //=======================================================================
    /**
     *		Compare the functors
     *
     *		@param	inFunctor
     *				The fucntor object to comapare.
     */
    virtual bool operator==(CFunctorObjectBase &inFunctor)
    {
        // Compare the Typinfo for the Callbacks. since we don't want to type cast just anything.
        if (*m_TypeInfo == *inFunctor.m_TypeInfo) {
            // This just does the type cast and the compare/
            return Compare(static_cast<Q3DStudio::CFunctorObject0<TObjectType> *>(&inFunctor));
        } else {
            // return false if we are the wrong type.
            return false;
        }
    }

protected:
    bool Compare(Q3DStudio::CFunctorObject0<TObjectType> *inFunctor)
    {
        // Don't compare the Args
        return (m_Object == (*inFunctor).m_Object) && (m_Method == (*inFunctor).m_Method);
    }
};

//==========================================================================
/**
 *		@class CFunctorObjectReturn0
 */
template <typename TReturnType, typename TObjectType>
class CFunctorObjectReturn0 : public CFunctorObjectReturnBase<TReturnType>
{
protected:
    typedef TReturnType (TObjectType::*TMethod)();

    TObjectType *m_Object; ///< The object on which to execute the methof
    TMethod m_Method; ///< The single parameter method to execute

public:
    // Constructor
    CFunctorObjectReturn0(TObjectType *inObject, TMethod inMethod)
        : m_Object(inObject)
        , m_Method(inMethod)
    {
        CFunctorObjectReturnBase<TReturnType>::m_TypeInfo = &typeid(TObjectType);
    }

    //=======================================================================
    /**
     *		Insert a functor object into the queue.
     *
     *		@param	inObjectBase
     *				The fucntor object to insert into the queue.
     */
    virtual TReturnType ExecuteReturn() { return (m_Object->*(m_Method))(); }

    //=======================================================================
    /**
     *		Compare the functors
     *
     *		@param	inFunctor
     *				The fucntor object to comapare.
     */
    virtual bool operator==(CFunctorObjectBase &inFunctor)
    {
        // Compare the Typinfo for the Callbacks. since we don't want to type cast just anything.
        if (*CFunctorObjectReturnBase<TReturnType>::m_TypeInfo == *inFunctor.m_TypeInfo) {
            // This just does the type cast and the compare/
            return Compare(
                static_cast<Q3DStudio::CFunctorObjectReturn0<TReturnType, TObjectType> *>(
                    &inFunctor));
        } else {
            // return false if we are the wrong type.
            return false;
        }
    }

protected:
    bool Compare(Q3DStudio::CFunctorObjectReturn0<TReturnType, TObjectType> *inFunctor)
    {
        // Don't compare the Args
        return (m_Object == (*inFunctor).m_Object) && (m_Method == (*inFunctor).m_Method);
    }
};

//==========================================================================
/**
 *		@class CFunctorObjectReturn1
 */
template <typename TReturnType, typename TObjectType, typename TFirstArg>
class CFunctorObjectReturn1 : public CFunctorObjectReturnBase1<TReturnType, TFirstArg>
{
protected:
    typedef TReturnType (TObjectType::*TMethod)(TFirstArg);

    TObjectType *m_Object; ///< The object on which to execute the method
    TMethod m_Method; ///< The single parameter method to execute

public:
    // Constructor
    CFunctorObjectReturn1(TObjectType *inObject, TMethod inMethod)
        : m_Object(inObject)
        , m_Method(inMethod)
    {
        CFunctorObjectReturnBase1<TReturnType, TFirstArg>::m_TypeInfo = &typeid(TObjectType);
    }

    //=======================================================================
    /**
     *		Insert a functor object into the queue.
     *
     *		@param	inObjectBase
     *				The fucntor object to insert into the queue.
     */
    virtual TReturnType ExecuteReturn(TFirstArg inFirstArg)
    {
        return (m_Object->*(m_Method))(inFirstArg);
    }

    //=======================================================================
    /**
     *		Compare the functors
     *
     *		@param	inFunctor
     *				The fucntor object to comapare.
     */
    virtual bool operator==(CFunctorObjectBase &inFunctor)
    {
        // Compare the Typinfo for the Callbacks. since we don't want to type cast just anything.
        if (*CFunctorObjectReturnBase1<TReturnType, TFirstArg>::m_TypeInfo == *inFunctor.m_TypeInfo) {
            // This just does the type cast and the compare/
            return Compare(static_cast<Q3DStudio::CFunctorObjectReturn1<TReturnType, TObjectType,
                                                                        TFirstArg> *>(&inFunctor));
        } else {
            // return false if we are the wrong type.
            return false;
        }
    }

protected:
    bool Compare(Q3DStudio::CFunctorObjectReturn1<TReturnType, TObjectType, TFirstArg> *inFunctor)
    {
        // Don't compare the Args
        return (m_Object == (*inFunctor).m_Object) && (m_Method == (*inFunctor).m_Method);
    }
};

//==========================================================================
/**
 *		@class CFunctorObject1
 *		Template class that stores the data needed to execute a functor
 *		with a single parameter.
 */
template <typename TObjectType, typename TFirstArg>
class CFunctorObjectParam1 : public CFunctorObjectParam1Base<TFirstArg>
{
protected:
    typedef void (TObjectType::*TMethod)(TFirstArg);

    TObjectType *m_Object; ///< The object on which to execute the method
    TMethod m_Method; ///< The single parameter method to execute

public:
    // Constructor
    CFunctorObjectParam1(TObjectType *inObject, TMethod inMethod)
        : m_Object(inObject)
        , m_Method(inMethod)
    {
    }

    //=======================================================================
    /**
     *		Insert a functor object into the queue.
     *
     *		@param	inObjectBase
     *				The fucntor object to insert into the queue.
     */
    virtual bool Execute(TFirstArg inFirstArg)
    {
        bool theReturn = true;

        // Try / Catch the execution since param objects could go
        // out of scope if not managed properly

        try {
            if (m_Object && m_Method) {
                (m_Object->*(m_Method))(inFirstArg);
            }
        } catch (...) {
            theReturn = false;
        }

        return theReturn;
    }
};

//==========================================================================
/**
 *		@class CFunctorObject1
 *		Template class that stores the data needed to execute a functor
 *		with a single parameter.
 */
template <typename TObjectType, typename TFirstArg>
class CFunctorObject1 : public CFunctorObjectBase
{
protected:
    typedef void (TObjectType::*TMethod)(TFirstArg);

    TObjectType *m_Object; ///< The object on which to execute the methof
    TMethod m_Method; ///< The single parameter method to execute
    TFirstArg m_FirstArg; ///< The value of the single arguement

public:
    // Constructor
    CFunctorObject1(TObjectType *inObject, TMethod inMethod, TFirstArg inFirstArg)
        : m_Object(inObject)
        , m_Method(inMethod)
        , m_FirstArg(inFirstArg)
    {
        m_TypeInfo = &typeid(TObjectType);
    }

    //=======================================================================
    /**
     *		Insert a functor object into the queue.
     *
     *		@param	inObjectBase
     *				The fucntor object to insert into the queue.
     */
    virtual bool Execute()
    {
        bool theReturn = true;

        // Try / Catch the execution since param objects could go
        // out of scope if not managed properly

        try {
            if (m_Object && m_Method) {
                (m_Object->*(m_Method))(m_FirstArg);
            }
        } catch (...) {
            theReturn = false;
        }

        return theReturn;
    }

    //=======================================================================
    /**
     *		Compare the functors
     *
     *		@param	inFunctor
     *				The fucntor object to comapare.
     */
    virtual bool operator==(CFunctorObjectBase &inFunctor)
    {
        // Compare the Typinfo for the Callbacks. since we don't want to type cast just anything.
        if (*m_TypeInfo == *inFunctor.m_TypeInfo) {
            // This just does the type cast and the compare/
            return Compare(
                static_cast<Q3DStudio::CFunctorObject1<TObjectType, TFirstArg> *>(&inFunctor));
        } else {
            // return false if we are the wrong type.
            return false;
        }
    }

protected:
    bool Compare(Q3DStudio::CFunctorObject1<TObjectType, TFirstArg> *inFunctor)
    {
        // Don't compare the Args
        return (m_Object == (*inFunctor).m_Object) && (m_Method == (*inFunctor).m_Method);
    }
};
} // namespace Q3DStudio

#endif // __QT3DS_Functor_H__
