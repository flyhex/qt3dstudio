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
#ifndef SELECTED_VALUE_IMPL_H
#define SELECTED_VALUE_IMPL_H
#include "SelectedValue.h"
#include "foundation/Qt3DSDiscriminatedUnion.h"
#include "Qt3DSDMHandles.h"
#include "foundation/Qt3DSUnionCast.h"

namespace Q3DStudio {

struct SSlideInstanceWrapper
{
    qt3dsdm::Qt3DSDMInstanceHandle m_Instance;
    qt3dsdm::Qt3DSDMSlideHandle m_Slide;
#ifdef _WIN32
    // We have a multiple unions which needs to be big enough
    // This is strange on 32 bit systems but wrong on 64 bit.
    // Align the structure to make it big enough.
    size_t m_Padding;
#endif
    SSlideInstanceWrapper() {}
    SSlideInstanceWrapper(qt3dsdm::Qt3DSDMInstanceHandle inst, qt3dsdm::Qt3DSDMSlideHandle slide)
        : m_Instance(inst)
        , m_Slide(slide)
    {
    }

    bool operator==(const SSlideInstanceWrapper &inOther) const
    {
        return m_Instance == inOther.m_Instance && m_Slide == inOther.m_Slide;
    }
};

struct SSlideInsertionWrapper
{
    void *m_UserData;
    SSlideInsertionWrapper(void *ud = NULL)
        : m_UserData(ud)
    {
    }

    bool operator==(const SSlideInsertionWrapper &inOther) const
    {
        return m_UserData == inOther.m_UserData;
    }
};

template <typename dtype>
struct SSelectedValueTypeMap
{
};

template <>
struct SSelectedValueTypeMap<qt3dsdm::Qt3DSDMInstanceHandle>
{
    static SelectedValueTypes GetType() { return SelectedValueTypes::Instance; }
};

template <>
struct SSelectedValueTypeMap<SSlideInstanceWrapper>
{
    static SelectedValueTypes GetType() { return SelectedValueTypes::Slide; }
};

template <>
struct SSelectedValueTypeMap<qt3dsdm::Qt3DSDMGuideHandle>
{
    static SelectedValueTypes GetType() { return SelectedValueTypes::Guide; }
};

template <>
struct SSelectedValueTypeMap<SSlideInsertionWrapper>
{
    static SelectedValueTypes GetType() { return SelectedValueTypes::SlideInsertion; }
};

template <>
struct SSelectedValueTypeMap<qt3dsdm::TInstanceHandleList>
{
    static SelectedValueTypes GetType() { return SelectedValueTypes::MultipleInstances; }
};

struct SSelectedValueTraits
{
    typedef SelectedValueTypes TIdType;
    enum {
        TBufferSize = sizeof(SSlideInstanceWrapper),
    };

    static TIdType getNoDataId() { return SelectedValueTypes::Unknown; }

    template <typename TDataType>
    static TIdType getType()
    {
        return SSelectedValueTypeMap<TDataType>().GetType();
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
    {
        switch (inType) {
        case SelectedValueTypes::Instance:
            return inVisitor(*qt3ds::NVUnionCast<qt3dsdm::Qt3DSDMInstanceHandle *>(inData));
        case SelectedValueTypes::Slide:
            return inVisitor(*qt3ds::NVUnionCast<SSlideInstanceWrapper *>(inData));
        case SelectedValueTypes::Guide:
            return inVisitor(*qt3ds::NVUnionCast<qt3dsdm::Qt3DSDMGuideHandle *>(inData));
        case SelectedValueTypes::SlideInsertion:
            return inVisitor(*qt3ds::NVUnionCast<void **>(inData));
        case SelectedValueTypes::MultipleInstances:
            return inVisitor(*qt3ds::NVUnionCast<qt3dsdm::TInstanceHandleList *>(inData));
        default:
            QT3DS_ASSERT(false);
        case SelectedValueTypes::Unknown:
            return inVisitor();
        }
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
    {
        switch (inType) {
        case SelectedValueTypes::Instance:
            return inVisitor(*qt3ds::NVUnionCast<const qt3dsdm::Qt3DSDMInstanceHandle *>(inData));
        case SelectedValueTypes::Slide:
            return inVisitor(*qt3ds::NVUnionCast<const SSlideInstanceWrapper *>(inData));
        case SelectedValueTypes::Guide:
            return inVisitor(*qt3ds::NVUnionCast<const qt3dsdm::Qt3DSDMGuideHandle *>(inData));
        case SelectedValueTypes::SlideInsertion:
            return inVisitor(*qt3ds::NVUnionCast<const void **>(inData));
        case SelectedValueTypes::MultipleInstances:
            return inVisitor(*qt3ds::NVUnionCast<const qt3dsdm::TInstanceHandleList *>(inData));
        default:
            QT3DS_ASSERT(false);
        case SelectedValueTypes::Unknown:
            return inVisitor();
        }
    }
};

typedef qt3ds::foundation::
    DiscriminatedUnion<qt3ds::foundation::
                           DiscriminatedUnionGenericBase<SSelectedValueTraits,
                                                         SSelectedValueTraits::TBufferSize>,
                       SSelectedValueTraits::TBufferSize>
        TSelectedValueUnionType;

struct SSelectedValue : public TSelectedValueUnionType
{

    SSelectedValue() {}
    SSelectedValue(const SSelectedValue &other)
        : TSelectedValueUnionType(static_cast<const TSelectedValueUnionType &>(other))
    {
    }
    SSelectedValue(qt3dsdm::Qt3DSDMInstanceHandle val)
        : TSelectedValueUnionType(val)
    {
    }
    SSelectedValue(SSlideInstanceWrapper val)
        : TSelectedValueUnionType(val)
    {
    }
    SSelectedValue(qt3dsdm::Qt3DSDMGuideHandle val)
        : TSelectedValueUnionType(val)
    {
    }
    SSelectedValue(SSlideInsertionWrapper val)
        : TSelectedValueUnionType(val)
    {
    }
    SSelectedValue(const qt3dsdm::TInstanceHandleList &val)
        : TSelectedValueUnionType(val)
    {
    }

    SSelectedValue &operator=(const SSelectedValue &other)
    {
        TSelectedValueUnionType::operator=(static_cast<const TSelectedValueUnionType &>(other));
        return *this;
    }

    bool empty() const { return getType() == SelectedValueTypes::Unknown; }

    operator bool() const { return !empty(); }

    qt3dsdm::TInstanceHandleList GetSelectedInstances() const
    {
        if (getType() == SelectedValueTypes::Instance) {
            qt3dsdm::TInstanceHandleList retval;
            retval.push_back(getData<qt3dsdm::Qt3DSDMInstanceHandle>());
            return retval;
        } else if (getType() == SelectedValueTypes::MultipleInstances)
            return getData<qt3dsdm::TInstanceHandleList>();
        return qt3dsdm::TInstanceHandleList();
    }
};
}

#endif
