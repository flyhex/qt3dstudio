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

#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "Qt3DSLuaIncludes.h"
#include "foundation/Qt3DSRefCounted.h"
#include "Qt3DSTypes.h"

struct lua_State;
//==============================================================================
//	Namespace
//==============================================================================
namespace qt3ds {
namespace state {

    class INDDStateApplication;

    namespace debugger {
        class IMultiProtocolSocket;
    }

    //==============================================================================
    /**
     *	@interface	IScriptBridge
     *	@brief		Callback and load interface for a script engine.
     */

    // class CScriptEngineCallFunctionArgRetriever
    //{
    // public:
    //	CScriptEngineCallFunctionArgRetriever( const char *inArguments ): m_ArgumentString(
    //inArguments ) {}
    //	virtual					~CScriptEngineCallFunctionArgRetriever() {}
    //	// Retrieve argument
    //	// Return value: -1 error; otherwise it indicates argument count
    //	virtual int				RetrieveArgument( lua_State *inState );
    //	virtual eastl::string	GetArgDescription();
    // protected:
    //	const char	*m_ArgumentString;
    //};

    class INDDStateScriptBridge : public qt3ds::foundation::NVRefCounted
    {
        //==============================================================================
        //	Methods
        //==============================================================================
    public: // Construction
        virtual ~INDDStateScriptBridge() {}

    public: // thread
        // After this call all public functions are protected by a mutex
        virtual void EnableMultithreadedAccess() = 0;
        // After this call all public functions are not threadsafe.
        virtual void DisableMultithreadedAccess() = 0;

    public: // Settings
        virtual void SetApplication(qt3ds::state::INDDStateApplication &inApplication) = 0;

    public: // Binary preloading
        // Starts preloading scripts offline.  This sets m_LuaState to NULL until after
        // EndPreloadScripts
        // This function is only used for binary loading, eg, .uiab
        // virtual void		BeginPreloadScripts( const eastl::vector<const char*>& inScripts,
        // uic::render::IThreadPool& inThreadPool, const char* inProjectDir ) = 0;
        // This function is blocking if m_LuaState == NULL, which only holds during binary loading
        // virtual void		EndPreloadScripts() = 0;
        // Fast loading support
        // get the set of loaded scripts relative file names
        // This has the side effect of writing out the loaded scripts to
        // projectDir/binary/compiledlua.bin, only used for binary save
        virtual eastl::vector<eastl::string> GetLoadedScripts() = 0;

    public: // Scripts
        virtual Q3DStudio::INT32
        InitializeApplicationBehavior(const char *inProjectRelativePath) = 0;

    public: // Script functions and Callbacks
        // Call a member function inFnName from self table whose script index is given by inApp
        virtual void ExecuteApplicationScriptFunction(Q3DStudio::INT32 inApp,
                                                      const char *inFnName) = 0;
        // virtual void		CallFunction( const char* behavior, const char* handler,
        // CScriptEngineCallFunctionArgRetriever &inArgRetriever ) = 0;

    public: // Miscellaneous
        // use lua_register to register a global function, this is useful to call from outside
        virtual void AddGlobalFunction(const Q3DStudio::CHAR *inFunctionName,
                                       lua_CFunction inFunction) = 0;
        // virtual void
        // EnableDebugging(uic::state::debugger::IMultiProtocolSocket& socket ) = 0;
        // virtual void				EnableProfiling() = 0;
        virtual void StepGC() = 0;
    };
}
}
