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
#include "MemoryObject.h"

//==============================================================================
//	Defines
//==============================================================================

#define DEFINE_MEMORY_OBJECT(class, name)                                                          \
protected:                                                                                         \
    CMemoryObject m_MemoryObject;                                                                  \
                                                                                                   \
public:                                                                                            \
    virtual const char *GetMemoryObjectName() const;                                               \
    virtual const char *GetMemoryObjectDescription() const;                                        \
    virtual long GetMemoryObjectId() const;                                                        \
    virtual Q3DStudio::IMemoryObject *FindMemoryObjectId(long inId) const;                         \
    virtual void RegisterMemoryObject(Q3DStudio::IMemoryObject *inMemoryTracker);                  \
    virtual void UnregisterMemoryObject(Q3DStudio::IMemoryObject *inMemoryTracker);                \
    virtual void RegisterMemoryObjectListener(                                                     \
        Q3DStudio::IMemoryObjectListener *inMemoryObjectListener);                                 \
    virtual void UnregisterMemoryObjectListener(                                                   \
        Q3DStudio::IMemoryObjectListener *inMemoryObjectListener);                                 \
    virtual long GetMemoryObjectBase() const;                                                      \
    virtual long GetMemoryObjectUsage() const;                                                     \
    virtual long GetMemoryObjectTotal() const;                                                     \
    virtual const Q3DStudio::IMemoryObject::TObjectListIt GetMemoryObjectBegin() const;            \
    virtual const Q3DStudio::IMemoryObject::TObjectListIt GetMemoryObjectEnd() const;              \
    virtual const Q3DStudio::IMemoryObject::TDetailListIt GetMemoryObjectDetailBegin() const;      \
    virtual const Q3DStudio::IMemoryObject::TDetailListIt GetMemoryObjectDetailEnd() const;        \
    virtual void SetMemoryObjectParent(Q3DStudio::IMemoryObject *inMemoryObject);                  \
    void SetSizeFunctor(CMemoryObject::TSizeFunctor *inFunctor);                                   \
    void SetDescFunctor(CMemoryObject::TDescFunctor *inFunctor);

#define IMPLEMENT_MEMORY_OBJECT(class, name)                                                       \
    const char *class ::GetMemoryObjectName() const                                                \
    {                                                                                              \
        return m_MemoryObject.GetMemoryObjectName();                                               \
    }                                                                                              \
    const char *class ::GetMemoryObjectDescription() const                                         \
    {                                                                                              \
        return m_MemoryObject.GetMemoryObjectDescription();                                        \
    }                                                                                              \
    long class ::GetMemoryObjectId() const { return m_MemoryObject.GetMemoryObjectId(); }          \
    Q3DStudio::IMemoryObject *class ::FindMemoryObjectId(long inId) const                          \
    {                                                                                              \
        return m_MemoryObject.FindMemoryObjectId(inId);                                            \
    }                                                                                              \
    void class ::RegisterMemoryObject(Q3DStudio::IMemoryObject *inMemoryTracker)                   \
    {                                                                                              \
        return m_MemoryObject.RegisterMemoryObject(inMemoryTracker);                               \
    }                                                                                              \
    void class ::UnregisterMemoryObject(Q3DStudio::IMemoryObject *inMemoryTracker)                 \
    {                                                                                              \
        return m_MemoryObject.UnregisterMemoryObject(inMemoryTracker);                             \
    }                                                                                              \
    void class ::RegisterMemoryObjectListener(                                                     \
        Q3DStudio::IMemoryObjectListener *inMemoryObjectListener)                                  \
    {                                                                                              \
        return m_MemoryObject.RegisterMemoryObjectListener(inMemoryObjectListener);                \
    }                                                                                              \
    void class ::UnregisterMemoryObjectListener(                                                   \
        Q3DStudio::IMemoryObjectListener *inMemoryObjectListener)                                  \
    {                                                                                              \
        return m_MemoryObject.UnregisterMemoryObjectListener(inMemoryObjectListener);              \
    }                                                                                              \
    long class ::GetMemoryObjectBase() const { return m_MemoryObject.GetMemoryObjectBase(); }      \
    long class ::GetMemoryObjectUsage() const { return m_MemoryObject.GetMemoryObjectUsage(); }    \
    long class ::GetMemoryObjectTotal() const { return m_MemoryObject.GetMemoryObjectTotal(); }    \
    const Q3DStudio::IMemoryObject::TObjectListIt class ::GetMemoryObjectBegin() const             \
    {                                                                                              \
        return m_MemoryObject.GetMemoryObjectBegin();                                              \
    }                                                                                              \
    const Q3DStudio::IMemoryObject::TObjectListIt class ::GetMemoryObjectEnd() const               \
    {                                                                                              \
        return m_MemoryObject.GetMemoryObjectEnd();                                                \
    }                                                                                              \
    const Q3DStudio::IMemoryObject::TDetailListIt class ::GetMemoryObjectDetailBegin() const       \
    {                                                                                              \
        return m_MemoryObject.GetMemoryObjectDetailBegin();                                        \
    }                                                                                              \
    const Q3DStudio::IMemoryObject::TDetailListIt class ::GetMemoryObjectDetailEnd() const         \
    {                                                                                              \
        return m_MemoryObject.GetMemoryObjectDetailEnd();                                          \
    }                                                                                              \
    void class ::SetMemoryObjectParent(Q3DStudio::IMemoryObject *inMemoryObject)                   \
    {                                                                                              \
        return m_MemoryObject.SetMemoryObjectParent(inMemoryObject);                               \
    }                                                                                              \
    void class ::SetSizeFunctor(CMemoryObject::TSizeFunctor *inFunctor)                            \
    {                                                                                              \
        return m_MemoryObject.SetSizeFunctor(inFunctor);                                           \
    }                                                                                              \
    void class ::SetDescFunctor(CMemoryObject::TDescFunctor *inFunctor)                            \
    {                                                                                              \
        return m_MemoryObject.SetDescFunctor(inFunctor);                                           \
    }

#define CONSTRUCT_MEMORY_OBJECT(class, name) m_MemoryObject(#name, sizeof(class), this)
