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
#include "EnginePrefix.h"
#include "UICIStateful.h"
#include "foundation/Qt3DSVec4.h"
#include "foundation/Qt3DSOption.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSRefCounted.h"
#include "UICWindowSystem.h"
#include "UICTimer.h"
#include "UICPresentation.h"
#include "UICRenderRuntimeBinding.h"
#include <QtCore/qobject.h>
#include <QtGui/qsurfaceformat.h>
#include <KD/kd.h>
//==============================================================================
//	Namespace
//==============================================================================

struct lua_State;

typedef void (*qml_Function)(void *inUserData);

class QINDDViewSignalProxy : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void SigSlideEntered(const QString &elementPath, unsigned int index, const QString &name);
    void SigSlideExited(const QString &elementPath, unsigned int index, const QString &name);
};

namespace qt3ds {
class UICAssetVisitor;
}

namespace qt3ds {
namespace render {
class NVRenderContext;
}
}

namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class CTegraInputEngine;
class CTegraRenderEngine;
class IScene;
class CInputEngine;
class IAudioPlayer;

class ITegraRenderStateManager : public IStatefulStackBase
{
protected:
    virtual ~ITegraRenderStateManager() {}
public:
    virtual void SetViewport(INT32 inX, INT32 inY, INT32 inWidth, INT32 inHeight) = 0;
    virtual void SetScissorTestEnabled(bool inValue) = 0;
    virtual void SaveAllState() = 0;
    virtual void RestoreAllState() = 0;
};

struct TegraRenderScaleModes
{
    enum Enum {
        ExactSize = 0, // Ensure the viewport is exactly same size as application
        ScaleToFit = 1, // Resize viewport keeping aspect ratio
        ScaleToFill = 2, // Resize viewport to entire window
    };
};

struct TegraRenderShadeModes
{
    enum Enum {
        Shaded = 0, // Geometry is shaded only
        ShadedWireframe = 1, // Wireframe is drawn on top shaded geometry
        Wireframe = 2, // Wireframe only
    };
};

class ITegraApplicationRenderEngine
{
protected:
    virtual ~ITegraApplicationRenderEngine() {}

public:
    virtual void SetViewport(INT32 inX, INT32 inY, INT32 inWidth, INT32 inHeight) = 0;
    virtual void SetApplicationViewport(const qt3ds::render::NVRenderRect &inViewport) = 0;
    virtual void ensureRenderTarget() = 0;
    virtual void CheckResize(bool inForce, IPresentation &inActivePresentation) = 0;
    virtual BOOL LoadShaderCache(const CHAR *inFilePath) = 0;
    virtual void AbandonLoadingImages(IScene &inScene) = 0;
    virtual BOOL IsPickValid(KDfloat32 &outX, KDfloat32 &outY,
                             const IPresentation &inPresentation) const = 0;
    virtual void SetScaleMode(TegraRenderScaleModes::Enum inScale) = 0;
    virtual void SetShadeMode(TegraRenderShadeModes::Enum inShade) = 0;
    virtual TegraRenderScaleModes::Enum GetScaleMode() const = 0;

    // TODO: To be removed, not used anywhere anymore
    void CycleScaleMode()
    {
        int mode = (int)GetScaleMode();
        mode = (mode + 1) % 3;
        SetScaleMode((TegraRenderScaleModes::Enum)mode);
    }

    virtual ITegraRenderStateManager &GetTegraRenderStateManager() = 0;
    virtual qt3ds::render::NVRenderContext &GetRenderContext() = 0;
    virtual void SetMatteColor(qt3ds::foundation::Option<qt3ds::QT3DSVec4> inColor) = 0;
    virtual void EnableRenderRotation(bool inEnable) = 0;
    virtual void SetWriteOutShaderCache(bool inWriteOutShaderCache) = 0;
    virtual void Release() = 0;
    virtual void RenderText2D(FLOAT x, FLOAT y, qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor,
                              const char *text) = 0;
    virtual void RenderGpuProfilerStats(FLOAT x, FLOAT y,
                                        qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor) = 0;

    // Set the watermark texture as a compressed PNG file.
    virtual bool SetWatermarkTextureDataDDS(const unsigned char *inTextureData,
                                            size_t inDataSize) = 0;
    virtual void SetWatermarkLocation(float x, float y) = 0;
};

class INDDView : public qt3ds::foundation::NVRefCounted
{
public:
    virtual ~INDDView(){}

public: // loading
    virtual bool BeginLoad(const QString &sourcePath) = 0;
    virtual bool HasOfflineLoadingCompleted() = 0;
    virtual void InitializeGraphics(const QSurfaceFormat &format) = 0;

    virtual void Cleanup() = 0;

    virtual bool CanRender() = 0;

    virtual void Render() = 0;

    virtual bool WasLastFrameDirty() = 0;

    virtual KDint HandleMessage(const KDEvent *inEvent) = 0;

    virtual void Pause() = 0;
    virtual void UnPause() = 0;
    virtual bool IsPaused() = 0;
    virtual INT32 GetFrameCount() = 0;
    virtual void showOnScreenStats(bool) = 0;

public: // Input engine access
    virtual CInputEngine *GetInputEngine() = 0;
    // Only valid after InitializeGraphics
    virtual ITegraApplicationRenderEngine *GetTegraRenderEngine() = 0;

public:
    virtual void GoToSlideByName(const char *elementPath, const char *slideName) = 0;
    virtual void GoToSlideByIndex(const char *elementPath, const int slideIndex) = 0;
    virtual void GoToSlideRelative(const char *elementPath, const bool next, const bool wrap) = 0;
    virtual bool GetSlideInfo(const char *elementPath, int &currentIndex, int &previousIndex,
                              QString &currentName, QString &previousName) = 0;
    virtual void SetPresentationAttribute(const char *presId, const char *, const char *value) = 0;
    virtual void GoToTime(const char *elementPath, const float time) = 0;
    virtual void SetGlobalAnimationTime(qint64 inMilliSecs) = 0;
    virtual void SetAttribute(const char *elementPath, const char *attributeName,
                              const char *value) = 0;
    virtual bool GetAttribute(const char *elementPath, const char *attributeName, void *value) = 0;
    virtual void FireEvent(const char *element, const char *evtName) = 0;
    virtual bool PeekCustomAction(char *&outElementPath, char *&outActionName) = 0;
    virtual bool RegisterScriptCallback(int callbackType, qml_Function func, void *inUserData) = 0;
    virtual void FireEvent(const TEventCommandHash inEventType, eastl::string inArgument) = 0;
    virtual bool AddGlobalFunction(const CHAR *inFunctionName, lua_CFunction inFunction) = 0;
    virtual qt3ds::foundation::Option<SPresentationSize> GetPresentationSize() = 0;

    virtual void setAssetVisitor(qt3ds::UICAssetVisitor *) = 0;

public:
    static INDDView &Create(ITimeProvider &inProvider, IWindowSystem &inWindowSystem,
                            IAudioPlayer *inAudioPlayer = NULL);

public:
    QINDDViewSignalProxy *signalProxy();
private:
    QINDDViewSignalProxy m_SignalProxy;
};

class CTegraApplication
{
    //==============================================================================
    //	Fields
    //==============================================================================
private:
    qt3ds::foundation::NVScopedRefCounted<INDDView> m_NDDView;

public:
    CTegraApplication(ITimeProvider &inProvider, IWindowSystem &inWindowSystem,
                      IAudioPlayer *inAudioPlayer = 0);
    virtual ~CTegraApplication();
    // loading
    KDint BeginLoad(const QString &sourcePath);
    // asynchronous BeginLoad completed? That only valid for binary presentation, for text
    // presentation, always true
    bool HasOfflineLoadingCompleted() { return m_NDDView->HasOfflineLoadingCompleted(); }
    // Invokes m_ApplicationCore->CreateApplication(), a blocking function ensures binary loading
    // completed
    void InitializeGraphics(const QSurfaceFormat& format);

    void Cleanup() { m_NDDView->Cleanup(); }
    bool CanRender() { return m_NDDView->CanRender(); }
    void Render();
    bool WasLastFrameDirty() { return m_NDDView->WasLastFrameDirty(); }

    KDint HandleMessage(const KDEvent *inEvent);
    void Pause() { m_NDDView->Pause(); }
    void UnPause() { m_NDDView->UnPause(); }
    bool IsPaused() { return m_NDDView->IsPaused(); }
    INT32 GetFrameCount() { return m_NDDView->GetFrameCount(); }

public:
    CInputEngine *GetInputEngine() { return m_NDDView->GetInputEngine(); }
    // Only valid after InitializeGraphics
    ITegraApplicationRenderEngine *GetTegraRenderEngine()
    {
        return m_NDDView->GetTegraRenderEngine();
    }

public:
    void GoToSlideByName(const char *elementPath, const char *slideName)
    {
        m_NDDView->GoToSlideByName(elementPath, slideName);
    }
    void GoToSlideByIndex(const char *elementPath, const int slideIndex)
    {
        m_NDDView->GoToSlideByIndex(elementPath, slideIndex);
    }
    void GoToSlideRelative(const char *elementPath, const bool next, const bool wrap)
    {
        m_NDDView->GoToSlideRelative(elementPath, next, wrap);
    }
    bool GetSlideInfo(const char *elementPath, int &currentIndex, int &previousIndex,
                      QString &currentName, QString &previousName)
    {
        return m_NDDView->GetSlideInfo(elementPath, currentIndex, previousIndex,
                                       currentName, previousName);
    }
    void SetPresentationAttribute(const char *presId, const char *, const char *value)
    {
        m_NDDView->SetPresentationAttribute(presId, nullptr, value);
    }
    void GoToTime(const char *elementPath, const float time)
    {
        m_NDDView->GoToTime(elementPath, time);
    }
    void SetGlobalAnimationTime(qint64 inMilliSecs)
    {
        m_NDDView->SetGlobalAnimationTime(inMilliSecs);
    }
    void SetAttribute(const char *elementPath, const char *attributeName, const char *value)
    {
        m_NDDView->SetAttribute(elementPath, attributeName, value);
    }
    bool GetAttribute(const char *elementPath, const char *attributeName, void *value)
    {
        return m_NDDView->GetAttribute(elementPath, attributeName, value);
    }
    void FireEvent(const char *element, const char *evtName)
    {
        m_NDDView->FireEvent(element, evtName);
    }
    bool PeekCustomAction(char *&outElementPath, char *&outActionName)
    {
        return m_NDDView->PeekCustomAction(outElementPath, outActionName);
    }
    bool RegisterScriptCallback(int callbackType, qml_Function func, void *inUserData)
    {
        return m_NDDView->RegisterScriptCallback(callbackType, func, inUserData);
    };
    void FireEvent(const TEventCommandHash inEventType, eastl::string inArgument)
    {
        m_NDDView->FireEvent(inEventType, inArgument);
    }
    bool AddGlobalFunction(const CHAR *inFunctionName, lua_CFunction inFunction)
    {
        return m_NDDView->AddGlobalFunction(inFunctionName, inFunction);
    }
    qt3ds::foundation::Option<SPresentationSize> GetPrimaryPresentationSize()
    {
        return m_NDDView->GetPresentationSize();
    }
    qt3ds::foundation::NVScopedRefCounted<INDDView> getNDDView()
    {
        return m_NDDView;
    }
};
} // namespace Q3DStudio
