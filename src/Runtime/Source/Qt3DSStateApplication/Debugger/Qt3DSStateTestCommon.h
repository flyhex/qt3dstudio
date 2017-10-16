/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
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
#ifndef UIC_STATE_TEST_COMMON_H
#define UIC_STATE_TEST_COMMON_H
#include "Qt3DSState.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "foundation/Qt3DSMath.h"
#include "foundation/FileTools.h"
#include "Qt3DSStateTest.h"
#include "EASTL/string.h"
#include "foundation/TrackingAllocator.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

namespace qt3ds {
namespace state {
    namespace test {

        struct SLuaContext
        {
            NVFoundationBase &m_Foundation;
            lua_State *m_LuaState;

            SLuaContext(NVFoundationBase &fnd)
                : m_Foundation(fnd)
            {
                m_LuaState = lua_newstate(LuaAlloc, this);
                luaL_openlibs(m_LuaState);
            }

            static void *LuaAlloc(void *ud, void *ptr, size_t osize, size_t nsize)
            {
                SLuaContext *ctx = reinterpret_cast<SLuaContext *>(ud);
                if (nsize == 0) {
                    if (ptr)
                        ctx->m_Foundation.getAllocator().deallocate(ptr);
                    return NULL;
                } else {
                    if (nsize < osize && ptr)
                        return ptr;

                    void *newMem = ctx->m_Foundation.getAllocator().allocate(nsize, "lua memory",
                                                                             __FILE__, __LINE__);
                    if (osize && ptr) {
                        size_t copyAmt = qt3ds::NVMin(osize, nsize);
                        qt3ds::intrinsics::memCopy(newMem, ptr, (QT3DSU32)copyAmt);
                        ctx->m_Foundation.getAllocator().deallocate(ptr);
                    }
                    return newMem;
                }
            }

            ~SLuaContext() { Close(); }

            void Close()
            {
                if (m_LuaState)
                    lua_close(m_LuaState);
                m_LuaState = NULL;
            }

            operator lua_State *() { return m_LuaState; }
        };
    }
}
}
#endif
