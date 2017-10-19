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
#ifndef UICDMHANDLESH
#define UICDMHANDLESH

#include <QMetaType>

namespace qt3dsdm {

class IDataCore;

class CDataModelHandle
{
    int m_Handle;

public:
    inline CDataModelHandle(int inHandle = 0);
    inline CDataModelHandle(const CDataModelHandle &other);
    virtual ~CDataModelHandle();

    // Operators
    inline CDataModelHandle &operator=(const CDataModelHandle &other);
    inline bool operator==(const CDataModelHandle &other) const;
    inline bool operator<(const CDataModelHandle &other) const;
    inline operator int() const { return GetHandleValue(); }

    // Use
    inline bool Valid() const { return m_Handle != 0; }
    int GetHandleValue() const { return m_Handle; }
};

inline CDataModelHandle::CDataModelHandle(int inHandle)
    : m_Handle(inHandle)
{
}
inline CDataModelHandle::CDataModelHandle(const CDataModelHandle &other)
    : m_Handle(other.m_Handle)
{
}
inline CDataModelHandle::~CDataModelHandle()
{
}

inline CDataModelHandle &CDataModelHandle::operator=(const CDataModelHandle &other)
{
    if (this != &other) {
        m_Handle = other.m_Handle;
    }
    return *this;
}

inline bool CDataModelHandle::operator==(const CDataModelHandle &other) const
{
    return m_Handle == other.m_Handle;
}

inline bool CDataModelHandle::operator<(const CDataModelHandle &other) const
{
    return m_Handle < other.m_Handle;
}

class Qt3DSDMInstanceHandle : public CDataModelHandle
{
public:
    Qt3DSDMInstanceHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMInstanceHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMInstanceHandle(const Qt3DSDMInstanceHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMInstanceHandle &operator=(const Qt3DSDMInstanceHandle &inOther)
    {
        return static_cast<Qt3DSDMInstanceHandle &>(CDataModelHandle::operator=(inOther));
    }

    inline bool operator==(const Qt3DSDMInstanceHandle &other) const
    {
        return CDataModelHandle::operator==(other);
    }
};

typedef std::vector<Qt3DSDMInstanceHandle> TInstanceHandleList;

class Qt3DSDMPropertyHandle : public CDataModelHandle
{
public:
    Qt3DSDMPropertyHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMPropertyHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMPropertyHandle(const Qt3DSDMInstanceHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMPropertyHandle &operator=(const Qt3DSDMPropertyHandle &inOther)
    {
        return static_cast<Qt3DSDMPropertyHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<Qt3DSDMPropertyHandle> TPropertyHandleList;

class CUICDMSlideHandle : public CDataModelHandle
{
public:
    CUICDMSlideHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    CUICDMSlideHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMSlideHandle(const CUICDMSlideHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMSlideHandle &operator=(const CUICDMSlideHandle &inOther)
    {
        return static_cast<CUICDMSlideHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<CUICDMSlideHandle> TSlideHandleList;

class CUICDMSlideGraphHandle : public CDataModelHandle
{
public:
    CUICDMSlideGraphHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    CUICDMSlideGraphHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMSlideGraphHandle(const CUICDMSlideGraphHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMSlideGraphHandle &operator=(const CUICDMSlideGraphHandle &inOther)
    {
        return static_cast<CUICDMSlideGraphHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<CUICDMSlideGraphHandle> TSlideGraphHandleList;

class CUICDMAnimationHandle : public CDataModelHandle
{
public:
    CUICDMAnimationHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    CUICDMAnimationHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMAnimationHandle(const CUICDMAnimationHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMAnimationHandle &operator=(const CUICDMAnimationHandle &inOther)
    {
        return static_cast<CUICDMAnimationHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<CUICDMAnimationHandle> TAnimationHandleList;

class CUICDMKeyframeHandle : public CDataModelHandle
{
public:
    CUICDMKeyframeHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    CUICDMKeyframeHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMKeyframeHandle(const CUICDMKeyframeHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMKeyframeHandle &operator=(const CUICDMKeyframeHandle &inOther)
    {
        return static_cast<CUICDMKeyframeHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<CUICDMKeyframeHandle> TKeyframeHandleList;

class CUICDMActionHandle : public CDataModelHandle
{
public:
    CUICDMActionHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    CUICDMActionHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMActionHandle(const CUICDMActionHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMActionHandle &operator=(const CUICDMActionHandle &inOther)
    {
        return static_cast<CUICDMActionHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<CUICDMActionHandle> TActionHandleList;

class CUICDMHandlerArgHandle : public CDataModelHandle
{
public:
    CUICDMHandlerArgHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    CUICDMHandlerArgHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMHandlerArgHandle(const CUICDMHandlerArgHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }
    CUICDMHandlerArgHandle &operator=(const CUICDMHandlerArgHandle &inOther)
    {
        return static_cast<CUICDMHandlerArgHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<CUICDMHandlerArgHandle> THandlerArgHandleList;

class CUICDMEventHandle : public CDataModelHandle
{
public:
    CUICDMEventHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    CUICDMEventHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMEventHandle(const CUICDMEventHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMEventHandle &operator=(const CUICDMEventHandle &inOther)
    {
        return static_cast<CUICDMEventHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<CUICDMEventHandle> TEventHandleList;

class CUICDMHandlerHandle : public CDataModelHandle
{
public:
    CUICDMHandlerHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    CUICDMHandlerHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMHandlerHandle(const CUICDMHandlerHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMHandlerHandle &operator=(const CUICDMHandlerHandle &inOther)
    {
        return static_cast<CUICDMHandlerHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<CUICDMHandlerHandle> THandlerHandleList;

class CUICDMHandlerParamHandle : public CDataModelHandle
{
public:
    CUICDMHandlerParamHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    CUICDMHandlerParamHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMHandlerParamHandle(const CUICDMHandlerParamHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMHandlerParamHandle &operator=(const CUICDMHandlerParamHandle &inOther)
    {
        return static_cast<CUICDMHandlerParamHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<CUICDMHandlerParamHandle> THandlerParamHandleList;

class CUICDMMetaDataPropertyHandle : public CDataModelHandle
{
public:
    CUICDMMetaDataPropertyHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    CUICDMMetaDataPropertyHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMMetaDataPropertyHandle(const CUICDMMetaDataPropertyHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMMetaDataPropertyHandle &operator=(const CUICDMMetaDataPropertyHandle &inOther)
    {
        return static_cast<CUICDMMetaDataPropertyHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<CUICDMMetaDataPropertyHandle> TMetaDataPropertyHandleList;

class CUICDMCategoryHandle : public CDataModelHandle
{
public:
    CUICDMCategoryHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    CUICDMCategoryHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMCategoryHandle(const CUICDMCategoryHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }
    CUICDMCategoryHandle &operator=(const CUICDMCategoryHandle &inOther)
    {
        return static_cast<CUICDMCategoryHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<CUICDMCategoryHandle> TCategoryHandleList;

class CUICDMGuideHandle : public CDataModelHandle
{
public:
    CUICDMGuideHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    CUICDMGuideHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    CUICDMGuideHandle(const CUICDMCategoryHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }
    CUICDMGuideHandle &operator=(const CUICDMGuideHandle &inOther)
    {
        CDataModelHandle::operator=(inOther);
        return *this;
    }
};

typedef std::vector<CUICDMGuideHandle> TGuideHandleList;
}

Q_DECLARE_METATYPE(qt3dsdm::CUICDMSlideHandle);

#endif
