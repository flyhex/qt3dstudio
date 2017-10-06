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

#include "Qt3DSStateFactory.h"

#include "cstdio"

#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"

#include "UICTypes.h"
#include "UICKernelTypes.h"
#include "Qt3DSStateLuaEngine.h"
#include "EventPollingSystem.h"
#include "EventSystem.h"
#include "UICStateVisualBindingContext.h"
#include "UICStateVisualBindingContextValues.h"
#include "UICStateScriptContext.h"

#include "Qt3DSStateInputStreamFactory.h"
#include "Qt3DSStateContext.h"

namespace {

class CNDDStateFactory;
struct SVisualStateHandler : public uic::state::IVisualStateInterpreterFactory,
                             public uic::state::IVisualStateCommandHandler
{
    qt3ds::NVAllocatorCallback &m_Allocator;
    CNDDStateFactory &m_Factory;
    uic::state::INDDStateFactory::IStateInterpreterCreateCallback *m_StateInterpreterCreateCallback;
    qt3ds::QT3DSI32 mRefCount;

public:
    SVisualStateHandler(qt3ds::NVAllocatorCallback &alloc, CNDDStateFactory &inFactory)
        : m_Allocator(alloc)
        , m_Factory(inFactory)
        , m_StateInterpreterCreateCallback(0)
        , mRefCount(0)
    {
    }
    void
    SetStateInterpreterCreateCallback(uic::state::INDDStateFactory::IStateInterpreterCreateCallback
                                          &inStateInterpreterCreateCallback)
    {
        m_StateInterpreterCreateCallback = &inStateInterpreterCreateCallback;
    }

    void addRef() override { qt3ds::foundation::atomicIncrement(&mRefCount); }
    void release() override
    {
        using namespace qt3ds;
        using namespace qt3ds::foundation;
        QT3DS_IMPLEMENT_REF_COUNT_RELEASE(m_Allocator);
    }

    uic::state::IStateInterpreter *OnNewStateMachine(const char8_t *inPath,
                                                             const char8_t *inId,
                                                             const char8_t *inDatamodelFunction) override;

    void Handle(const uic::state::SVisualStateCommand &inCommand,
                        uic::state::IScriptContext &inScriptContext) override;
};

class CNDDStateFactory : public uic::state::INDDStateFactory
{
public:
    uic::state::SNDDStateContext &m_Context;
    qt3ds::foundation::NVScopedRefCounted<uic::state::INDDStateLuaEngine> m_ScriptBridge;
    qt3ds::foundation::NVScopedRefCounted<uic::state::IVisualStateContext> m_VisualStateContext;
    qt3ds::foundation::NVScopedRefCounted<uic::evt::IEventSystem> m_EventSystem;
    uic::state::INDDStateApplication *m_Application;
    SVisualStateHandler *m_VisualStateHandler;
    uic::state::INDDStateFactory::IStateInterpreterCreateCallback *m_StateInterpreterCreateCallback;

    qt3ds::QT3DSI32 m_RefCount;

    CNDDStateFactory(uic::state::SNDDStateContext &inContext)
        : m_Context(inContext)
        , m_ScriptBridge(0)
        , m_VisualStateContext(0)
        , m_EventSystem(0)
        , m_Application(0)
        , m_VisualStateHandler(0)
        , m_StateInterpreterCreateCallback(0)
        , m_RefCount(0)
    {
    }

    ~CNDDStateFactory()
    {
        qCDebug (qt3ds::TRACE_INFO) << "CNDDStateFactory destructing";
        using namespace Q3DStudio;
        // Release the visual state context.
        m_VisualStateContext = 0;
        // Release the event system, it must be released before script engine
        m_EventSystem = NULL;
        m_ScriptBridge->Shutdown(*m_Context.m_Foundation);
    }

    void addRef() override { qt3ds::foundation::atomicIncrement(&m_RefCount); }

    void release() override
    {
        qt3ds::foundation::atomicDecrement(&m_RefCount);
        if (m_RefCount <= 0) {
            NVDelete(m_Context.GetAllocator(), this);
        }
    }

    uic::state::INDDStateScriptBridge &GetScriptEngine() override
    {
        if (m_ScriptBridge == NULL) {
            m_ScriptBridge = uic::state::INDDStateLuaEngine::Create(m_Context);
            m_ScriptBridge->PreInitialize();
        }
        return *m_ScriptBridge;
    }

    uic::state::IVisualStateContext &GetVisualStateContext() override
    {
        if (!m_VisualStateContext) {
            m_VisualStateContext = uic::state::IVisualStateContext::Create(
                *m_Context.m_Foundation, m_Context.GetStringTable());
            SVisualStateHandler *newHandle =
                QT3DS_NEW(m_Context.m_Foundation->getAllocator(),
                       SVisualStateHandler)(m_Context.m_Foundation->getAllocator(), *this);
            if (m_StateInterpreterCreateCallback)
                newHandle->SetStateInterpreterCreateCallback(*m_StateInterpreterCreateCallback);
            m_VisualStateContext->SetCommandHandler(newHandle);
            m_VisualStateContext->SetInterpreterFactory(newHandle);
        }
        return *m_VisualStateContext;
    }
    uic::evt::IEventSystem &GetEventSystem() override
    {
        if (!m_EventSystem) {
            m_EventSystem = uic::evt::IEventSystem::Create(*m_Context.m_Foundation);
        }
        return *m_EventSystem;
    }
    qt3ds::foundation::IStringTable &GetStringTable() override { return m_Context.GetStringTable(); }
    qt3ds::NVFoundationBase &GetFoundation() override { return *m_Context.m_Foundation.mPtr; }
    Q3DStudio::ITimeProvider &GetTimeProvider() override { return m_Context.m_TimeProvider; }
    uic::state::IInputStreamFactory &GetInputStreamFactory() override
    {
        return *m_Context.m_InputStreamFactory;
    }

    uic::state::INDDStateApplication *GetApplication() override { return m_Application; }
    void SetApplication(uic::state::INDDStateApplication *inApplication) override
    {
        m_Application = inApplication;
        if (inApplication) {
            GetScriptEngine();
            m_ScriptBridge->SetApplication(*inApplication);
            // Most of the script bridge lua functions will hard crash if
            // the initialization happens before the application is set.
            // This keeps the errors in lua-land instead of in crashing ui composer.
            m_ScriptBridge->Initialize();
        }
    }
    void SetStateInterpreterCreateCallback(
        IStateInterpreterCreateCallback &inStateInterpreterCreateCallback) override
    {
        m_StateInterpreterCreateCallback = &inStateInterpreterCreateCallback;
        if (m_VisualStateHandler)
            m_VisualStateHandler->SetStateInterpreterCreateCallback(
                inStateInterpreterCreateCallback);
    }
};

uic::state::IStateInterpreter *
SVisualStateHandler::OnNewStateMachine(const char8_t *inPath, const char8_t *inId,
                                       const char8_t *inDatamodelFunction)
{
    uic::state::IStateInterpreter *theInterpreter =
        m_Factory.m_ScriptBridge->CreateStateMachine(inPath, inId, inDatamodelFunction);
    if (m_StateInterpreterCreateCallback)
        m_StateInterpreterCreateCallback->OnCreate(
            m_Factory.m_Context.GetStringTable().RegisterStr(inId), *theInterpreter);
    return theInterpreter;
}

void SVisualStateHandler::Handle(const uic::state::SVisualStateCommand &inCommand,
                                 uic::state::IScriptContext &inScriptContext)
{
    (void)inCommand;
    (void)inScriptContext;
}
}

namespace uic {
namespace state {

    INDDStateFactory &INDDStateFactory::Create(SNDDStateContext &inContext)
    {
        return *QT3DS_NEW(inContext.m_Foundation->getAllocator(), CNDDStateFactory)(inContext);
    }
}
}
