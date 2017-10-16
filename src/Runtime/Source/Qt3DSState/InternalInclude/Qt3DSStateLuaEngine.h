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

//==============================================================================
//	Lua Includes
//==============================================================================
#include "Qt3DSLuaIncludes.h"

//==============================================================================
//	Studio Includes
//==============================================================================
#include "Qt3DSIScriptBridge.h"
#include "EASTL/map.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSPool.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSSync.h"
#include "foundation/Qt3DSMutex.h"
#include "Qt3DSIComponentManager.h"
#include "Qt3DSStateContext.h"

#include "Qt3DSStateScriptBridge.h"

namespace Q3DStudio {
class ITimeProvider;
}

namespace qt3ds {
namespace state {

    struct SNDDStateContext;
    class IStateInterpreter;

    //==============================================================================
    //	Typedefs
    //==============================================================================
    typedef void (*TLuaLibraryLoader)(lua_State *);

    //==============================================================================
    //	Global Lua Library Loader
    //==============================================================================
    extern TLuaLibraryLoader LoadLuaLibraries;

    class INDDStateLuaEngine : public INDDStateScriptBridge
    {
    public:
        virtual ~INDDStateLuaEngine() {}

    public: // Public functions but not functions on the script bridge
        virtual qt3ds::state::IStateInterpreter *
        CreateStateMachine(const char8_t *inPath, const char8_t *inId,
                           const char8_t *inDatamodelFunction) = 0;
        virtual void PreInitialize() = 0;
        virtual void Initialize() = 0;
        virtual void Shutdown(qt3ds::NVFoundationBase &inFoundation) = 0;

    public:
        static INDDStateLuaEngine *Create(qt3ds::state::SNDDStateContext &inContext);
    };
}
}
