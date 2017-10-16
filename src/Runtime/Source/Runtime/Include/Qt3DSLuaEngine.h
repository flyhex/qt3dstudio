/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#include "Qt3DSIComponentManager.h"
#include "foundation/Qt3DSSync.h"
#include "foundation/Qt3DSMutex.h"

namespace qt3ds {
namespace state {
    class IStateInterpreter;
}
}

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class CPresentation;
class ITimeProvider;

//==============================================================================
//	Typedefs
//==============================================================================
typedef void (*TLuaLibraryLoader)(lua_State *);

//==============================================================================
//	Global Lua Library Loader
//==============================================================================
extern TLuaLibraryLoader LoadLuaLibraries;

extern const INT32 KEY_PRESENTATION;
extern const INT32 KEY_BEHAVIORS;
extern const INT32 KEY_EVENTDATA_LIST;
extern const INT32 KEY_CHANGEDATA_LIST;
extern const INT32 KEY_CALLBACKS;
extern const INT32 KEY_REGISTEREDCALLBACKS;

extern const THashValue HASH_ONINITIALIZE;
extern const THashValue HASH_ONACTIVATE;
extern const THashValue HASH_ONDEACTIVATE;
extern const THashValue HASH_ONUPDATE;

extern const UINT32 LENGTH_KEY_ELEMENT;
extern const UINT32 LENGTH_KEY_SELF;
extern const UINT32 LENGTH_KEY_SCRIPT_INDEX;
extern const UINT32 LENGTH_KEY_ONINITIALIZE;
extern const UINT32 LENGTH_KEY_ONACTIVATE;
extern const UINT32 LENGTH_KEY_ONDEACTIVATE;
extern const UINT32 LENGTH_KEY_ONUPDDATE;

class CLuaEngine : public IScriptBridge
{
    //==============================================================================
    //	Structs
    //==============================================================================
public:
    struct SCallbackData
    {
        INT32 m_FunctionReference; ///< The Lua function to call when it occurs
        TElement *m_TargetElement; ///< The source element where the event is going to occur

        /* The behavior element that the lua code is executing from.  May be null in the case of
         * application behaviors */
        TElement *m_SelfElement;
        THashValue m_EventAttributeHash; ///< The event hash or the attribute hash
        lua_State *m_LuaState; ///< The associated lua state
        INT16 m_Global; ///< Is the function a global function or it resides in self
        INT16 m_ScriptIndex; ///< The associated script index if the function is a local function
    };

    typedef CArray<SCallbackData *> TCallbackDataList;

    virtual ~CLuaEngine() {}

public: // Public functions but not functions on the script bridge
    virtual qt3ds::state::IStateInterpreter *
    CreateStateMachine(const char8_t *inPath, const char8_t *inId,
                       const char8_t *inDatamodelFunction) = 0;
    virtual void PreInitialize() = 0;
    virtual void Initialize() = 0;
    virtual void Shutdown(qt3ds::NVFoundationBase &inFoundation) = 0;
    // use lua_register to register a global function, this is useful to call from outside
    virtual void AddGlobalFunction(const CHAR *inFunctionName, lua_CFunction inFunction) = 0;

public: // Static Helpers
    static void FreeCallbackData(SCallbackData *inData);
    static CPresentation *GetCurrentPresentation(lua_State *inLuaState);
    static void SetAdditionalSharedLibraryPath(const eastl::string &inAdditionalSharedLibraryPath);
    static CLuaEngine *Create(qt3ds::NVFoundationBase &inFoundation, ITimeProvider &inTimeProvider);
};

} // namespace Q3DStudio
