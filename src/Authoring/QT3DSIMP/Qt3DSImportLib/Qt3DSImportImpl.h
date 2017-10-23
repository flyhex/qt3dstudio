/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#ifndef QT3DS_IMPORT_IMPL_H
#define QT3DS_IMPORT_IMPL_H
#include "Qt3DSImport.h"
#include "Qt3DSImportContainers.h"
#include "EASTL/functional.h"

namespace eastl {
template <>
struct hash<qt3dsdm::ComposerPropertyNames::Enum>
{
    size_t operator()(qt3dsdm::ComposerPropertyNames::Enum val) const
    {
        return hash<qt3ds::QT3DSU32>()((qt3ds::QT3DSU32)val);
    }
};
}

namespace qt3dsimp {

struct Instance;
inline TIMPHandle toHdl(Instance *inst)
{
    return (QT3DSU64)inst;
}

inline Instance *fromHdl(TIMPHandle hdl)
{
    return reinterpret_cast<Instance *>(reinterpret_cast<void *>(hdl));
}

struct Instance : public InstanceDesc
{
    ImportHashMap<ComposerPropertyNames::Enum, SInternValue> m_PropertyValues;
    eastl::vector<TCharPtr> m_Children;
    bool m_Valid;

public:
    Instance()
        : m_Valid(true)
    {
        m_Handle = toHdl(this);
    }
    Instance(const InstanceDesc &desc)
        : InstanceDesc(desc)
        , m_Valid(true)
    {
        m_Handle = toHdl(this);
    }
    void SetPropertyValue(ComposerPropertyNames::Enum name, const SInternValue &val)
    {
        m_PropertyValues[name] = val;
    }
    void SetPropertyValues(NVConstDataRef<PropertyValue> values, qt3dsdm::IStringTable &inStringTable)
    {
        m_PropertyValues.clear();
        QT3DSIMP_FOREACH(idx, values.size())
        SetPropertyValue(values[idx].m_Name, SInternValue(values[idx].m_Value, inStringTable));
    }
    void AddChild(Instance *child)
    {
        if (m_Valid == false)
            child->m_Valid = false;
        QT3DSIMP_FOREACH(idx, (QT3DSU32)m_Children.size())
        {
            // Pointer comparison is fine as both strings are just in the
            // string table.
            if (m_Children[idx] == child->m_Id)
                return;
        }
        QT3DS_ASSERT(child->m_Parent == 0);
        child->m_Parent = toHdl(this);
        m_Children.push_back(child->m_Id);
    }
    void AddChild(Instance *child, Instance *inInsertAfter)
    {
        if (m_Valid == false)
            child->m_Valid = false;
        QT3DSIMP_FOREACH(idx, (QT3DSU32)m_Children.size())
        {
            // Pointer comparison is fine as both strings are just in the
            // string table.
            if (m_Children[idx] == child->m_Id)
                return;
        }
        QT3DS_ASSERT(child->m_Parent == 0);
        child->m_Parent = toHdl(this);
        eastl::vector<TCharPtr>::iterator theIter =
            std::find(m_Children.begin(), m_Children.end(), inInsertAfter->m_Id);
        if (theIter != m_Children.end())
            ++theIter;
        m_Children.insert(theIter, child->m_Id);
    }
    void MarkInvalid() { m_Valid = false; }
};

struct AnimationId
{
    TIMPHandle m_Instance;
    TCharPtr m_Property;
    QT3DSU32 m_SubPropIndex;
    AnimationId(TIMPHandle inst, TCharPtr p, QT3DSU32 spi)
        : m_Instance(inst)
        , m_Property(p)
        , m_SubPropIndex(spi)
    {
    }
    AnimationId()
        : m_Instance(0)
        , m_Property(0)
        , m_SubPropIndex(0)
    {
    }
};

inline Animation *CreateAnimation(TCharPtr instance, TCharPtr propName, QT3DSU32 spi,
                                  EAnimationType bufType, NVConstDataRef<QT3DSF32> values)
{
    QT3DSU32 animBufSize = sizeof(Animation);
    QT3DSU32 valueSize = values.size() * sizeof(QT3DSF32);
    QT3DSU8 *memBuf = (QT3DSU8 *)malloc(animBufSize + valueSize);
    QT3DSF32 *framePtr = (QT3DSF32 *)(memBuf + animBufSize);
    memCopy(framePtr, values.begin(), valueSize);
    values = toConstDataRef(framePtr, values.size());
    Animation *newBuf = new (memBuf) Animation(instance, propName, spi, bufType, values);
    return newBuf;
}
}

#endif
