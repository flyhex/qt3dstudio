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
#pragma once
#ifndef QT3DS_APPLICATION_H
#define QT3DS_APPLICATION_H
#include "foundation/Qt3DSRefCounted.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Utils.h"
#include "Qt3DSKernelTypes.h"
#include "Qt3DSMetadata.h"

namespace Q3DStudio {
class IRuntimeFactory;
class IRuntimeFactoryCore;
class CRuntime;
class CPresentation;
class CInputEngine;
class IAudioPlayer;
}

namespace qt3ds {
namespace state {
namespace debugger {
class IDebugger;
class ISceneGraphRuntimeDebugger;
}
}
}

namespace qt3ds {
class Qt3DSAssetVisitor;
namespace runtime {
using namespace qt3ds::foundation;
using namespace qt3ds;

class IElementAllocator;
class IActivityZoneManager;

class CAppStr : public eastl::basic_string<char8_t, ForwardingAllocator>
{
    typedef eastl::basic_string<char8_t, ForwardingAllocator> TBase;

public:
    CAppStr(NVAllocatorCallback &alloc, const char8_t *inStr = NULL);
    CAppStr(const CAppStr &inOther);
    CAppStr();
    CAppStr &operator=(const CAppStr &inOther);
};

class IApplication;

class IAppRunnable : public NVRefCounted
{
public:
    virtual void Run() = 0;
};

class QT3DS_AUTOTEST_EXPORT IApplicationCore : public NVRefCounted
{
public:
    // threadsafe call.
    virtual void QueueForMainThread(IAppRunnable &inRunnable) = 0;

    // The directory that contains the executable and the root resource path
    virtual CRegisteredString GetApplicationDirectory() const = 0;
    // Directory that contained the UIA file.
    virtual CRegisteredString GetProjectDirectory() const = 0;
    // Directory where we will copy shared object files to before we load them.
    // This is specifically for android and the case where the place where the project exists
    // is not always the place where we can load the project.
    virtual CRegisteredString GetDllDir() const = 0;
    virtual void SetDllDir(const char *inDllDir) = 0;
    virtual Q3DStudio::THashValue HashString(const char *inStr) = 0;
    virtual const char *ReverseHash(Q3DStudio::THashValue theValue) = 0;

    virtual Q3DStudio::IRuntimeMetaData &GetMetaData() = 0;
    // Element handles are unique across all presentations.
    virtual Q3DStudio::TElement *GetElementByHandle(Q3DStudio::UINT32 inHandle) = 0;
    // Passing in NULL gets you zero as the return handle.
    virtual Q3DStudio::UINT32 GetHandleForElement(Q3DStudio::TElement *inElement) = 0;

    virtual Q3DStudio::IRuntimeFactoryCore &GetRuntimeFactoryCore() = 0;
    virtual void HideFPS(bool flag) = 0;

    virtual qt3ds::state::debugger::ISceneGraphRuntimeDebugger &GetSceneGraphDebugger() = 0;
    virtual qt3ds::state::debugger::IDebugger &GetStateDebugger() = 0;
    virtual IActivityZoneManager &GetActivityZoneManager() = 0;
    virtual IElementAllocator &GetElementAllocator() = 0;

    // Debugging is disabled by default.
    // This is expected be be called before load and will not immediately connect.
    // until we have a valid presentation directory (which is required for the debug system
    // at the other end of the connection).
    // listen - either list for a connection else actively connect to a server.
    // inServer - if listen is false, then the server addr to connect to.  Ignored if server is
    // true
    // inPort - If listen is false, the port on server to connect to, else port to open up
    // socket on.
    virtual void EnableDebugging(bool inListen = false, const char8_t *inServer = NULL,
                                 int inPort = 0) = 0;

    // State machine is enabled by default
    // Disable state machine is meant to disable internal statemachine, but may plug in an
    // external statemachine
    virtual void DisableStateMachine() = 0;

    // nonblocking call to begin loading, loads uia file alone and returns.
    virtual bool BeginLoad(const char8_t *inFilePath) = 0;
    // blocking call to end all loading threads and such/wait till finished
    virtual void EndLoad() = 0;
    // Will EndLoad cause nontrivial blocking.
    // Runs any queued runnables.
    virtual bool HasCompletedLoading() = 0;

    virtual void setAssetVisitor(qt3ds::Qt3DSAssetVisitor *) = 0;

    // will force loading to end if endLoad hasn't been called yet.  Will fire off loading
    // of resources that need to be uploaded to opengl.  Maintains reference to runtime factory
    virtual IApplication &CreateApplication(Q3DStudio::CInputEngine &inInputEngine,
                                            Q3DStudio::IAudioPlayer *inAudioPlayer,
                                            Q3DStudio::IRuntimeFactory &inFactory) = 0;

    // maintains reference to runtime factory core.  AppDir is where the executable is located;
    // the system will expect res directory
    // next to executable.
    static IApplicationCore &CreateApplicationCore(Q3DStudio::IRuntimeFactoryCore &inFactory,
                                                   const char8_t *inApplicationDirectory);
    static bool isPickingEvent(Q3DStudio::TEventCommandHash event);
};

struct DataInputControlledAttribute
{
    QByteArray elementPath;
    QVector<QByteArray> attributeName;
    Q3DStudio::EAttributeType propertyType = Q3DStudio::ATTRIBUTETYPE_NONE;
};

enum DataInputType {
    DataInputTypeInvalid = 0,
    DataInputTypeRangedNumber,
    DataInputTypeString,
    DataInputTypeFloat,
    // TODO Evaluator
    // DataTypeEvaluator,
    DataInputTypeBoolean,
    DataInputTypeVector3,
    DataInputTypeVariant
};

struct DataInputDef
{
    QVector<DataInputControlledAttribute> controlledAttributes;
    DataInputType type = DataInputTypeInvalid;
    float min = 0.0f;
    float max = 0.0f;
};

typedef QMap<QString, DataInputDef> DataInputMap;

class IApplication : public IApplicationCore
{
public:
    virtual Q3DStudio::IRuntimeFactory &GetRuntimeFactory() const = 0;

    virtual void SetFrameCount(Q3DStudio::INT32 inFrameCount) = 0;
    virtual Q3DStudio::INT32 GetFrameCount() = 0;

    // Allow overriding the global wall time. This way animations can be
    // advanced at an arbitrary rate when rendering offscreen. Set to 0 to
    // disable (the default).
    virtual void SetTimeMilliSecs(Q3DStudio::INT64 inMilliSecs) = 0;

    virtual Q3DStudio::INT64 GetTimeMilliSecs() = 0;
    virtual void ResetTime() = 0;

    // Setup during UpdateAndRender.  Returns 0 for first frame.
    virtual double GetMillisecondsSinceLastFrame() = 0;

    virtual Q3DStudio::CInputEngine &GetInputEngine() = 0;

    virtual Q3DStudio::CPresentation *GetPrimaryPresentation() = 0;

    virtual Q3DStudio::CPresentation *GetPresentationById(const char8_t *inId) = 0;

    virtual QList<Q3DStudio::CPresentation *> GetPresentationList() = 0;

    // Update all the presentations and render them.  Called exactly once per frame.
    virtual void UpdateAndRender() = 0;

    virtual bool IsApplicationDirty() = 0;

    virtual void MarkApplicationDirty() = 0;

    virtual Q3DStudio::IAudioPlayer &GetAudioPlayer() = 0;

    virtual bool createSuccessful() = 0;

    virtual DataInputMap &dataInputMap() = 0;
};
}
}

#endif
