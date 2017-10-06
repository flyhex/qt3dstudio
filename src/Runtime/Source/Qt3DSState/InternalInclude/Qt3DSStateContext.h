/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#include "foundation/TrackingAllocator.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSAtomic.h"
#include "Qt3DSStateInputStreamFactory.h"
#include "Qt3DSStateFactory.h"

namespace Q3DStudio {
class ITimeProvider;
}

namespace uic {
namespace state {

    class INDDStateFactory;

    struct SNDDStateContext : public qt3ds::foundation::NVRefCounted
    {
        qt3ds::foundation::NVScopedRefCounted<qt3ds::NVFoundation> m_Foundation;
        qt3ds::NVAllocatorCallback &m_Allocator;
        qt3ds::foundation::NVScopedRefCounted<qt3ds::foundation::IStringTable> m_StringTable;
        qt3ds::foundation::NVScopedRefCounted<uic::state::IInputStreamFactory> m_InputStreamFactory;
        Q3DStudio::ITimeProvider &m_TimeProvider;
        qt3ds::foundation::NVScopedRefCounted<INDDStateFactory> m_Factory;
        eastl::string m_ProjectDir;
        volatile qt3ds::QT3DSI32 mRefCount;

        SNDDStateContext(
            qt3ds::foundation::NVScopedRefCounted<qt3ds::NVFoundation> inFoundation,
            qt3ds::foundation::NVScopedRefCounted<qt3ds::foundation::IStringTable> inStringTable,
            qt3ds::foundation::NVScopedRefCounted<uic::state::IInputStreamFactory>
                inInputStreamFactory,
            Q3DStudio::ITimeProvider &inTimeProvider, const char *inProjectDir)
            : m_Foundation(inFoundation)
            , m_Allocator(m_Foundation->getAllocatorCallback())
            , m_StringTable(inStringTable)
            , m_InputStreamFactory(inInputStreamFactory)
            , m_TimeProvider(inTimeProvider)
            , m_Factory(0)
            , m_ProjectDir(inProjectDir)
            , mRefCount(0)
        {
            m_Factory = INDDStateFactory::Create(*this);
        }
        virtual ~SNDDStateContext()
        {
            qCDebug (qt3ds::TRACE_INFO) << "SNDDStateContext destructing";
        }

        void addRef() override { qt3ds::foundation::atomicIncrement(&mRefCount); }
        void release() override
        {
            using namespace qt3ds;
            using namespace qt3ds::foundation;
            QT3DS_IMPLEMENT_REF_COUNT_RELEASE(m_Allocator);
        }

        qt3ds::NVAllocatorCallback &GetAllocator() { return m_Allocator; }
        qt3ds::foundation::IStringTable &GetStringTable() { return *m_StringTable; }
        const char *GetProjectDir() { return m_ProjectDir.c_str(); }
    };
}
}
