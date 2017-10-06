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

#ifndef INCLUDED_GENERIC_FUNCTOR_H
#define INCLUDED_GENERIC_FUNCTOR_H 1

#pragma once

#define CREATE_LISTENER(listenerClass, implementorClass, function)                                 \
    new CSpecific##listenerClass<implementorClass>(this, &implementorClass::function)

#define GENERIC_FUNCTOR(FunctorName, FunctionName)                                                 \
    class FunctorName                                                                              \
    {                                                                                              \
    public:                                                                                        \
        virtual void FunctionName() = 0;                                                           \
        virtual ~FunctorName() {};                                                                 \
    };                                                                                             \
                                                                                                   \
    template <class TClass>                                                                        \
    class CSpecific##FunctorName : public FunctorName                                              \
    {                                                                                              \
    public:                                                                                        \
        typedef void (TClass::*TFunction)();                                                       \
        CSpecific##FunctorName(TClass *inObject, TFunction inFunction)                             \
        {                                                                                          \
            m_Object = inObject;                                                                   \
            m_Function = inFunction;                                                               \
        }                                                                                          \
                                                                                                   \
        virtual void FunctionName() { (m_Object->*m_Function)(); }                                 \
                                                                                                   \
        TClass *m_Object;                                                                          \
        void (TClass::*m_Function)();                                                              \
    };

#define GENERIC_FUNCTOR_1(FunctorName, FunctionName, Arg1)                                         \
    class FunctorName                                                                              \
    {                                                                                              \
    public:                                                                                        \
        virtual void FunctionName(Arg1) = 0;                                                       \
        virtual ~FunctorName() {};                                                                 \
    };                                                                                             \
                                                                                                   \
    template <class TClass>                                                                        \
    class CSpecific##FunctorName : public FunctorName                                              \
    {                                                                                              \
    public:                                                                                        \
        typedef void (TClass::*TFunction)(Arg1);                                                   \
        CSpecific##FunctorName(TClass *inObject, void (TClass::*inFunction)(Arg1))                 \
        {                                                                                          \
            m_Object = inObject;                                                                   \
            m_Function = inFunction;                                                               \
        }                                                                                          \
                                                                                                   \
        virtual void FunctionName(Arg1 inArg1) { (m_Object->*m_Function)(inArg1); }                \
                                                                                                   \
        TClass *m_Object;                                                                          \
        void (TClass::*m_Function)(Arg1);                                                          \
    };

#define GENERIC_FUNCTOR_2(FunctorName, FunctionName, Arg1, Arg2)                                   \
    class FunctorName                                                                              \
    {                                                                                              \
    public:                                                                                        \
        virtual void FunctionName(Arg1, Arg2) = 0;                                                 \
        virtual ~FunctorName() {};                                                                 \
    };                                                                                             \
                                                                                                   \
    template <class TClass>                                                                        \
    class CSpecific##FunctorName : public FunctorName                                              \
    {                                                                                              \
    public:                                                                                        \
        CSpecific##FunctorName(TClass *inObject, void (TClass::*inFunction)(Arg1, Arg2))           \
        {                                                                                          \
            m_Object = inObject;                                                                   \
            m_Function = inFunction;                                                               \
        }                                                                                          \
                                                                                                   \
        virtual void FunctionName(Arg1 inArg1, Arg2 inArg2)                                        \
        {                                                                                          \
            (m_Object->*m_Function)(inArg1, inArg2);                                               \
        }                                                                                          \
                                                                                                   \
        TClass *m_Object;                                                                          \
        void (TClass::*m_Function)(Arg1, Arg2);                                                    \
    };
#endif // INCLUDED_GENERIC_FUNCTOR_H
