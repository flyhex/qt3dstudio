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
#include "UICRender.h"
#include "EABase/eabase.h" //char16_t definition
#include "UICRenderContext.h"
#include "foundation/StringTable.h"
#include "UICRenderNode.h"
#include "UICRenderBufferManager.h"
#include "UICRenderer.h"
#include "UICRenderResourceManager.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/Qt3DSAtomic.h"
#include "UICOffscreenRenderManager.h"
#include "UICTextRenderer.h"
#include "UICRenderInputStreamFactory.h"
#include "UICRenderEffectSystem.h"
#include "UICRenderShaderCache.h"
#include "foundation/Qt3DSFoundation.h"
#include "render/Qt3DSRenderFrameBuffer.h"
#include "render/Qt3DSRenderRenderBuffer.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "UICRenderCamera.h"
#include "foundation/Qt3DSContainers.h"
#include "UICRenderThreadPool.h"
#include "UICRenderImageBatchLoader.h"
#include "UICRenderTextTextureCache.h"
#include "UICRenderTextTextureAtlas.h"
#include "UICRenderPlugin.h"
#include "UICRenderDynamicObjectSystem.h"
#include "UICRenderCustomMaterialSystem.h"
#include "UICRenderPixelGraphicsRenderer.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "UICRenderBufferLoader.h"
#include "foundation/FastAllocator.h"
#include "foundation/AutoDeallocatorAllocator.h"
#include "UICRenderRenderList.h"
#include "UICRenderPathManager.h"
#include "UICRenderShaderCodeGeneratorV2.h"
#include "UICRenderDefaultMaterialShaderGenerator.h"
#include "UICRenderCustomMaterialShaderGenerator.h"

using namespace qt3ds::render;

namespace {

struct SRenderContextCore : public IUICRenderContextCore
{
    NVFoundationBase &m_Foundation;
    NVScopedRefCounted<IStringTable> m_StringTable;
    NVScopedRefCounted<IPerfTimer> m_PerfTimer;
    NVScopedRefCounted<IInputStreamFactory> m_InputStreamFactory;
    NVScopedRefCounted<IThreadPool> m_ThreadPool;
    NVScopedRefCounted<IDynamicObjectSystemCore> m_DynamicObjectSystem;
    NVScopedRefCounted<ICustomMaterialSystemCore> m_MaterialSystem;
    NVScopedRefCounted<IEffectSystemCore> m_EffectSystem;
    NVScopedRefCounted<IBufferLoader> m_BufferLoader;
    NVScopedRefCounted<IRenderPluginManagerCore> m_RenderPluginManagerCore;
    NVScopedRefCounted<ITextRendererCore> m_TextRenderer;
    NVScopedRefCounted<ITextRendererCore> m_OnscreenTexRenderer;
    NVScopedRefCounted<IPathManagerCore> m_PathManagerCore;

    QT3DSI32 mRefCount;
    SRenderContextCore(NVFoundationBase &fnd, IStringTable &strTable)
        : m_Foundation(fnd)
        , m_StringTable(strTable)
        , m_PerfTimer(IPerfTimer::CreatePerfTimer(fnd))
        , m_InputStreamFactory(IInputStreamFactory::Create(fnd))
        , m_ThreadPool(IThreadPool::CreateThreadPool(fnd, 4))
        , mRefCount(0)
    {
        m_DynamicObjectSystem = IDynamicObjectSystemCore::CreateDynamicSystemCore(*this);
        m_MaterialSystem = ICustomMaterialSystemCore::CreateCustomMaterialSystemCore(*this);
        m_EffectSystem = IEffectSystemCore::CreateEffectSystemCore(*this);
        m_RenderPluginManagerCore =
            IRenderPluginManagerCore::Create(fnd, strTable, *m_InputStreamFactory);
        m_BufferLoader = IBufferLoader::Create(m_Foundation, *m_InputStreamFactory, *m_ThreadPool);
        m_PathManagerCore = IPathManagerCore::CreatePathManagerCore(*this);
    }

    virtual ~SRenderContextCore() {}

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation.getAllocator())

    IStringTable &GetStringTable() override { return *m_StringTable; }
    NVFoundationBase &GetFoundation() override { return m_Foundation; }
    NVAllocatorCallback &GetAllocator() override { return m_Foundation.getAllocator(); }
    IInputStreamFactory &GetInputStreamFactory() override { return *m_InputStreamFactory; }
    IThreadPool &GetThreadPool() override { return *m_ThreadPool; }
    IDynamicObjectSystemCore &GetDynamicObjectSystemCore() override
    {
        return *m_DynamicObjectSystem;
    }
    ICustomMaterialSystemCore &GetMaterialSystemCore() override { return *m_MaterialSystem; }
    IEffectSystemCore &GetEffectSystemCore() override { return *m_EffectSystem; }
    IPerfTimer &GetPerfTimer() override { return *m_PerfTimer; }
    IBufferLoader &GetBufferLoader() override { return *m_BufferLoader; }
    IRenderPluginManagerCore &GetRenderPluginCore() override { return *m_RenderPluginManagerCore; }
    IPathManagerCore &GetPathManagerCore() override { return *m_PathManagerCore; }
    IUICRenderContext &CreateRenderContext(NVRenderContext &inContext,
                                                   const char8_t *inPrimitivesDirectory) override;
    void SetTextRendererCore(ITextRendererCore &inRenderer) override { m_TextRenderer = inRenderer; }
    ITextRendererCore *GetTextRendererCore() override { return m_TextRenderer.mPtr; }
    void SetOnscreenTextRendererCore(ITextRendererCore &inRenderer) override
    {
        m_OnscreenTexRenderer = inRenderer;
    }
    ITextRendererCore *GetOnscreenTextRendererCore() override { return m_OnscreenTexRenderer.mPtr; }
};

inline float Clamp(float val, float inMin = 0.0f, float inMax = 1.0f)
{
    if (val < inMin)
        return inMin;
    if (val > inMax)
        return inMax;
    return val;
}

struct SPerFrameAllocator : public NVAllocatorCallback
{
    SFastAllocator<> m_FastAllocator;
    SSAutoDeallocatorAllocator m_LargeAllocator;

    SPerFrameAllocator(NVAllocatorCallback &baseAllocator)
        : m_FastAllocator(baseAllocator, "PerFrameAllocation")
        , m_LargeAllocator(baseAllocator)
    {
    }

    inline void *allocate(size_t inSize, const char *inFile, int inLine)
    {
        if (inSize < 8192)
            return m_FastAllocator.allocate(inSize, "PerFrameAllocation", inFile, inLine, 0);
        else
            return m_LargeAllocator.allocate(inSize, "PerFrameAllocation", inFile, inLine, 0);
    }

    inline void *allocate(size_t inSize, const char *inFile, int inLine, int, int)
    {
        if (inSize < 8192)
            return m_FastAllocator.allocate(inSize, "PerFrameAllocation", inFile, inLine, 0);
        else
            return m_LargeAllocator.allocate(inSize, "PerFrameAllocation", inFile, inLine, 0);
    }

    inline void deallocate(void *, size_t) {}

    void reset()
    {
        m_FastAllocator.reset();
        m_LargeAllocator.deallocateAllAllocations();
    }

    void *allocate(size_t inSize, const char *typeName, const char *inFile, int inLine,
                           int flags = 0) override
    {
        if (inSize < SFastAllocator<>::SlabSize)
            return m_FastAllocator.allocate(inSize, typeName, inFile, inLine, flags);
        else
            return m_LargeAllocator.allocate(inSize, typeName, inFile, inLine, flags);
    }

    void *allocate(size_t inSize, const char *typeName, const char *inFile, int inLine,
                           size_t alignment, size_t alignmentOffset) override
    {
        if (inSize < SFastAllocator<>::SlabSize)
            return m_FastAllocator.allocate(inSize, typeName, inFile, inLine, alignment,
                                            alignmentOffset);
        else
            return m_LargeAllocator.allocate(inSize, typeName, inFile, inLine, alignment,
                                             alignmentOffset);
    }

    void deallocate(void *) override {}
};

struct SRenderContext : public IUICRenderContext
{
    NVScopedRefCounted<NVRenderContext> m_RenderContext;
    NVScopedRefCounted<IUICRenderContextCore> m_CoreContext;
    NVScopedRefCounted<IStringTable> m_StringTable;
    NVScopedRefCounted<IPerfTimer> m_PerfTimer;
    NVScopedRefCounted<IInputStreamFactory> m_InputStreamFactory;
    NVScopedRefCounted<IBufferManager> m_BufferManager;
    NVScopedRefCounted<IResourceManager> m_ResourceManager;
    NVScopedRefCounted<IOffscreenRenderManager> m_OffscreenRenderManager;
    NVScopedRefCounted<IUICRenderer> m_Renderer;
    NVScopedRefCounted<ITextRenderer> m_TextRenderer;
    NVScopedRefCounted<ITextRenderer> m_OnscreenTextRenderer;
    NVScopedRefCounted<ITextTextureCache> m_TextTextureCache;
    NVScopedRefCounted<ITextTextureAtlas> m_TextTextureAtlas;
    NVScopedRefCounted<IDynamicObjectSystem> m_DynamicObjectSystem;
    NVScopedRefCounted<IEffectSystem> m_EffectSystem;
    NVScopedRefCounted<IShaderCache> m_ShaderCache;
    NVScopedRefCounted<IThreadPool> m_ThreadPool;
    NVScopedRefCounted<IImageBatchLoader> m_ImageBatchLoader;
    NVScopedRefCounted<IRenderPluginManager> m_RenderPluginManager;
    NVScopedRefCounted<ICustomMaterialSystem> m_CustomMaterialSystem;
    NVScopedRefCounted<IPixelGraphicsRenderer> m_PixelGraphicsRenderer;
    NVScopedRefCounted<IPathManager> m_PathManager;
    NVScopedRefCounted<IShaderProgramGenerator> m_ShaderProgramGenerator;
    NVScopedRefCounted<IDefaultMaterialShaderGenerator> m_DefaultMaterialShaderGenerator;
    NVScopedRefCounted<ICustomMaterialShaderGenerator> m_CustomMaterialShaderGenerator;
    SPerFrameAllocator m_PerFrameAllocator;
    NVScopedRefCounted<IRenderList> m_RenderList;
    QT3DSU32 m_FrameCount;
    volatile QT3DSI32 mRefCount;
    // Viewport that this render context should use
    Option<NVRenderRect> m_Viewport;
    SWindowDimensions m_WindowDimensions;
    ScaleModes::Enum m_ScaleMode;
    bool m_WireframeMode;
    bool m_IsInSubPresentation;
    Option<QT3DSVec4> m_SceneColor;
    Option<QT3DSVec4> m_MatteColor;
    RenderRotationValues::Enum m_Rotation;
    NVScopedRefCounted<NVRenderFrameBuffer> m_RotationFBO;
    NVScopedRefCounted<NVRenderTexture2D> m_RotationTexture;
    NVScopedRefCounted<NVRenderRenderBuffer> m_RotationDepthBuffer;
    NVRenderFrameBuffer *m_ContextRenderTarget;
    NVRenderRect m_PresentationViewport;
    SWindowDimensions m_PresentationDimensions;
    SWindowDimensions m_RenderPresentationDimensions;
    SWindowDimensions m_PreRenderPresentationDimensions;
    QT3DSVec2 m_PresentationScale;
    NVRenderRect m_VirtualViewport;
    NVScopedRefCounted<NVRenderTexture2D> m_Watermark;
    QT3DSVec2 m_WatermarkLocation;
    QPair<QT3DSF32, int> m_FPS;
    bool m_AuthoringMode;

    SRenderContext(NVRenderContext &ctx, IUICRenderContextCore &inCore,
                   const char8_t *inApplicationDirectory)
        : m_RenderContext(ctx)
        , m_CoreContext(inCore)
        , m_StringTable(ctx.GetStringTable())
        , m_PerfTimer(inCore.GetPerfTimer())
        , m_InputStreamFactory(inCore.GetInputStreamFactory())
        , m_BufferManager(
              IBufferManager::Create(ctx, *m_StringTable, *m_InputStreamFactory, *m_PerfTimer))
        , m_ResourceManager(IResourceManager::CreateResourceManager(ctx))
        , m_ShaderCache(IShaderCache::CreateShaderCache(ctx, *m_InputStreamFactory, *m_PerfTimer))
        , m_ThreadPool(inCore.GetThreadPool())
        , m_RenderList(IRenderList::CreateRenderList(ctx.GetFoundation()))
        , m_PerFrameAllocator(ctx.GetAllocator())
        , m_FrameCount(0)
        , mRefCount(0)
        , m_WindowDimensions(800, 480)
        , m_ScaleMode(ScaleModes::ExactSize)
        , m_WireframeMode(false)
        , m_IsInSubPresentation(false)
        , m_Rotation(RenderRotationValues::NoRotation)
        , m_ContextRenderTarget(NULL)
        , m_PresentationScale(0, 0)
        , m_WatermarkLocation(1, 1)
        , m_FPS(qMakePair(0.0, 0))
        , m_AuthoringMode(false)
    {
        m_OffscreenRenderManager = IOffscreenRenderManager::CreateOffscreenRenderManager(
            ctx.GetAllocator(), *m_StringTable, *m_ResourceManager, *this);
        m_Renderer = IUICRenderer::CreateRenderer(*this);
        if (inApplicationDirectory && *inApplicationDirectory)
            m_InputStreamFactory->AddSearchDirectory(inApplicationDirectory);

        m_ImageBatchLoader =
            IImageBatchLoader::CreateBatchLoader(ctx.GetFoundation(), *m_InputStreamFactory,
                                                 *m_BufferManager, *m_ThreadPool, *m_PerfTimer);
        m_RenderPluginManager = inCore.GetRenderPluginCore().GetRenderPluginManager(ctx);
        m_DynamicObjectSystem = inCore.GetDynamicObjectSystemCore().CreateDynamicSystem(*this);
        m_EffectSystem = inCore.GetEffectSystemCore().GetEffectSystem(*this);
        m_CustomMaterialSystem = inCore.GetMaterialSystemCore().GetCustomMaterialSystem(*this);
        // as does the custom material system
        m_PixelGraphicsRenderer = IPixelGraphicsRenderer::CreateRenderer(*this, *m_StringTable);
        ITextRendererCore *theTextCore = inCore.GetTextRendererCore();
        m_ShaderProgramGenerator = IShaderProgramGenerator::CreateProgramGenerator(*this);
        m_DefaultMaterialShaderGenerator =
            IDefaultMaterialShaderGenerator::CreateDefaultMaterialShaderGenerator(*this);
        m_CustomMaterialShaderGenerator =
            ICustomMaterialShaderGenerator::CreateCustomMaterialShaderGenerator(*this);
        if (theTextCore) {
            m_TextRenderer = theTextCore->GetTextRenderer(ctx);
            m_TextTextureCache = ITextTextureCache::CreateTextureCache(
                m_RenderContext->GetFoundation(), *m_TextRenderer, *m_RenderContext);
        }

        ITextRendererCore *theOnscreenTextCore = inCore.GetOnscreenTextRendererCore();
        if (theOnscreenTextCore) {
            m_OnscreenTextRenderer = theOnscreenTextCore->GetTextRenderer(ctx);
            m_TextTextureAtlas = ITextTextureAtlas::CreateTextureAtlas(
                m_RenderContext->GetFoundation(), *m_OnscreenTextRenderer, *m_RenderContext);
        }
        m_PathManager = inCore.GetPathManagerCore().OnRenderSystemInitialize(*this);

        QString versionString;
        switch ((QT3DSU32)ctx.GetRenderContextType()) {
        case NVRenderContextValues::GLES2:
            versionString = QLatin1Literal("gles2");
            break;
        case NVRenderContextValues::GL2:
            versionString = QLatin1Literal("gl2");
            break;
        case NVRenderContextValues::GLES3:
            versionString = QLatin1Literal("gles3");
            break;
        case NVRenderContextValues::GL3:
            versionString = QLatin1Literal("gl3");
            break;
        case NVRenderContextValues::GLES3PLUS:
            versionString = QLatin1Literal("gles3x");
            break;
        case NVRenderContextValues::GL4:
            versionString = QLatin1Literal("gl4");
            break;
        default:
            break;
        }

        GetDynamicObjectSystem().setShaderCodeLibraryVersion(versionString);
#if defined (QT3DS_SHADER_PLATFORM_LIBRARY_DIR)
        const QString platformDirectory;
#if defined(_WIN32)
        platformDirectory = QStringLiteral("res\\platform\\win");
#elif defined(_LINUX)
        platformDirectory = QStringLiteral("res\\platform\\linux");
#elif defined(_MACOSX)
        platformDirectory = QStringLiteral("res\\platform\\macos");
#endif
        GetDynamicObjectSystem().setShaderCodeLibraryPlatformDirectory(platformDirectory);
#endif
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_RenderContext->GetAllocator());

    IStringTable &GetStringTable() override { return *m_StringTable; }
    NVFoundationBase &GetFoundation() override { return m_RenderContext->GetFoundation(); }
    NVAllocatorCallback &GetAllocator() override { return m_RenderContext->GetAllocator(); }
    IUICRenderer &GetRenderer() override { return *m_Renderer; }
    IBufferManager &GetBufferManager() override { return *m_BufferManager; }
    IResourceManager &GetResourceManager() override { return *m_ResourceManager; }
    NVRenderContext &GetRenderContext() override { return *m_RenderContext; }
    IOffscreenRenderManager &GetOffscreenRenderManager() override
    {
        return *m_OffscreenRenderManager;
    }
    IInputStreamFactory &GetInputStreamFactory() override { return *m_InputStreamFactory; }
    IEffectSystem &GetEffectSystem() override { return *m_EffectSystem; }
    IShaderCache &GetShaderCache() override { return *m_ShaderCache; }
    IThreadPool &GetThreadPool() override { return *m_ThreadPool; }
    IImageBatchLoader &GetImageBatchLoader() override { return *m_ImageBatchLoader; }
    ITextTextureCache *GetTextureCache() override { return m_TextTextureCache.mPtr; }
    ITextTextureAtlas *GetTextureAtlas() override { return m_TextTextureAtlas.mPtr; }
    IRenderPluginManager &GetRenderPluginManager() override { return *m_RenderPluginManager; }
    IDynamicObjectSystem &GetDynamicObjectSystem() override { return *m_DynamicObjectSystem; }
    ICustomMaterialSystem &GetCustomMaterialSystem() override { return *m_CustomMaterialSystem; }
    IPixelGraphicsRenderer &GetPixelGraphicsRenderer() override { return *m_PixelGraphicsRenderer; }
    IPerfTimer &GetPerfTimer() override { return *m_PerfTimer; }
    IRenderList &GetRenderList() override { return *m_RenderList; }
    IPathManager &GetPathManager() override { return *m_PathManager; }
    IShaderProgramGenerator &GetShaderProgramGenerator() override
    {
        return *m_ShaderProgramGenerator;
    }
    IDefaultMaterialShaderGenerator &GetDefaultMaterialShaderGenerator() override
    {
        return *m_DefaultMaterialShaderGenerator;
    }
    ICustomMaterialShaderGenerator &GetCustomMaterialShaderGenerator() override
    {
        return *m_CustomMaterialShaderGenerator;
    }
    NVAllocatorCallback &GetPerFrameAllocator() override { return m_PerFrameAllocator; }

    QT3DSU32 GetFrameCount() override { return m_FrameCount; }
    void SetFPS(QPair<QT3DSF32, int> inFPS) override { m_FPS = inFPS; }
    QPair<QT3DSF32, int> GetFPS(void) override { return m_FPS; }

    bool IsAuthoringMode() override { return m_AuthoringMode; }
    void SetAuthoringMode(bool inMode) override { m_AuthoringMode = inMode; }

    bool IsInSubPresentation() override { return m_IsInSubPresentation; }
    void SetInSubPresentation(bool inValue) override { m_IsInSubPresentation = inValue; }

    ITextRenderer *GetTextRenderer() override { return m_TextRenderer; }

    ITextRenderer *GetOnscreenTextRenderer() override { return m_OnscreenTextRenderer; }

    void SetSceneColor(Option<QT3DSVec4> inSceneColor) override { m_SceneColor = inSceneColor; }
    void SetMatteColor(Option<QT3DSVec4> inMatteColor) override { m_MatteColor = inMatteColor; }

    void SetWindowDimensions(const SWindowDimensions &inWindowDimensions) override
    {
        m_WindowDimensions = inWindowDimensions;
    }

    SWindowDimensions GetWindowDimensions() override { return m_WindowDimensions; }

    void SetScaleMode(ScaleModes::Enum inMode) override { m_ScaleMode = inMode; }

    ScaleModes::Enum GetScaleMode() override { return m_ScaleMode; }

    void SetWireframeMode(bool inEnable) override { m_WireframeMode = inEnable; }

    bool GetWireframeMode() override { return m_WireframeMode; }

    void SetWatermark(NVRenderTexture2D &inTexture) override
    {
        qCInfo(TRACE_INFO, "UICContext -- Setting watermark texture");
        m_Watermark = inTexture;
    }

    void SetWatermarkLocation(QT3DSVec2 inLocation) override
    {
        m_WatermarkLocation = QT3DSVec2(Clamp(inLocation.x), Clamp(inLocation.y));
    }

    void SetViewport(Option<NVRenderRect> inViewport) override { m_Viewport = inViewport; }
    Option<NVRenderRect> GetViewport() const override { return m_Viewport; }

    eastl::pair<NVRenderRect, NVRenderRect> GetPresentationViewportAndOuterViewport() const
    {
        SWindowDimensions thePresentationDimensions(m_PresentationDimensions);
        NVRenderRect theOuterViewport(GetContextViewport());
        if (m_Rotation == RenderRotationValues::Clockwise90
            || m_Rotation == RenderRotationValues::Clockwise270) {
            eastl::swap(theOuterViewport.m_Width, theOuterViewport.m_Height);
            eastl::swap(theOuterViewport.m_X, theOuterViewport.m_Y);
        }
        // Calculate the presentation viewport perhaps with the window width and height swapped.
        return eastl::make_pair(
            GetPresentationViewport(theOuterViewport, m_ScaleMode, thePresentationDimensions),
            theOuterViewport);
    }

    NVRenderRectF GetDisplayViewport() const override
    {
        return GetPresentationViewportAndOuterViewport().first;
    }

    void SetPresentationDimensions(const SWindowDimensions &inPresentationDimensions) override
    {
        m_PresentationDimensions = inPresentationDimensions;
    }
    SWindowDimensions GetCurrentPresentationDimensions() const override
    {
        return m_PresentationDimensions;
    }

    void SetRenderRotation(RenderRotationValues::Enum inRotation) override
    {
        m_Rotation = inRotation;
    }

    RenderRotationValues::Enum GetRenderRotation() const override { return m_Rotation; }
    QT3DSVec2 GetMousePickViewport() const override
    {
        bool renderOffscreen = m_Rotation != RenderRotationValues::NoRotation;
        if (renderOffscreen)
            return QT3DSVec2((QT3DSF32)m_PresentationViewport.m_Width,
                          (QT3DSF32)m_PresentationViewport.m_Height);
        else
            return QT3DSVec2((QT3DSF32)m_WindowDimensions.m_Width, (QT3DSF32)m_WindowDimensions.m_Height);
    }
    NVRenderRect GetContextViewport() const override
    {
        NVRenderRect retval;
        if (m_Viewport.hasValue())
            retval = *m_Viewport;
        else
            retval = NVRenderRect(0, 0, m_WindowDimensions.m_Width, m_WindowDimensions.m_Height);

        return retval;
    }

    QT3DSVec2 GetMousePickMouseCoords(const QT3DSVec2 &inMouseCoords) const override
    {
        bool renderOffscreen = m_Rotation != RenderRotationValues::NoRotation;
        if (renderOffscreen) {
            SWindowDimensions thePresentationDimensions(m_RenderPresentationDimensions);
            NVRenderRect theViewport(GetContextViewport());
            // Calculate the presentation viewport perhaps with the presentation width and height
            // swapped.
            NVRenderRect thePresentationViewport =
                GetPresentationViewport(theViewport, m_ScaleMode, thePresentationDimensions);
            // Translate pick into presentation space without rotations or anything else.
            QT3DSF32 YHeightDiff = (QT3DSF32)((QT3DSF32)m_WindowDimensions.m_Height
                                        - (QT3DSF32)thePresentationViewport.m_Height);
            QT3DSVec2 theLocalMouse((inMouseCoords.x - thePresentationViewport.m_X),
                                 (inMouseCoords.y - YHeightDiff + thePresentationViewport.m_Y));
            switch (m_Rotation) {
            default:
            case RenderRotationValues::NoRotation:
                QT3DS_ASSERT(false);
                break;
            case RenderRotationValues::Clockwise90:
                eastl::swap(theLocalMouse.x, theLocalMouse.y);
                theLocalMouse.y = thePresentationViewport.m_Width - theLocalMouse.y;
                break;
            case RenderRotationValues::Clockwise180:
                theLocalMouse.y = thePresentationViewport.m_Height - theLocalMouse.y;
                theLocalMouse.x = thePresentationViewport.m_Width - theLocalMouse.x;
                break;
            case RenderRotationValues::Clockwise270:
                eastl::swap(theLocalMouse.x, theLocalMouse.y);
                theLocalMouse.x = thePresentationViewport.m_Height - theLocalMouse.x;
                break;
            }
            return theLocalMouse;
        }
        return inMouseCoords;
    }

    NVRenderRect GetPresentationViewport(const NVRenderRect &inViewerViewport,
                                         ScaleModes::Enum inScaleToFit,
                                         const SWindowDimensions &inPresDimensions) const
    {
        NVRenderRect retval;
        QT3DSI32 theWidth = inViewerViewport.m_Width;
        QT3DSI32 theHeight = inViewerViewport.m_Height;
        if (inPresDimensions.m_Width == 0 || inPresDimensions.m_Height == 0)
            return NVRenderRect(0, 0, 0, 0);
        // Setup presentation viewport.  This may or may not match the physical viewport that we
        // want to setup.
        // Avoiding scaling keeps things as sharp as possible.
        if (inScaleToFit == ScaleModes::ExactSize) {
            retval.m_Width = inPresDimensions.m_Width;
            retval.m_Height = inPresDimensions.m_Height;
            retval.m_X = (theWidth - (QT3DSI32)inPresDimensions.m_Width) / 2;
            retval.m_Y = (theHeight - (QT3DSI32)inPresDimensions.m_Height) / 2;
        } else if (inScaleToFit == ScaleModes::ScaleToFit
                   || inScaleToFit == ScaleModes::FitSelected) {
            // Scale down in such a way to preserve aspect ratio.
            float screenAspect = (float)theWidth / (float)theHeight;
            float thePresentationAspect =
                (float)inPresDimensions.m_Width / (float)inPresDimensions.m_Height;
            if (screenAspect >= thePresentationAspect) {
                // if the screen height is the limiting factor
                retval.m_Y = 0;
                retval.m_Height = theHeight;
                retval.m_Width = (QT3DSI32)(thePresentationAspect * retval.m_Height);
                retval.m_X = (theWidth - retval.m_Width) / 2;
            } else {
                retval.m_X = 0;
                retval.m_Width = theWidth;
                retval.m_Height = (QT3DSI32)(retval.m_Width / thePresentationAspect);
                retval.m_Y = (theHeight - retval.m_Height) / 2;
            }
        } else {
            // Setup the viewport for everything and let the presentations figure it out.
            retval.m_X = 0;
            retval.m_Y = 0;
            retval.m_Width = theWidth;
            retval.m_Height = theHeight;
        }
        retval.m_X += inViewerViewport.m_X;
        retval.m_Y += inViewerViewport.m_Y;
        return retval;
    }

    void RenderText2D(QT3DSF32 x, QT3DSF32 y, qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor,
                              const char *text) override
    {
        m_Renderer->RenderText2D(x, y, inColor, text);
    }

    void RenderGpuProfilerStats(QT3DSF32 x, QT3DSF32 y,
                                        qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor) override
    {
        m_Renderer->RenderGpuProfilerStats(x, y, inColor);
    }

    NVRenderRect GetPresentationViewport() const override { return m_PresentationViewport; }
    struct SBeginFrameResult
    {
        bool m_RenderOffscreen;
        SWindowDimensions m_PresentationDimensions;
        bool m_ScissorTestEnabled;
        NVRenderRect m_ScissorRect;
        NVRenderRect m_Viewport;
        SWindowDimensions m_FBODimensions;
        SBeginFrameResult(bool ro, SWindowDimensions presDims, bool scissorEnabled,
                          NVRenderRect scissorRect, NVRenderRect viewport,
                          SWindowDimensions fboDims)
            : m_RenderOffscreen(ro)
            , m_PresentationDimensions(presDims)
            , m_ScissorTestEnabled(scissorEnabled)
            , m_ScissorRect(scissorRect)
            , m_Viewport(viewport)
            , m_FBODimensions(fboDims)
        {
        }
        SBeginFrameResult() {}
    };

    // Calculated values passed from beginframe to setupRenderTarget.
    // Trying to avoid duplicate code as much as possible.
    SBeginFrameResult m_BeginFrameResult;

    void BeginFrame() override
    {
        m_PreRenderPresentationDimensions = m_PresentationDimensions;
        SWindowDimensions thePresentationDimensions(m_PreRenderPresentationDimensions);
        NVRenderRect theContextViewport(GetContextViewport());
        m_PerFrameAllocator.reset();
        IRenderList &theRenderList(*m_RenderList);
        theRenderList.BeginFrame();
        if (m_Viewport.hasValue()) {
            theRenderList.SetScissorTestEnabled(true);
            theRenderList.SetScissorRect(theContextViewport);
        } else {
            theRenderList.SetScissorTestEnabled(false);
        }
        bool renderOffscreen = m_Rotation != RenderRotationValues::NoRotation;
        eastl::pair<NVRenderRect, NVRenderRect> thePresViewportAndOuterViewport =
            GetPresentationViewportAndOuterViewport();
        NVRenderRect theOuterViewport = thePresViewportAndOuterViewport.second;
        // Calculate the presentation viewport perhaps with the window width and height swapped.
        NVRenderRect thePresentationViewport = thePresViewportAndOuterViewport.first;
        m_PresentationViewport = thePresentationViewport;
        m_PresentationScale = QT3DSVec2(
            (QT3DSF32)thePresentationViewport.m_Width / (QT3DSF32)thePresentationDimensions.m_Width,
            (QT3DSF32)thePresentationViewport.m_Height / (QT3DSF32)thePresentationDimensions.m_Height);
        SWindowDimensions fboDimensions;
        if (thePresentationViewport.m_Width > 0 && thePresentationViewport.m_Height > 0) {
            if (renderOffscreen == false) {
                m_PresentationDimensions = SWindowDimensions(thePresentationViewport.m_Width,
                                                             thePresentationViewport.m_Height);
                m_RenderList->SetViewport(thePresentationViewport);
                if (thePresentationViewport.m_X || thePresentationViewport.m_Y
                    || thePresentationViewport.m_Width != (QT3DSI32)theOuterViewport.m_Width
                    || thePresentationViewport.m_Height != (QT3DSI32)theOuterViewport.m_Height) {
                    m_RenderList->SetScissorRect(thePresentationViewport);
                    m_RenderList->SetScissorTestEnabled(true);
                }
            } else {
                QT3DSU32 imageWidth = ITextRenderer::NextMultipleOf4(thePresentationViewport.m_Width);
                QT3DSU32 imageHeight =
                    ITextRenderer::NextMultipleOf4(thePresentationViewport.m_Height);
                fboDimensions = SWindowDimensions(imageWidth, imageHeight);
                m_PresentationDimensions = SWindowDimensions(thePresentationViewport.m_Width,
                                                             thePresentationViewport.m_Height);
                NVRenderRect theSceneViewport = NVRenderRect(0, 0, imageWidth, imageHeight);
                m_RenderList->SetScissorTestEnabled(false);
                m_RenderList->SetViewport(theSceneViewport);
            }
        }

        m_BeginFrameResult = SBeginFrameResult(
            renderOffscreen, m_PresentationDimensions, m_RenderList->IsScissorTestEnabled(),
            m_RenderList->GetScissor(), m_RenderList->GetViewport(), fboDimensions);

        m_Renderer->BeginFrame();
        m_OffscreenRenderManager->BeginFrame();
        if (m_TextRenderer)
            m_TextRenderer->BeginFrame();
        if (m_TextTextureCache)
            m_TextTextureCache->BeginFrame();
        m_ImageBatchLoader->BeginFrame();
    }

    QT3DSVec2 GetPresentationScaleFactor() const override { return m_PresentationScale; }

    virtual void SetupRenderTarget()
    {
        NVRenderRect theContextViewport(GetContextViewport());
        if (m_Viewport.hasValue()) {
            m_RenderContext->SetScissorTestEnabled(true);
            m_RenderContext->SetScissorRect(theContextViewport);
        } else {
            m_RenderContext->SetScissorTestEnabled(false);
        }
        bool clearedScene = false;
        {
            QT3DSVec4 theClearColor;
            if (m_MatteColor.hasValue())
                theClearColor = m_MatteColor;
            else {
                theClearColor = m_SceneColor;
                clearedScene = true;
            }
            m_RenderContext->SetClearColor(theClearColor);
            m_RenderContext->Clear(qt3ds::render::NVRenderClearValues::Color);
        }
        bool renderOffscreen = m_BeginFrameResult.m_RenderOffscreen;
        m_RenderContext->SetViewport(m_BeginFrameResult.m_Viewport);
        m_RenderContext->SetScissorRect(m_BeginFrameResult.m_ScissorRect);
        m_RenderContext->SetScissorTestEnabled(m_BeginFrameResult.m_ScissorTestEnabled);

        if (m_PresentationViewport.m_Width > 0 && m_PresentationViewport.m_Height > 0) {
            if (renderOffscreen == false) {
                if (m_RotationFBO != NULL) {
                    m_ResourceManager->Release(*m_RotationFBO);
                    m_ResourceManager->Release(*m_RotationTexture);
                    m_ResourceManager->Release(*m_RotationDepthBuffer);
                    m_RotationFBO = NULL;
                    m_RotationTexture = NULL;
                    m_RotationDepthBuffer = NULL;
                }
                if (m_SceneColor.hasValue() && !clearedScene) {
                    m_RenderContext->SetClearColor(m_SceneColor);
                    m_RenderContext->Clear(qt3ds::render::NVRenderClearValues::Color);
                }
            } else {
                QT3DSU32 imageWidth = m_BeginFrameResult.m_FBODimensions.m_Width;
                QT3DSU32 imageHeight = m_BeginFrameResult.m_FBODimensions.m_Height;
                NVRenderTextureFormats::Enum theColorBufferFormat = NVRenderTextureFormats::RGBA8;
                NVRenderRenderBufferFormats::Enum theDepthBufferFormat =
                    NVRenderRenderBufferFormats::Depth16;
                m_ContextRenderTarget = m_RenderContext->GetRenderTarget();
                if (m_RotationFBO == NULL) {
                    m_RotationFBO = m_ResourceManager->AllocateFrameBuffer();
                    m_RotationTexture = m_ResourceManager->AllocateTexture2D(
                        imageWidth, imageHeight, theColorBufferFormat);
                    m_RotationDepthBuffer = m_ResourceManager->AllocateRenderBuffer(
                        imageWidth, imageHeight, theDepthBufferFormat);
                    m_RotationFBO->Attach(NVRenderFrameBufferAttachments::Color0,
                                          *m_RotationTexture);
                    m_RotationFBO->Attach(NVRenderFrameBufferAttachments::Depth,
                                          *m_RotationDepthBuffer);
                } else {
                    STextureDetails theDetails = m_RotationTexture->GetTextureDetails();
                    if (theDetails.m_Width != imageWidth || theDetails.m_Height != imageHeight) {
                        m_RotationTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, imageWidth,
                                                          imageHeight, theColorBufferFormat);
                        m_RotationDepthBuffer->SetDimensions(
                            qt3ds::render::NVRenderRenderBufferDimensions(imageWidth, imageHeight));
                    }
                }
                m_RenderContext->SetRenderTarget(m_RotationFBO);
                if (m_SceneColor.hasValue()) {
                    m_RenderContext->SetClearColor(m_SceneColor);
                    m_RenderContext->Clear(qt3ds::render::NVRenderClearValues::Color);
                }
            }
        }
    }

    void RunRenderTasks() override
    {
        m_RenderList->RunRenderTasks();
        SetupRenderTarget();
    }

    // Note this runs before EndFrame
    virtual void TeardownRenderTarget()
    {
        if (m_Watermark) {
            STextureDetails theWatermarkDims = m_Watermark->GetTextureDetails();
            SWindowDimensions thePresentationDimensions(m_PresentationDimensions);
            NVRenderRect theViewport(GetContextViewport());
            // Calculate the presentation viewport perhaps with the presentation width and height
            // swapped.
            NVRenderRect thePresentationViewport =
                GetPresentationViewport(theViewport, m_ScaleMode, thePresentationDimensions);
            if (m_RotationFBO) {
                QT3DSU32 imageWidth = ITextRenderer::NextMultipleOf4(thePresentationViewport.m_Width);
                QT3DSU32 imageHeight =
                    ITextRenderer::NextMultipleOf4(thePresentationViewport.m_Height);
                m_RenderContext->SetRenderTarget(m_RotationFBO);
                NVRenderRect theSceneViewport = NVRenderRect(0, 0, imageWidth, imageHeight);
                m_RenderContext->SetViewport(theSceneViewport);
            } else {
                m_RenderContext->SetRenderTarget(m_ContextRenderTarget);
                m_RenderContext->SetViewport(thePresentationViewport);
            }
            NVRenderRect theRenderViewport = m_RenderContext->GetViewport();
            if (theRenderViewport.m_Width && theRenderViewport.m_Height) {
                double theDesiredTextureArea =
                    theRenderViewport.m_Width * theRenderViewport.m_Height * 1.0 / 100;
                double theMultiplier = sqrt(
                    theDesiredTextureArea / (theWatermarkDims.m_Width * theWatermarkDims.m_Height));
                double theTargetWidth = theWatermarkDims.m_Width * theMultiplier;
                double theTargetHeight = theWatermarkDims.m_Height * theMultiplier;
                double theInset = theTargetHeight / 4.0f;
                double theInsetRectWidth = theRenderViewport.m_Width - 2 * theInset;
                double theInsetRectHeight = theRenderViewport.m_Height - 2 * theInset;
                double rangeWidth = theInsetRectWidth - theTargetWidth;
                double rangeHeight = theInsetRectHeight - theTargetHeight;
                double leftOffset = rangeWidth * m_WatermarkLocation.x;
                double bottomOffset = theInsetRectHeight - (rangeHeight * m_WatermarkLocation.y);
                theRenderViewport.m_X += (QT3DSI32)leftOffset;
                theRenderViewport.m_Y += (QT3DSI32)bottomOffset;
                theRenderViewport.m_Width = (QT3DSI32)theTargetWidth;
                theRenderViewport.m_Height = (QT3DSI32)theTargetHeight;
                m_RenderContext->SetViewport(theRenderViewport);
                m_RenderContext->SetDepthTestEnabled(false);
                m_RenderContext->SetBlendingEnabled(true);
                m_RenderContext->SetBlendFunction(qt3ds::render::NVRenderBlendFunctionArgument(
                    qt3ds::render::NVRenderSrcBlendFunc::SrcAlpha,
                    qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha,
                    qt3ds::render::NVRenderSrcBlendFunc::One,
                    qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha));
                m_RenderContext->SetBlendEquation(qt3ds::render::NVRenderBlendEquationArgument(
                    NVRenderBlendEquation::Add, NVRenderBlendEquation::Add));
                m_RenderContext->SetScissorTestEnabled(false);
                SCamera theCamera;
                theCamera.MarkDirty(NodeTransformDirtyFlag::TransformIsDirty);
                theCamera.m_Flags.SetOrthographic(true);
                QT3DSVec2 theCameraDimensions((QT3DSF32)theRenderViewport.m_Width,
                                           (QT3DSF32)theRenderViewport.m_Height);
                theCamera.CalculateGlobalVariables(NVRenderRect(0, 0,
                                                                (QT3DSU32)theRenderViewport.m_Width,
                                                                (QT3DSU32)theRenderViewport.m_Height),
                                                   theCameraDimensions);

                QT3DSMat44 theVP;
                theCamera.CalculateViewProjectionMatrix(theVP);
                SNode theTempNode;
                theTempNode.CalculateGlobalVariables();
                QT3DSMat44 theMVP;
                QT3DSMat33 theNormalMat;
                theTempNode.CalculateMVPAndNormalMatrix(theVP, theMVP, theNormalMat);
                // Render fullscreen quad with the screen being defined as where the texture should
                // go.
                m_Renderer->RenderQuad(
                    QT3DSVec2((QT3DSF32)theRenderViewport.m_Width, (QT3DSF32)theRenderViewport.m_Height),
                    theMVP, *m_Watermark);
            }
        }
        if (m_RotationFBO) {
            ScaleModes::Enum theScaleToFit = m_ScaleMode;
            NVRenderRect theOuterViewport(GetContextViewport());
            m_RenderContext->SetRenderTarget(m_ContextRenderTarget);
            SWindowDimensions thePresentationDimensions = GetCurrentPresentationDimensions();
            if (m_Rotation == RenderRotationValues::Clockwise90
                || m_Rotation == RenderRotationValues::Clockwise270) {
                eastl::swap(thePresentationDimensions.m_Width, thePresentationDimensions.m_Height);
            }
            m_RenderPresentationDimensions = thePresentationDimensions;
            // Calculate the presentation viewport perhaps with the presentation width and height
            // swapped.
            NVRenderRect thePresentationViewport =
                GetPresentationViewport(theOuterViewport, theScaleToFit, thePresentationDimensions);
            SCamera theCamera;
            switch (m_Rotation) {
            default:
                QT3DS_ASSERT(false);
                break;
            case RenderRotationValues::Clockwise90:
                theCamera.m_Rotation.z = 90;
                break;
            case RenderRotationValues::Clockwise180:
                theCamera.m_Rotation.z = 180;
                break;
            case RenderRotationValues::Clockwise270:
                theCamera.m_Rotation.z = 270;
                break;
            }
            TORAD(theCamera.m_Rotation.z);
            theCamera.MarkDirty(NodeTransformDirtyFlag::TransformIsDirty);
            theCamera.m_Flags.SetOrthographic(true);
            m_RenderContext->SetViewport(thePresentationViewport);
            QT3DSVec2 theCameraDimensions((QT3DSF32)thePresentationViewport.m_Width,
                                       (QT3DSF32)thePresentationViewport.m_Height);
            theCamera.CalculateGlobalVariables(
                NVRenderRect(0, 0, (QT3DSU32)thePresentationViewport.m_Width,
                             (QT3DSU32)thePresentationViewport.m_Height),
                theCameraDimensions);
            QT3DSMat44 theVP;
            theCamera.CalculateViewProjectionMatrix(theVP);
            SNode theTempNode;
            theTempNode.CalculateGlobalVariables();
            QT3DSMat44 theMVP;
            QT3DSMat33 theNormalMat;
            theTempNode.CalculateMVPAndNormalMatrix(theVP, theMVP, theNormalMat);
            m_RenderContext->SetCullingEnabled(false);
            m_RenderContext->SetBlendingEnabled(false);
            m_RenderContext->SetDepthTestEnabled(false);
            m_Renderer->RenderQuad(QT3DSVec2((QT3DSF32)m_PresentationViewport.m_Width,
                                          (QT3DSF32)m_PresentationViewport.m_Height),
                                   theMVP, *m_RotationTexture);
        }
    }

    void EndFrame() override
    {
        TeardownRenderTarget();
        m_ImageBatchLoader->EndFrame();
        if (m_TextTextureCache)
            m_TextTextureCache->EndFrame();
        if (m_TextRenderer)
            m_TextRenderer->EndFrame();
        m_OffscreenRenderManager->EndFrame();
        m_Renderer->EndFrame();
        m_CustomMaterialSystem->EndFrame();
        m_PresentationDimensions = m_PreRenderPresentationDimensions;
        ++m_FrameCount;
    }
};

IUICRenderContext &SRenderContextCore::CreateRenderContext(NVRenderContext &inContext,
                                                           const char8_t *inPrimitivesDirectory)
{
    return *QT3DS_NEW(m_Foundation.getAllocator(), SRenderContext)(inContext, *this,
                                                                inPrimitivesDirectory);
}
}

IUICRenderContextCore &IUICRenderContextCore::Create(NVFoundationBase &fnd, IStringTable &strt)
{
    return *QT3DS_NEW(fnd.getAllocator(), SRenderContextCore)(fnd, strt);
}
