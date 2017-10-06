/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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
#ifndef UIC_RENDER_DYNAMIC_OBJECT_SYSTEM_UTIL_H
#define UIC_RENDER_DYNAMIC_OBJECT_SYSTEM_UTIL_H
#include "UICRender.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "UICRenderString.h"

namespace uic {
namespace render {
    namespace dynamic {

        struct SStringLoadRemapper
        {
            CStrTableOrDataRef m_StrData;
            IStringTable &m_StringTable;
            CRenderString m_PathMapper;
            const char8_t *m_ProjectDir;
            SStringLoadRemapper(NVAllocatorCallback &alloc, CStrTableOrDataRef inData,
                                const char8_t *inProjectDir, IStringTable &inStrTable)
                : m_StrData(inData)
                , m_StringTable(inStrTable)
                , m_ProjectDir(inProjectDir)
            {
            }
            void Remap(CRegisteredString &inStr) { inStr.Remap(m_StrData); }
        };

        struct SStringSaveRemapper
        {
            const qt3ds::render::SStrRemapMap &m_Map;
            CRenderString m_RelativeBuffer;
            CRenderString m_ProjectDir;
            CRenderString m_FinalBuffer;
            IStringTable &m_StringTable;
            SStringSaveRemapper(NVAllocatorCallback &alloc, const qt3ds::render::SStrRemapMap &map,
                                const char8_t *inProjectDir, IStringTable &inStrTable)
                : m_Map(map)
                , m_StringTable(inStrTable)
            {
                m_ProjectDir.assign(inProjectDir);
            }
            void Remap(CRegisteredString &inStr) { inStr.Remap(m_Map); }
        };

        inline QT3DSU32 Align(QT3DSU32 inValue)
        {
            if (inValue % 4)
                return inValue + (4 - (inValue % 4));
            return inValue;
        }

        inline QT3DSU32 Align8(QT3DSU32 inValue)
        {
            if (inValue % 8)
                return inValue + (8 - (inValue % 8));
            return inValue;
        }

        inline qt3ds::QT3DSU32 getSizeofShaderDataType(NVRenderShaderDataTypes::Enum value)
        {
            using namespace qt3ds;
            using namespace qt3ds::render;
            switch (value) {
#define HANDLE_QT3DS_SHADER_DATA_TYPE(x)                                                              \
    case NVRenderShaderDataTypes::x:                                                               \
        return sizeof(x);
                ITERATE_QT3DS_SHADER_DATA_TYPES
#undef HANDLE_QT3DS_SHADER_DATA_TYPE
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        inline const char *GetShaderDatatypeName(NVRenderShaderDataTypes::Enum inValue)
        {
            switch (inValue) {
#define HANDLE_QT3DS_SHADER_DATA_TYPE(type)                                                           \
    case NVRenderShaderDataTypes::type:                                                            \
        return #type;
                ITERATE_QT3DS_SHADER_DATA_TYPES
#undef HANDLE_QT3DS_SHADER_DATA_TYPE
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return "";
        }
    }
}
}

#endif
