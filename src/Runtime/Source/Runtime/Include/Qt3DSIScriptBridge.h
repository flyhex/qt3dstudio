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
#include "Qt3DSIComponentManager.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "Qt3DSLuaIncludes.h"
#include "foundation/Qt3DSRefCounted.h"
//==============================================================================
//	Namespace
//==============================================================================
namespace qt3ds {
namespace runtime {
    class IApplication;
    class IApplicationCore;
}
}
namespace qt3ds {
namespace state {
    namespace debugger {
        class IMultiProtocolSocket;
    }
}
}
namespace qt3ds {
namespace render {
    class IThreadPool;
}
}

struct lua_State;

namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
struct SEventCommand;
class IPresentation;

//==============================================================================
/**
 *	@interface	IScriptBridge
 *	@brief		Callback and load interface for a script engine.
 */

class ILuaScriptTableProvider
{
protected:
    virtual ~ILuaScriptTableProvider() {}
public:
    virtual void CreateTable(lua_State *inState) = 0;
};

struct SScriptEngineGotoSlideArgs
{
    TTimePolicyModeOption m_Mode;
    //-2 means previous, -1 means next, 1-N means slide
    const char *m_PlaythroughTo;
    TULongOption m_StartTime;
    FLOAT m_Rate;
    bool m_Reverse;
    TBoolOption m_Paused;
    SScriptEngineGotoSlideArgs()
        : m_PlaythroughTo(NULL)
        , m_Rate(1.0f)
        , m_Reverse(false)
    {
    }
};

class CScriptEngineCallFunctionArgRetriever
{
public:
    CScriptEngineCallFunctionArgRetriever(const char *inArguments)
        : m_ArgumentString(inArguments)
    {
    }
    virtual ~CScriptEngineCallFunctionArgRetriever() {}
    // Retrieve argument
    // Return value: -1 error; otherwise it indicates argument count
    virtual int RetrieveArgument(lua_State *inState);
    virtual eastl::string GetArgDescription();

protected:
    const char *m_ArgumentString;
};

class IScriptBridge : public qt3ds::foundation::NVRefCounted
{
    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    virtual ~IScriptBridge() {}

public: // thread
    // After this call all public functions are protected by a mutex
    virtual void EnableMultithreadedAccess() = 0;
    // After this call all public functions are not threadsafe.
    virtual void DisableMultithreadedAccess() = 0;

public: // Settings
    virtual void SetApplicationCore(qt3ds::runtime::IApplicationCore &inApplication) = 0;
    virtual void SetApplication(qt3ds::runtime::IApplication &inApplication) = 0;

public: // Scripts
    // Both loads lua script, create an self table -> scriptIndex in a behaviors table
    // LoadScript goes further by registering scriptIndex->inPresentation, and inOwner->m_ScriptID=
    // scriptIndex
    virtual void LoadScript(IPresentation *inPresentation, TElement *inOwner,
                            const CHAR *inName) = 0;
    virtual Q3DStudio::INT32 InitializeApplicationBehavior(const char *inProjectRelativePath) = 0;

public: // Script functions and Callbacks
    virtual void ProcessFrameCallbacks(IPresentation *inPresentation) = 0;
    // Call a member function inFnName from self table whose script index is given by inApp
    virtual void ExecuteApplicationScriptFunction(Q3DStudio::INT32 inApp, const char *inFnName) = 0;
    virtual void CallFunction(const char *behavior, const char *handler,
                              CScriptEngineCallFunctionArgRetriever &inArgRetriever) = 0;

public: // Custom Actions
    virtual void ProcessSignal(IPresentation *inPresentation,
                               const SEventCommand &inCommand) = 0;
    virtual void ProcessCustomActions(IPresentation *inPresentation,
                                      const SEventCommand &inCommand) = 0;
    virtual void ProcessCustomCallback(IPresentation *inPresentation,
                                       const SEventCommand &inCommand) = 0;

public: // Elements
    // Use inProvider to create a new table and associate with inElement: currently a render plugin
    // element
    // this mimics render plugin as an behaviour element
    virtual void SetTableForElement(TElement &inElement, ILuaScriptTableProvider &inProvider) = 0;
    virtual void SetAttribute(const char *element, const char *attName, const char *value) = 0;
    virtual void FireEvent(const char *element, const char *evtName) = 0;
    virtual void SetDataInputValue(const QString &name, const QVariant &value) = 0;

public: // Components
    virtual void GotoSlide(const char *component, const char *slideName,
                           const SScriptEngineGotoSlideArgs &inArgs) = 0;
    virtual void GotoSlideRelative(const char *component, bool inNextSlide, bool inWrap,
                                   const SScriptEngineGotoSlideArgs &inArgs) = 0;

public: // Presentation
    virtual void SetPresentationAttribute(const char *presId, const char *attName,
                                          const char *attValue) = 0;

public: // Multimedia
    virtual bool PlaySoundFile(const char *soundPath) = 0;

public: // Miscellaneous
    virtual void EnableDebugging(qt3ds::state::debugger::IMultiProtocolSocket &socket) = 0;
    virtual void EnableProfiling() = 0;
    virtual void StepGC() = 0;
};

} // namespace Q3DStudio
