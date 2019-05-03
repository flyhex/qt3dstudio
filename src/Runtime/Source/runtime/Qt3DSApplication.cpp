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

// We need a Qt header first here because Qt's metatype system insists that Bool
// can not be defined first before Qt headers are included and the includes below
// define Bool by way of Xll/XLib.h via khronos -> egl -> X11
#include <QImage>

#include "RuntimePrefix.h"
#include "Qt3DSApplication.h"
#include "Qt3DSApplicationValues.h"
#include "foundation/Qt3DSAtomic.h"
#include "Qt3DSMemory.h"
#include "Qt3DSRuntimeFactory.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/FileTools.h"
#include "Qt3DSIScriptBridge.h"
#include "foundation/Qt3DSOption.h"
#include "foundation/XML.h"
#include "foundation/IOStreams.h"
#include "foundation/Qt3DSContainers.h"
#include "EASTL/hash_map.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSInputEventTypes.h"
#include "Qt3DSSceneManager.h"
#include "Qt3DSIScene.h"
#include "Qt3DSInputEngine.h"
#include "Qt3DSMetadata.h"
#include "Qt3DSUIPParser.h"
#include "foundation/Socket.h"
#include "EventPollingSystem.h"
#include "Qt3DSRenderContextCore.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "foundation/SerializationTypes.h"
#include "EASTL/sort.h"
#include "Qt3DSRenderBufferLoader.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSSync.h"
#include "Qt3DSTextRenderer.h"
#include "Qt3DSRenderThreadPool.h"
#include "foundation/StringConversionImpl.h"
#include "Qt3DSRenderLoadedTexture.h"
#include "render/Qt3DSRenderContext.h"
#include "Qt3DSActivationManager.h"
#include "Qt3DSRenderer.h"
#include "Qt3DSRenderShaderCache.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "Qt3DSAudioPlayer.h"
#include "Qt3DSElementSystem.h"
#include "Qt3DSSlideSystem.h"
#include "Qt3DSQmlElementHelper.h"
#include "Qt3DSRenderBufferManager.h"
#include "Qt3DSRenderRenderList.h"
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qpair.h>
#include <QtCore/qdir.h>
#include "q3dsvariantconfig_p.h"

using namespace qt3ds;
using namespace qt3ds::runtime;
using namespace qt3ds::render;
using namespace Q3DStudio;

namespace qt3ds {
namespace foundation {
template <>
struct StringConversion<QT3DSVec2>
{
    void StrTo(const char8_t *buffer, QT3DSVec2 &item)
    {
        char *endPtr = NULL;
        item.x = (float)strtod(buffer, &endPtr);
        if (endPtr)
            item.y = (float)strtod(endPtr, NULL);
    }
};
}
}

struct SFrameTimer
{
    int m_FrameCount;
    QT3DSU64 m_FrameTime;
    SFrameTimer(QT3DSU64 fc = 0)
        : m_FrameCount(fc)
        , m_FrameTime(qt3ds::foundation::Time::getCurrentCounterValue())
    {
    }

    QT3DSF32 GetElapsedSeconds(QT3DSU64 currentTime) const
    {
        QT3DSU64 diff = currentTime - m_FrameTime;
        QT3DSF32 diffNanos
                = static_cast<QT3DSF32>(qt3ds::foundation::Time::sCounterFreq.toTensOfNanos(diff));
        return diffNanos / qt3ds::foundation::Time::sNumTensOfNanoSecondsInASecond;
    }

    QT3DSF32 GetElapsedSeconds() const
    {
        return GetElapsedSeconds(qt3ds::foundation::Time::getCurrentCounterValue());
    }

    QPair<QT3DSF32, int> GetFPS(int updateFC)
    {
        int elapsedFrames = updateFC - m_FrameCount;
        QT3DSU64 currentTime = qt3ds::foundation::Time::getCurrentCounterValue();
        QT3DSF32 elapsedSeconds = GetElapsedSeconds(currentTime);
        QT3DSF32 retval = elapsedFrames / elapsedSeconds;
        m_FrameCount = updateFC;
        m_FrameTime = currentTime;
        return qMakePair(retval, elapsedFrames);
    }
};

struct SRefCountedAssetValue : public SAssetValue
{
    NVFoundationBase &m_Foundation;
    QT3DSI32 mRefCount;
    SRefCountedAssetValue(NVFoundationBase &fnd)
        : SAssetValue()
        , m_Foundation(fnd)
        , mRefCount(0)
    {
    }

    SRefCountedAssetValue(NVFoundationBase &fnd, const SAssetValue &asset)
        : SAssetValue(asset)
        , m_Foundation(fnd)
        , mRefCount(0)
    {
    }

    SRefCountedAssetValue(NVFoundationBase &fnd, const SPresentationAsset &asset)
        : SAssetValue(asset)
        , m_Foundation(fnd)
        , mRefCount(0)
    {
    }

    SRefCountedAssetValue(NVFoundationBase &fnd, const SBehaviorAsset &asset)
        : SAssetValue(asset)
        , m_Foundation(fnd)
        , mRefCount(0)
    {
    }

    SRefCountedAssetValue(NVFoundationBase &fnd, const SRenderPluginAsset &asset)
        : SAssetValue(asset)
        , m_Foundation(fnd)
        , mRefCount(0)
    {
    }

    SRefCountedAssetValue(NVFoundationBase &fnd, const SSCXMLAsset &asset)
        : SAssetValue(asset)
        , m_Foundation(fnd)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())
};

typedef nvhash_map<CRegisteredString, NVScopedRefCounted<SRefCountedAssetValue>> TIdAssetMap;
typedef nvhash_map<THashValue, CRegisteredString> THashStrMap;
typedef nvvector<eastl::pair<CRegisteredString, NVScopedRefCounted<SRefCountedAssetValue>>>
TIdAssetList;
typedef eastl::pair<QT3DSU32, TElement *> THandleElementPair;
typedef NVConstDataRef<THandleElementPair> THandleElementDataBuffer;
typedef nvvector<THandleElementDataBuffer> THandleElementDataBufferList;
typedef nvhash_map<QT3DSU32, TElement *> THandleElementMap;

struct SHandleElementPairComparator
{
    bool operator()(const THandleElementPair &lhs, const THandleElementPair &rhs) const
    {
        return lhs.first < rhs.first;
    }
};

struct SSlideResourceCounter
{
    QHash<QString, int> counters;
    QSet<QString> createSet;
    QSet<QString> deleteSet;

    QVector<QString> loadedSlides;

    void increment(const QSet<QString> &set)
    {
        for (auto &r : set) {
            if (counters.value(r, 0) == 0)
                createSet.insert(r);
            counters[r]++;
        }
    }
    void decrement(const QSet<QString> &set)
    {
        for (auto &r : set) {
            if (counters.contains(r)) {
                int count = qMax(counters[r] - 1, 0);
                if (count == 0)
                    deleteSet.insert(r);
                counters[r] = count;
            }
        }
    }
    void begin()
    {
        createSet.clear();
        deleteSet.clear();
    }
    void reset()
    {
        loadedSlides.clear();
        counters.clear();
        begin();
    }
    bool isImage(const QString &path)
    {
        int index = path.lastIndexOf(".");
        if (index < 0)
            return false;
        const QString ext = path.right(path.length() - index - 1);
        return (ext == QLatin1String("jpg") || ext == QLatin1String("jpeg")
                || ext == QLatin1String("png") || ext == QLatin1String("hdr")
                || ext == QLatin1String("dds") || ext == QLatin1String("ktx"));
    }
    QSet<QString> toImageSet(const QVector<QString> &vec)
    {
        QSet<QString> s;
        for (const auto &x : vec) {
            if (isImage(x))
                s.insert(x);
        }
        return s;
    }
    void handleLoadSlide(const QString &slide, SSlideKey key, ISlideSystem &slideSystem)
    {
        if (loadedSlides.contains(slide))
            return;
        loadedSlides.push_back(slide);
        begin();
        increment(toImageSet(slideSystem.GetSourcePaths(key)));
        print();
    }
    void handleUnloadSlide(const QString &slide, SSlideKey key, ISlideSystem &slideSystem)
    {
        if (!loadedSlides.contains(slide))
            return;
        loadedSlides.removeOne(slide);
        begin();
        decrement(toImageSet(slideSystem.GetSourcePaths(key)));
        print();
    }
    void print()
    {
        static const bool debugging = qEnvironmentVariableIntValue("QT3DS_DEBUG") >= 1;
        if (debugging) {
            qDebug() << "SlideResourceCounter resources:";
            const auto keys = counters.keys();
            for (auto &x : keys)
                qDebug() << x << ": " << counters[x];
            if (createSet.size()) {
                qDebug() << "New resources: ";
                for (auto y : qAsConst(createSet))
                    qDebug() << y;
            }
            if (deleteSet.size()) {
                qDebug() << "Deleted resources: ";
                for (auto y : qAsConst(deleteSet))
                    qDebug() << y;
            }
        }
    }
};

struct STextureUploadRenderTask : public IRenderTask
{
    IBufferManager &m_bufferManager;
    QSet<QString> m_uploadSet;
    QSet<QString> m_deleteSet;
    QMutex m_updateMutex;

    STextureUploadRenderTask(IBufferManager &mgr)
        : m_bufferManager(mgr)
    {

    }
    void Run() override
    {
        QMutexLocker loc(&m_updateMutex);
        m_bufferManager.loadSet(m_uploadSet);
        m_bufferManager.unloadSet(m_deleteSet);
    }
    void add(const QSet<QString> &set)
    {
        QMutexLocker loc(&m_updateMutex);
        m_uploadSet.unite(set);
        m_deleteSet.subtract(set);
    }
    void remove(const QSet<QString> &set)
    {
        QMutexLocker loc(&m_updateMutex);
        m_uploadSet.subtract(set);
        m_deleteSet.unite(set);
    }
    bool persistent() const override
    {
        return true;
    }
};

struct SApp;

class IAppLoadContext : public NVRefCounted
{
public:
    virtual void EndLoad() = 0;
    virtual bool OnGraphicsInitialized(IRuntimeFactory &inFactory) = 0;
    virtual bool HasCompletedLoading() = 0;
    static IAppLoadContext &CreateXMLLoadContext(
            SApp &inApp, const char8_t *inScaleMode);
};

inline float Clamp(float val, float inMin = 0.0f, float inMax = 1.0f)
{
    if (val < inMin)
        return inMin;
    if (val > inMax)
        return inMax;
    return val;
}

// A set of common settings that may come from the UIA or from the command line.
// command line settings always override uia settings.
struct SApplicationSettings
{
    Option<bool> m_LayerCacheEnabled;
    Option<bool> m_LayerGpuProfilingEnabled;
    Option<bool> m_ShaderCachePersistenceEnabled;

    SApplicationSettings() {}

    template <typename TDataType>
    static Option<TDataType> Choose(const Option<TDataType> &inCommandLine,
                                    const Option<TDataType> &inUIAFile)
    {
        if (inCommandLine.hasValue())
            return inCommandLine;
        return inUIAFile;
    }

    SApplicationSettings(const SApplicationSettings &inCommandLine,
                         const SApplicationSettings &inUIAFileSettings)
        : m_LayerCacheEnabled(
              Choose(inCommandLine.m_LayerCacheEnabled, inUIAFileSettings.m_LayerCacheEnabled))
        , m_LayerGpuProfilingEnabled(Choose(inCommandLine.m_LayerGpuProfilingEnabled,
                                            inUIAFileSettings.m_LayerGpuProfilingEnabled))
        , m_ShaderCachePersistenceEnabled(Choose(inCommandLine.m_ShaderCachePersistenceEnabled,
                                                 inUIAFileSettings.m_ShaderCachePersistenceEnabled))
    {
    }

    static const char8_t *LayerCacheName() { return "layer-caching"; }
    static const char8_t *LayerGpuProfilerName() { return "layer-gpu-profiling"; }
    static const char8_t *ShaderCacheName() { return "shader-cache-persistence"; }

    void ParseBoolEnableDisableItem(const IDOMReader &inReader, const char8_t *itemName,
                                    Option<bool> &itemValue)
    {
        const char8_t *inItem;
        if (const_cast<IDOMReader &>(inReader).UnregisteredAtt(itemName, inItem)) {
            if (AreEqualCaseless(inItem, "disabled"))
                itemValue = false;
            else
                itemValue = true;
        }
    }

    void ParseBoolEnableDisableItem(const eastl::vector<eastl::string> &inCommandLine,
                                    const char8_t *itemName, Option<bool> &itemValue)
    {
        eastl::string temp;
        temp.assign("-");
        temp.append(itemName);
        for (QT3DSU32 idx = 0, end = inCommandLine.size(); idx < end; ++idx) {
            if (inCommandLine[idx].find(temp) == 0) {
                if (inCommandLine[idx].length() == temp.size()) {
                    qCWarning(qt3ds::INVALID_OPERATION)
                            << "Unable to parse parameter %s. Please pass =enable|disable as "
                            << "part of the parameter. " << temp.c_str();
                } else {
                    temp = inCommandLine[idx].substr(temp.size() + 1);
                    eastl::string::size_type start = temp.find_first_of("'\"");
                    if (start != eastl::string::npos)
                        temp.erase(0, start);

                    eastl::string::size_type end = temp.find_first_of("'\"");
                    if (end != eastl::string::npos)
                        temp.erase(end);
                    if (AreEqualCaseless(temp.c_str(), "disabled"))
                        itemValue = false;
                    else
                        itemValue = true;
                    qCInfo(qt3ds::INVALID_OPERATION)
                            << "Item " << itemName
                            << (itemValue ? " enabled" : " disabled");
                }
            }
        }
    }

    template <typename TReaderType>
    void ParseItems(const TReaderType &inReader)
    {
        ParseBoolEnableDisableItem(inReader, LayerCacheName(), m_LayerCacheEnabled);
        ParseBoolEnableDisableItem(inReader, LayerGpuProfilerName(), m_LayerGpuProfilingEnabled);
        ParseBoolEnableDisableItem(inReader, ShaderCacheName(), m_ShaderCachePersistenceEnabled);
    }

    void Parse(IDOMReader &inReader) { ParseItems(inReader); }

    void Parse(const eastl::vector<eastl::string> &inCommandLine) { ParseItems(inCommandLine); }

    struct SOptionSerializer
    {
        bool m_HasValue;
        bool m_Value;
        QT3DSU8 m_Padding[2];
        SOptionSerializer(const Option<bool> &inValue = Empty())
            : m_HasValue(inValue.hasValue())
            , m_Value(inValue.hasValue() ? *inValue : false)
        {
            m_Padding[0] = 0;
            m_Padding[1] = 0;
        }

        operator Option<bool>() const
        {
            if (m_HasValue)
                return m_Value;
            return Empty();
        }
    };

    void Save(IOutStream &outStream) const
    {
        outStream.Write(SOptionSerializer(m_LayerCacheEnabled));
        outStream.Write(SOptionSerializer(m_LayerGpuProfilingEnabled));
        outStream.Write(SOptionSerializer(m_ShaderCachePersistenceEnabled));
    }

    void Load(IInStream &inStream)
    {
        SOptionSerializer s;
        inStream.Read(s);
        m_LayerCacheEnabled = s;
        inStream.Read(s);
        m_LayerGpuProfilingEnabled = s;
        inStream.Read(s);
        m_ShaderCachePersistenceEnabled = s;
    }
};

struct SDummyAudioPlayer : public IAudioPlayer
{
    virtual ~SDummyAudioPlayer() {}
    bool PlaySoundFile(const char *inFilePath) override
    {
        (void *)inFilePath;
        qCWarning(qt3ds::TRACE_INFO)
                << "Qt3DSTegraApplication: Unimplemented method IAudioPlayer::PlaySoundFile";
        return false;
    }
} g_DummyAudioPlayer;

struct SAudioPlayerWrapper : public IAudioPlayer
{
private:
    IApplication *m_Application;
    IAudioPlayer *m_RealPlayer;

public:
    SAudioPlayerWrapper()
        : m_Application(0)
        , m_RealPlayer(&g_DummyAudioPlayer)
    {
    }
    virtual ~SAudioPlayerWrapper() {}

    void SetApplication(IApplication &inApplication) { m_Application = &inApplication; }

    void SetPlayer(IAudioPlayer *inPlayer)
    {
        if (inPlayer)
            m_RealPlayer = inPlayer;
        else
            m_RealPlayer = &g_DummyAudioPlayer;
    }

    bool PlaySoundFile(const char *inFilePath) override
    {
        eastl::string theFilePath(nonNull(inFilePath));
        if (m_RealPlayer != &g_DummyAudioPlayer) {
            qt3ds::foundation::CFileTools::CombineBaseAndRelative(
                        m_Application->GetProjectDirectory().c_str(), inFilePath, theFilePath);
        }
        return m_RealPlayer->PlaySoundFile(theFilePath.c_str());
    }
};

struct SApp : public IApplication
{
    NVScopedRefCounted<Q3DStudio::IRuntimeFactoryCore> m_CoreFactory;
    NVScopedRefCounted<Q3DStudio::IRuntimeFactory> m_RuntimeFactory;

    Q3DStudio::CInputEngine *m_InputEnginePtr;
    CAppStr m_ApplicationDir;
    CAppStr m_ProjectDir;
    CAppStr m_InitialPresentationId;
    CAppStr m_DLLDirectory;
    TIdAssetMap m_AssetMap;
    // Keep the assets ordered.  This enables the uia order to mean something.
    TIdAssetList m_OrderedAssets;
    SPickFrame m_PickFrame;
    SPickFrame m_MousePickCache;
    SPickFrame m_MouseOverCache;
    THashStrMap m_HashStrMap;
    CTimer m_Timer;
    Q3DStudio::INT64 m_ManualTime;
    SFrameTimer m_FrameTimer;
    Q3DStudio::INT32 m_FrameCount;
    // the name of the file without extension.
    eastl::string m_Filename;
    Q3DSVariantConfig m_variantConfig;
    QScopedPointer<STextureUploadRenderTask> m_uploadRenderTask;

    qt3ds::foundation::NVScopedReleasable<IRuntimeMetaData> m_MetaData;
    nvvector<eastl::pair<SBehaviorAsset, bool>> m_Behaviors;
    NVScopedRefCounted<SocketSystem> m_SocketSystem;
    NVScopedRefCounted<SocketStream> m_ServerStream;
    NVScopedRefCounted<IActivityZoneManager> m_ActivityZoneManager;
    NVScopedRefCounted<IElementAllocator> m_ElementAllocator;

    // Handles are loaded sorted but only added to the handle map when needed.
    nvvector<char8_t> m_LoadBuffer;
    nvvector<CPresentation *> m_PresentationBuffer;
    Mutex m_RunnableMutex;
    nvvector<NVScopedRefCounted<IAppRunnable>> m_ThreadRunnables;
    nvvector<NVScopedRefCounted<IAppRunnable>> m_MainThreadRunnables;
    NVScopedRefCounted<IAppLoadContext> m_AppLoadContext;
    bool m_HideFPS;
    bool m_DisableState;
    bool m_ProfileLogging;
    bool m_LastRenderWasDirty;
    QT3DSU64 m_LastFrameStartTime;
    QT3DSU64 m_ThisFrameStartTime;
    double m_MillisecondsSinceLastFrame;
    // We get odd oscillations if we do are too quick to report that the frame wasn't dirty
    // after input.
    int m_DirtyCountdown;
    SApplicationSettings m_UIAFileSettings;
    eastl::pair<NVDataRef<Q3DStudio::TElement *>, size_t> m_ElementLoadResult;

    SAudioPlayerWrapper m_AudioPlayer;

    Qt3DSAssetVisitor *m_visitor;

    bool m_createSuccessful;

    DataInputMap m_dataInputDefs;

    SSlideResourceCounter m_resourceCounter;
    QSet<QString> m_createSet;

    QT3DSI32 mRefCount;
    SApp(Q3DStudio::IRuntimeFactoryCore &inFactory, const char8_t *inAppDir)
        : m_CoreFactory(inFactory)
        , m_InputEnginePtr(NULL)
        , m_ApplicationDir(inFactory.GetFoundation().getAllocator())
        , m_ProjectDir(inFactory.GetFoundation().getAllocator())
        , m_InitialPresentationId(inFactory.GetFoundation().getAllocator())
        , m_DLLDirectory(inFactory.GetFoundation().getAllocator())
        , m_AssetMap(inFactory.GetFoundation().getAllocator(), "SApp::m_AssetMap")
        , m_OrderedAssets(inFactory.GetFoundation().getAllocator(), "SApp::m_OrderedAssets")
        , m_HashStrMap(inFactory.GetFoundation().getAllocator(), "SApp::m_HashStrMap")
        , m_Timer(inFactory.GetTimeProvider())
        , m_ManualTime(0)
        , m_FrameCount(0)
        , m_Behaviors(inFactory.GetFoundation().getAllocator(), "SApp::m_Behaviors")
        , m_ActivityZoneManager(IActivityZoneManager::CreateActivityZoneManager(
                                    inFactory.GetFoundation(), inFactory.GetStringTable()))
        , m_ElementAllocator(IElementAllocator::CreateElementAllocator(inFactory.GetFoundation(),
                                                                       inFactory.GetStringTable()))
        , m_LoadBuffer(inFactory.GetFoundation().getAllocator(), "SApp::m_LoadBuffer")
        , m_PresentationBuffer(inFactory.GetFoundation().getAllocator(),
                               "SApp::m_PresentationBuffer")
        , m_RunnableMutex(inFactory.GetFoundation().getAllocator())
        , m_ThreadRunnables(inFactory.GetFoundation().getAllocator(), "SApp::m_ThreadRunnables")
        , m_MainThreadRunnables(inFactory.GetFoundation().getAllocator(),
                                "SApp::m_MainThreadRunnables")
        , m_HideFPS(true)
        , m_DisableState(true)
        , m_ProfileLogging(false)
        , m_LastRenderWasDirty(true)
        , m_LastFrameStartTime(0)
        , m_ThisFrameStartTime(0)
        , m_MillisecondsSinceLastFrame(0)
        , m_DirtyCountdown(5)
        , m_visitor(nullptr)
        , m_createSuccessful(false)
        , mRefCount(0)
    {
        m_AudioPlayer.SetApplication(*this);
        eastl::string tempStr(inAppDir);
        CFileTools::NormalizePath(tempStr);
        m_ApplicationDir.assign(tempStr.c_str());

        Q3DStudio_memset(&m_PickFrame, 0, sizeof(SPickFrame));
        Q3DStudio_memset(&m_MousePickCache, 0, sizeof(SPickFrame));
        Q3DStudio_memset(&m_MouseOverCache, 0, sizeof(SPickFrame));

        m_Timer.Start();

        m_CoreFactory->SetApplicationCore(this);
        m_CoreFactory->GetScriptEngineQml().SetApplicationCore(*this);

        m_CoreFactory->AddSearchPath(tempStr.c_str());
    }

    ~SApp()
    {
        EndLoad();
        {
            Mutex::ScopedLock __locker(m_RunnableMutex);
            m_ThreadRunnables.clear();
        }
        // Ensure we stop the timer.
        HasCompletedLoading();
        m_AppLoadContext = NULL;

        for (QT3DSU32 idx = 0, end = m_OrderedAssets.size(); idx < end; ++idx) {
            SAssetValue &theAsset = *m_OrderedAssets[idx].second;
            if (theAsset.getType() == AssetValueTypes::Presentation) {
                SPresentationAsset &thePresAsset = *theAsset.getDataPtr<SPresentationAsset>();
                if (thePresAsset.m_Presentation) {
                    Q3DStudio_delete(thePresAsset.m_Presentation, CPresentation);
                    thePresAsset.m_Presentation = NULL;
                }
            }
        }
    }

    void setPresentationId(const QString &id) override
    {
        m_InitialPresentationId.assign(qPrintable(id));
    }

    void setAssetVisitor(qt3ds::Qt3DSAssetVisitor *v) override
    {
        m_visitor = v;
    }

    NVDataRef<CPresentation *> GetPresentations()
    {
        if (m_PresentationBuffer.empty()) {
            for (QT3DSU32 idx = 0, end = m_OrderedAssets.size(); idx < end; ++idx) {
                SAssetValue &theAsset = *m_OrderedAssets[idx].second;
                if (theAsset.getType() == AssetValueTypes::Presentation) {
                    SPresentationAsset &thePresAsset = *theAsset.getDataPtr<SPresentationAsset>();
                    if (thePresAsset.m_Presentation)
                        m_PresentationBuffer.push_back(thePresAsset.m_Presentation);
                }
            }
        }
        return m_PresentationBuffer;
    }
    QVector<CPresentation *> getPresentations()
    {
        QVector<CPresentation *> presentations;
        if (m_PresentationBuffer.empty()) {
            for (QT3DSU32 idx = 0, end = m_OrderedAssets.size(); idx < end; ++idx) {
                SAssetValue &theAsset = *m_OrderedAssets[idx].second;
                if (theAsset.getType() == AssetValueTypes::Presentation) {
                    SPresentationAsset &thePresAsset = *theAsset.getDataPtr<SPresentationAsset>();
                    if (thePresAsset.m_Presentation)
                        presentations.push_back(thePresAsset.m_Presentation);
                }
            }
        }
        return presentations;
    }

    void addRef() override { atomicIncrement(&mRefCount); }

    void release() override
    {
        atomicDecrement(&mRefCount);
        if (mRefCount <= 0)
            NVDelete(m_CoreFactory->GetFoundation().getAllocator(), this);
    }

    void QueueForMainThread(IAppRunnable &inRunnable) override
    {
        Mutex::ScopedLock __locker(m_RunnableMutex);
        m_ThreadRunnables.push_back(inRunnable);
    }

    virtual void EnableProfileLogging()
    {
        m_ProfileLogging = true;
        if (m_RuntimeFactory)
            m_RuntimeFactory->GetScriptEngineQml().EnableProfiling();
    }

    // Verbose logging is disabled by default.
    virtual void SetVerboseLogging(bool inEnableVerboseLogging)
    {

    }

    ////////////////////////////////////////////////////////////////////////////
    // Update rhythm implementations
    ////////////////////////////////////////////////////////////////////////////

    void SetPickFrame(const SPickFrame &inPickFrame)
    {
        // The model has changed, fire enter and exit mouse events
        if (inPickFrame.m_Model != m_PickFrame.m_Model) {
            // For determining onGroupedMouseOver/Out:
            // arg1 = the original onMouseOut model and arg2 = the original onMouseOver model
            UVariant theMouseOutModel;
            UVariant theMouseOverModel;
            theMouseOutModel.m_VoidPointer = m_PickFrame.m_Model;
            theMouseOverModel.m_VoidPointer = inPickFrame.m_Model;

            // It seems like you would want to 'onMouseOut' before you 'onMouseOver' something new?
            if (m_PickFrame.m_Model) {
                m_PickFrame.m_Model->GetBelongedPresentation()->FireEvent(
                            ON_MOUSEOUT, m_PickFrame.m_Model, &theMouseOutModel, &theMouseOverModel,
                            ATTRIBUTETYPE_POINTER, ATTRIBUTETYPE_POINTER);
            }
            if (inPickFrame.m_Model) {
                inPickFrame.m_Model->GetBelongedPresentation()->FireEvent(
                            ON_MOUSEOVER, inPickFrame.m_Model, &theMouseOutModel, &theMouseOverModel,
                            ATTRIBUTETYPE_POINTER, ATTRIBUTETYPE_POINTER);
                m_MouseOverCache = inPickFrame;
            }
        }

        const TEventCommandHash *theEventArray[] = { &ON_MOUSEDOWN,       &ON_MOUSEUP,
                                                     &ON_MIDDLEMOUSEDOWN, &ON_MIDDLEMOUSEUP,
                                                     &ON_RIGHTMOUSEDOWN,  &ON_RIGHTMOUSEUP };
        const TEventCommandHash *theClickEventArray[] = { &ON_MOUSECLICK, &ON_MIDDLEMOUSECLICK,
                                                          &ON_RIGHTMOUSECLICK };

        // Click events...
        // NOTE:  This is a fancy way to iterate programatically over all the handled mouse inputs
        // handled (declared in AKPickFrame.h for now)
        // we iterate to INPUTBUTTONCOUNT (see comment in AKPickFrame.h) * 2, because we handle
        // mouse down and up
        for (QT3DSI32 theMouseEvtIter = 0; theMouseEvtIter < MOUSEBUTTONCOUNT - 1;
             theMouseEvtIter++) {
            // we effectively iterate to MOUSEBUTTONCOUNT * 2 (see comment in AKPickFrame.h) to
            // handle mouse down and up
            QT3DSI32 theMouseDownFlag = 1 << (theMouseEvtIter * 2);
            QT3DSI32 theMouseUpFlag = 1 << (theMouseEvtIter * 2 + 1);

            // on*MouseDown
            // if this frame, the mouse button is down, and last frame it wasn't (new down click)
            if (inPickFrame.m_Model && inPickFrame.m_InputFrame.m_MouseFlags & theMouseDownFlag
                    && !(m_PickFrame.m_InputFrame.m_MouseFlags & theMouseDownFlag)) {
                // fire the 'on*MouseDown' event - which is at the even indices since the down
                // events for each button are before the up
                inPickFrame.m_Model->GetBelongedPresentation()->FireEvent(
                            *theEventArray[theMouseEvtIter * 2], inPickFrame.m_Model);

                // cache this as the last item we 'onMouseDown' on
                m_MousePickCache = inPickFrame;
            }

            // on*MouseUp
            // if we mouse up on anything, send the event
            if (inPickFrame.m_InputFrame.m_MouseFlags & theMouseUpFlag) {
                // fire the 'on*MouseUp' event - odd indices (1,3,5 etc)
                if (m_MousePickCache.m_Model) {
                    m_MousePickCache.m_Model->GetBelongedPresentation()->FireEvent(
                                *theEventArray[theMouseEvtIter * 2 + 1], m_MousePickCache.m_Model);
                }

                // on*MouseClick
                // if we had a up click on the same item we were mouse down on last frame ... we had
                // a click
                if (inPickFrame.m_Model && inPickFrame.m_Model == m_MousePickCache.m_Model) {
                    inPickFrame.m_Model->GetBelongedPresentation()->FireEvent(
                                *theClickEventArray[theMouseEvtIter], inPickFrame.m_Model);
                }

                // clear the stored 'last mouse down' since we just got a mouse up
                Q3DStudio_memset(&m_MousePickCache, 0, sizeof(SPickFrame));
            }

            // on*MouseDblClick
        }

        if (m_MouseOverCache.m_Model) {

            if (inPickFrame.m_InputFrame.m_MouseFlags & VSCROLLWHEEL) {
                UVariant theScrollValue;
                theScrollValue.m_INT32 = inPickFrame.m_InputFrame.m_ScrollValue;
                m_MouseOverCache.m_Model->GetBelongedPresentation()->FireEvent(
                            ON_VERTICALSCROLLWHEEL, m_MouseOverCache.m_Model, &theScrollValue,
                            NULL, ATTRIBUTETYPE_INT32);
            } else if (inPickFrame.m_InputFrame.m_MouseFlags & HSCROLLWHEEL) {
                UVariant theScrollValue;
                theScrollValue.m_INT32 = inPickFrame.m_InputFrame.m_ScrollValue;
                m_MouseOverCache.m_Model->GetBelongedPresentation()->FireEvent(
                            ON_HORIZONTALSCROLLWHEEL, m_MouseOverCache.m_Model, &theScrollValue,
                            NULL, ATTRIBUTETYPE_INT32);
            }
        }

        // Do this last
        m_PickFrame = inPickFrame;
    }

    void ClearPresentationDirtyLists()
    {
        NVDataRef<CPresentation *> thePresentations(GetPresentations());
        for (QT3DSU32 idx = 0, end = thePresentations.size(); idx < end; ++idx)
            thePresentations[idx]->ClearDirtyList();
    }

    void UpdatePresentations()
    {
        SStackPerfTimer __updateTimer(m_RuntimeFactory->GetPerfTimer(), "UpdatePresentations");
        // Transfer the input frame to the kernel for pick processing
        // the scene manager now handles the picking on each of its scenes
        SetPickFrame(m_RuntimeFactory->GetSceneManager().AdvancePickFrame(
                         m_InputEnginePtr->GetInputFrame()));
        // clear up mouse flag for horizontal and vertical scroll
        m_InputEnginePtr->GetInputFrame().m_MouseFlags &= !(HSCROLLWHEEL | VSCROLLWHEEL);

        // Update all the presentations.
        // Animations are advanced based on m_Timer by default, but this can be overridden via
        // SetTimeMilliSecs().
        Q3DStudio::INT64 globalTime(GetTimeMilliSecs());

        NVDataRef<CPresentation *> thePresentations(GetPresentations());

        {
            SStackPerfTimer __updateTimer(m_RuntimeFactory->GetPerfTimer(),
                                          "UpdatePresentations - pre update");
            for (QT3DSU32 idx = 0, end = m_OrderedAssets.size(); idx < end; ++idx) {
                if (m_OrderedAssets[idx].second->getType() == AssetValueTypes::Presentation) {
                    SPresentationAsset &theAsset(
                                *m_OrderedAssets[idx].second->getDataPtr<SPresentationAsset>());
                    CPresentation *thePresentation = theAsset.m_Presentation;
                    if (thePresentation && thePresentation->GetActive())
                        thePresentation->PreUpdate(globalTime);
                }
            }
        }
        {
            SStackPerfTimer __updateTimer(m_RuntimeFactory->GetPerfTimer(),
                                          "UpdatePresentations - begin update");
            for (QT3DSU32 idx = 0, end = m_OrderedAssets.size(); idx < end; ++idx) {
                if (m_OrderedAssets[idx].second->getType() == AssetValueTypes::Presentation) {
                    SPresentationAsset &theAsset(
                                *m_OrderedAssets[idx].second->getDataPtr<SPresentationAsset>());
                    CPresentation *thePresentation = theAsset.m_Presentation;
                    if (thePresentation && thePresentation->GetActive())
                        thePresentation->BeginUpdate();
                }
            }
        }
        {
            SStackPerfTimer __updateTimer(m_RuntimeFactory->GetPerfTimer(),
                                          "UpdatePresentations - end update");

            for (QT3DSU32 idx = 0, end = m_OrderedAssets.size(); idx < end; ++idx) {
                if (m_OrderedAssets[idx].second->getType() == AssetValueTypes::Presentation) {
                    SPresentationAsset &theAsset(
                                *m_OrderedAssets[idx].second->getDataPtr<SPresentationAsset>());
                    CPresentation *thePresentation = theAsset.m_Presentation;
                    // allow EndUpdate also for inactive presentations so that we can
                    // activate it
                    if (thePresentation)
                        thePresentation->EndUpdate();
                }
            }
        }
        {
            SStackPerfTimer __updateTimer(m_RuntimeFactory->GetPerfTimer(),
                                          "UpdatePresentations - postupdate");

            for (QT3DSU32 idx = 0, end = m_OrderedAssets.size(); idx < end; ++idx) {
                if (m_OrderedAssets[idx].second->getType() == AssetValueTypes::Presentation) {
                    SPresentationAsset &theAsset(
                                *m_OrderedAssets[idx].second->getDataPtr<SPresentationAsset>());
                    CPresentation *thePresentation = theAsset.m_Presentation;
                    // allow PostUpdate also for inactive presentations so that we can
                    // activate it
                    if (thePresentation)
                        thePresentation->PostUpdate(globalTime);
                }
            }
        }

        // Run the garbage collection
        m_CoreFactory->GetScriptEngineQml().StepGC();
    }

    void UpdateScenes() { m_RuntimeFactory->GetSceneManager().Update(); }

    void Render()
    {
        SStackPerfTimer __updateTimer(m_RuntimeFactory->GetPerfTimer(), "Render");
        CPresentation *pres = GetPrimaryPresentation();
        if (pres)
            m_LastRenderWasDirty = m_RuntimeFactory->GetSceneManager().RenderPresentation(pres);
    }

    void ResetDirtyCounter() { m_DirtyCountdown = 5; }

    // Update all the presentations and render them.
    void UpdateAndRender() override
    {
        QT3DS_ASSERT(m_AppLoadContext.mPtr == NULL);
        m_ThisFrameStartTime = qt3ds::foundation::Time::getCurrentCounterValue();
        if (m_LastFrameStartTime) {
            QT3DSU64 durationSinceLastFrame = m_ThisFrameStartTime - m_LastFrameStartTime;
            m_MillisecondsSinceLastFrame =
                    qt3ds::foundation::Time::sCounterFreq.toTensOfNanos(durationSinceLastFrame)
                    * (1.0 / 100000.0);
        } else {
            m_MillisecondsSinceLastFrame = 0;
        }

        ++m_FrameCount;

        // First off, update any application level behaviors.
        IScriptBridge &theScriptEngine = m_CoreFactory->GetScriptEngineQml();
        for (QT3DSU32 idx = 0, end = m_Behaviors.size(); idx < end; ++idx) {
            eastl::pair<SBehaviorAsset, bool> &entry(m_Behaviors[idx]);
            if (!entry.second) {
                entry.second = true;
                theScriptEngine.ExecuteApplicationScriptFunction(entry.first.m_Handle,
                                                                 "onInitialize");
            }
        }

        // TODO: Initialize presentations

        for (QT3DSU32 idx = 0, end = m_Behaviors.size(); idx < end; ++idx) {
            eastl::pair<SBehaviorAsset, bool> &entry(m_Behaviors[idx]);
            theScriptEngine.ExecuteApplicationScriptFunction(entry.first.m_Handle, "onUpdate");
        }

        UpdatePresentations();

        UpdateScenes();

        Render();

        m_InputEnginePtr->ClearInputFrame();
        ClearPresentationDirtyLists();

        if (!m_CoreFactory->GetEventSystem().GetAndClearEventFetchedFlag())
            m_CoreFactory->GetEventSystem().PurgeEvents(); // GetNextEvents of event system has not
                                                           // been called in this round, so clear
                                                           // events to avoid events to be piled up

        QT3DSU64 updateEndTime = qt3ds::foundation::Time::getCurrentCounterValue();
        QT3DSU64 updateDuration = updateEndTime - m_ThisFrameStartTime;
        double millis
                = qt3ds::foundation::Time::sCounterFreq.toTensOfNanos(updateDuration)
                * (1.0 / 100000);
        if (floor(m_FrameTimer.GetElapsedSeconds()) > 0.0f) {
            QPair<QT3DSF32, int> fps = m_FrameTimer.GetFPS(m_FrameCount);
            m_RuntimeFactory->GetQt3DSRenderContext().SetFPS(fps);
            if (m_ProfileLogging || !m_HideFPS) {
                qCInfo(PERF_INFO, "Render Statistics: %3.2ffps, frame count %d",
                       fps.first, fps.second);
            }
        }

        (void)millis;

        /*if ( millis > 30.0 )
        {
                m_CoreFactory->GetFoundation().error( NVErrorCode::eDEBUG_INFO, __FILE__, __LINE__,
        "Qt3DS Long Frame: %3.2fms", millis );
                //Useful for figuring out where the frame time comes from.
                m_CoreFactory->GetPerfTimer().OutputTimerData();

        }
        else*/
        m_CoreFactory->GetPerfTimer().ResetTimerData();

        fflush(stdout);
        m_LastFrameStartTime = m_ThisFrameStartTime;
        if (m_LastRenderWasDirty)
            ResetDirtyCounter();
        else
            m_DirtyCountdown = NVMax(0, m_DirtyCountdown - 1);
    }

    // hook this up to -layer-caching.
    // otherwise it might be hard to measure performance
    bool IsApplicationDirty() override
    {
        return (m_DirtyCountdown > 0);
    }

    double GetMillisecondsSinceLastFrame() override { return m_MillisecondsSinceLastFrame; }

    void MarkApplicationDirty() override { ResetDirtyCounter(); }

    Q3DStudio::IAudioPlayer &GetAudioPlayer() override { return m_AudioPlayer; }
    ////////////////////////////////////////////////////////////////////////////////
    // Generalized save/load
    ////////////////////////////////////////////////////////////////////////////////

    void loadComponentSlideResources(TElement *component, CPresentation *presentation, int index,
                                     const QString slideName)
    {
        if (m_RuntimeFactory->GetQt3DSRenderContext().GetBufferManager()
                .isReloadableResourcesEnabled()) {
            auto &slidesystem = presentation->GetSlideSystem();
            SSlideKey key;
            key.m_Component = component;
            key.m_Index = index;
            slidesystem.setUnloadSlide(key, false);
            const QString completeName = presentation->GetName() + QLatin1Char(':')
                    + QString::fromUtf8(key.m_Component->m_Name) + QLatin1Char(':') + slideName;
            qCInfo(PERF_INFO) << "Load component slide resources: " << completeName;
            m_resourceCounter.handleLoadSlide(completeName, key, slidesystem);
            if (m_uploadRenderTask)
                m_uploadRenderTask->add(m_resourceCounter.createSet);
            else
                m_createSet.unite(m_resourceCounter.createSet);
        }
    }

    void unloadComponentSlideResources(TElement *component, CPresentation *presentation, int index,
                                       const QString slideName)
    {
        if (m_RuntimeFactory->GetQt3DSRenderContext().GetBufferManager()
                .isReloadableResourcesEnabled()) {
            auto &slidesystem = presentation->GetSlideSystem();
            SSlideKey key;
            key.m_Component = component;
            key.m_Index = index;
            slidesystem.setUnloadSlide(key, true);
            if (!slidesystem.isActiveSlide(key)) {
                const QString completeName = presentation->GetName() + QLatin1Char(':')
                        + QString::fromUtf8(key.m_Component->m_Name) + QLatin1Char(':') + slideName;
                qCInfo(PERF_INFO) << "Unload component slide resources: " << completeName;
                m_resourceCounter.handleUnloadSlide(completeName, key, slidesystem);

                if (m_uploadRenderTask)
                    m_uploadRenderTask->remove(m_resourceCounter.deleteSet);
            }
        }
    }

    bool LoadUIP(SPresentationAsset &inAsset,
                 NVConstDataRef<SElementAttributeReference> inExternalReferences)
    {
        GetMetaData();
        eastl::string theFile;
        CFileTools::CombineBaseAndRelative(GetProjectDirectory().c_str(), inAsset.m_Src.c_str(),
                                           theFile);
        // Check if the file event exists
        eastl::string fullPath;
        NVScopedRefCounted<qt3ds::render::IRefCountedInputStream> theStream
                = m_CoreFactory->GetRenderContextCore().GetInputStreamFactory().GetStreamForFile(
                    theFile.c_str());
        if (theStream) {
            theStream = NULL;
            CPresentation *thePresentation
                    = Q3DStudio_new(CPresentation) CPresentation(inAsset.m_Id.c_str(), this);
            inAsset.m_Presentation = thePresentation;
            thePresentation->SetFilePath(theFile.c_str());
            NVScopedReleasable<IUIPParser> theUIPParser(IUIPParser::Create(
                                                            theFile.c_str(), *m_MetaData,
                                                            m_CoreFactory->GetInputStreamFactory(),
                                                            m_CoreFactory->GetStringTable()));
            Q3DStudio::IScene *newScene = NULL;
            m_PresentationBuffer.clear();
            if (theUIPParser->Load(*thePresentation, inExternalReferences)) {
                // Load the scene graph portion of the scene.
                newScene = m_RuntimeFactory->GetSceneManager().LoadScene(
                            thePresentation, theUIPParser.mPtr,
                            m_CoreFactory->GetScriptEngineQml(),
                            m_variantConfig);
            }

            if (newScene == NULL) {
                Q3DStudio_delete(thePresentation, CPresentation);
                qCWarning(qt3ds::INVALID_OPERATION)
                        << "Unable to load presentation " << theFile.c_str();
                inAsset.m_Presentation = NULL;
                return false;
            } else {
                if (inAsset.m_Id.IsValid() && m_InitialPresentationId.empty())
                    m_InitialPresentationId.assign(inAsset.m_Id);

                if (inAsset.m_Id.IsValid())
                    newScene->RegisterOffscreenRenderer(inAsset.m_Id);

                QVector<TElement *> components;
                thePresentation->GetRoot()->findComponents(components);
                for (auto &component : qAsConst(components))
                    loadComponentSlideResources(component, thePresentation, 0, "Master");

                return true;
            }
        }
        qCWarning(qt3ds::INVALID_OPERATION) << "Unable to load presentation " << theFile.c_str();
        return false;
    }

    bool LoadUIA(IDOMReader &inReader, NVFoundationBase &fnd)
    {
            IDOMReader::Scope __preparseScope(inReader);
        {
            m_UIAFileSettings.Parse(inReader);
        }
        {
            IDOMReader::Scope __assetsScope(inReader);
            if (!inReader.MoveToFirstChild("assets")) {
                qCCritical(INVALID_OPERATION,
                           "UIA input xml doesn't contain <assets> tag; load failed");
                return false;
            }

            eastl::string pathString;

            const char8_t *initialItem = "";
            inReader.UnregisteredAtt("initial", initialItem);
            m_InitialPresentationId.clear();
            if (!isTrivial(initialItem)) {
                if (initialItem[0] == '#')
                    ++initialItem;

                m_InitialPresentationId.assign(initialItem);
            }
            eastl::vector<SElementAttributeReference> theUIPReferences;
            eastl::string tempString;

            for (bool success = inReader.MoveToFirstChild(); success;
                 success = inReader.MoveToNextSibling()) {
                IDOMReader::Scope __assetScope(inReader);
                const char8_t *itemId("");
                inReader.UnregisteredAtt("id", itemId);
                const char8_t *src = "";
                inReader.UnregisteredAtt("src", src);
                pathString.clear();
                if (!isTrivial(src))
                    CFileTools::CombineBaseAndRelative(m_ProjectDir.c_str(), src, pathString);

                const char8_t *assetName = inReader.GetElementName();
                if (AreEqual(assetName, "presentation")) {
                    SPresentationAsset theAsset(RegisterStr(itemId), RegisterStr(src));
                    bool activeFlag;
                    if (inReader.Att("active", activeFlag))
                        theAsset.m_Active = activeFlag;
                    RegisterAsset(theAsset);
                } else if (AreEqual(assetName, "dataInput")) {
                    DataInputDef diDef;
                    const char8_t *name = "";
                    const char8_t *type = "";
                    const char8_t *evaluator = "";
                    diDef.value = QVariant::Invalid;
                    inReader.UnregisteredAtt("name", name);
                    inReader.UnregisteredAtt("type", type);
                    inReader.Att("min", diDef.min);
                    inReader.Att("max", diDef.max);
                    inReader.UnregisteredAtt("evaluator", evaluator);
                    if (AreEqual(type, "Ranged Number"))
                        diDef.type = DataInputTypeRangedNumber;
                    else if (AreEqual(type, "String"))
                        diDef.type = DataInputTypeString;
                    else if (AreEqual(type, "Float"))
                        diDef.type = DataInputTypeFloat;
                    else if (AreEqual(type, "Vector3"))
                        diDef.type = DataInputTypeVector3;
                    else if (AreEqual(type, "Vector2"))
                        diDef.type = DataInputTypeVector2;
                    else if (AreEqual(type, "Boolean"))
                        diDef.type = DataInputTypeBoolean;
                    else if (AreEqual(type, "Variant"))
                        diDef.type = DataInputTypeVariant;

                    if (AreEqual(type, "Evaluator")) {
                        diDef.type = DataInputTypeEvaluator;
                        diDef.evaluator = QString::fromUtf8(evaluator);
                    }

                    m_dataInputDefs.insert(QString::fromUtf8(name), diDef);
                } else if (AreEqual(assetName, "renderplugin")) {
                    const char8_t *pluginArgs = "";
                    inReader.UnregisteredAtt("args", pluginArgs);
                    RegisterAsset(SRenderPluginAsset(RegisterStr(itemId), RegisterStr(src),
                                                     RegisterStr(pluginArgs)));
                } else if (AreEqual(assetName, "behavior")) {
                    SBehaviorAsset theAsset(RegisterStr(itemId), RegisterStr(src), 0);
                    RegisterAsset(theAsset);
                } else if (AreEqual(assetName, "presentation-qml")) {
                    const char8_t *args = "";
                    inReader.UnregisteredAtt("args", args);
                    RegisterAsset(SQmlPresentationAsset(RegisterStr(itemId), RegisterStr(src),
                                                        RegisterStr(args)));
                } else {
                    qCWarning(WARNING, "Unrecognized <assets> child %s", assetName);
                }
            }
        } // end assets scope
        const char8_t *initialScaleMode = "";
        inReader.UnregisteredAtt("scalemode", initialScaleMode);

        m_AppLoadContext
                = IAppLoadContext::CreateXMLLoadContext(*this,
                                                        initialScaleMode);
        return true;
    }

    DataInputMap &dataInputMap() override
    {
        return m_dataInputDefs;
    }

    QList<QString> dataInputs() const override
    {
        return m_dataInputDefs.keys();
    }

    float dataInputMax(const QString &name) const override
    {
        return m_dataInputDefs[name].max;
    }

    float dataInputMin(const QString &name) const override
    {
        return m_dataInputDefs[name].min;
    }

    struct SAppXMLErrorHandler : public qt3ds::foundation::CXmlErrorHandler
    {
        NVFoundationBase &m_Foundation;
        const char8_t *m_FilePath;
        SAppXMLErrorHandler(NVFoundationBase &fnd, const char8_t *filePath)
            : m_Foundation(fnd)
            , m_FilePath(filePath)
        {
        }

        void OnXmlError(TXMLCharPtr errorName, int line, int /*column*/) override
        {
            qCWarning(INVALID_OPERATION, m_FilePath, line, "%s", errorName);
        }
    };

    bool BeginLoad(const QString &sourcePath, const QStringList &variantList) override
    {
        SStackPerfTimer __loadTimer(m_CoreFactory->GetPerfTimer(), "Application: Begin Load");
        eastl::string directory;
        eastl::string filename;
        eastl::string extension;
        CFileTools::Split(sourcePath.toUtf8().constData(), directory, filename, extension);
        eastl::string projectDirectory(directory);

        m_ProjectDir.assign(projectDirectory.c_str());
        m_CoreFactory->AddSearchPath(projectDirectory.c_str());

        // add additional search path
        QString projectDir = CFileTools::NormalizePathForQtUsage(projectDirectory.c_str());
        if (!projectDir.startsWith(QStringLiteral(":"))) {
            eastl::string relativeProjectDir;
            CFileTools::CombineBaseAndRelative(m_ApplicationDir.c_str(), projectDirectory.c_str(),
                                               relativeProjectDir);
            m_CoreFactory->AddSearchPath(relativeProjectDir.c_str());
        }

        NVFoundationBase &fnd(m_CoreFactory->GetFoundation());

        if (m_CoreFactory->GetRenderContextCore().getDistanceFieldRenderer()) {
            m_CoreFactory->GetRenderContextCore().getDistanceFieldRenderer()
                    ->AddProjectFontDirectory(projectDirectory.c_str());
        }

        if (m_CoreFactory->GetRenderContextCore().GetTextRendererCore()) {
            m_CoreFactory->GetRenderContextCore().GetTextRendererCore()->AddProjectFontDirectory(
                        projectDirectory.c_str());
            m_CoreFactory->GetRenderContextCore().GetTextRendererCore()->BeginPreloadFonts(
                        m_CoreFactory->GetRenderContextCore().GetThreadPool(),
                        m_CoreFactory->GetRenderContextCore().GetPerfTimer());
        }
        m_Filename = filename;
        m_variantConfig.setVariantList(variantList);
        bool retval = false;
        if (extension.comparei("uip") == 0) {
            m_InitialPresentationId.assign(filename.c_str());
            eastl::string relativePath = "./";
            relativePath.append(filename);
            relativePath.append(".");
            relativePath.append("uip");
            RegisterAsset(SPresentationAsset(RegisterStr(filename.c_str()),
                                             RegisterStr(relativePath.c_str())));
            m_AppLoadContext = IAppLoadContext::CreateXMLLoadContext(*this, "");

            retval = true;
        } else if (extension.comparei("uia") == 0) {
            CFileSeekableIOStream inputStream(sourcePath, FileReadFlags());
            if (inputStream.IsOpen()) {
                NVScopedRefCounted<IStringTable> strTable(
                            IStringTable::CreateStringTable(fnd.getAllocator()));
                NVScopedRefCounted<IDOMFactory> domFactory(
                            IDOMFactory::CreateDOMFactory(fnd.getAllocator(), strTable));
                SAppXMLErrorHandler errorHandler(fnd, sourcePath.toUtf8().constData());
                eastl::pair<SNamespacePairNode *, SDOMElement *> readResult =
                        CDOMSerializer::Read(*domFactory, inputStream, &errorHandler);
                if (!readResult.second) {
                    qCCritical(INVALID_PARAMETER, "%s doesn't appear to be valid xml",
                               sourcePath.toUtf8().constData());
                } else {
                    NVScopedRefCounted<IDOMReader> domReader = IDOMReader::CreateDOMReader(
                                fnd.getAllocator(), *readResult.second, strTable, domFactory);
                    if (m_visitor)
                        m_visitor->visit(sourcePath.toUtf8().constData());
                    retval = LoadUIA(*domReader, fnd);
                }
            } else {
                qCCritical(INVALID_PARAMETER, "Unable to open input file %s",
                           sourcePath.toUtf8().constData());
            }
        } else {
            QT3DS_ASSERT(false);
        }
        return retval;
    }

    void EndLoad() override
    {
        if (m_AppLoadContext)
            m_AppLoadContext->EndLoad();
    }

    void RunAllRunnables()
    {
        {
            Mutex::ScopedLock __locker(m_RunnableMutex);
            m_MainThreadRunnables = m_ThreadRunnables;
            m_ThreadRunnables.clear();
        }
        for (QT3DSU32 idx = 0, end = m_MainThreadRunnables.size(); idx < end; ++idx)
            m_MainThreadRunnables[idx]->Run();
        m_MainThreadRunnables.clear();
    }

    bool HasCompletedLoading() override
    {
        RunAllRunnables();
        if (m_AppLoadContext)
            return m_AppLoadContext->HasCompletedLoading();

        return true;
    }

    bool createSuccessful() override
    {
        return m_createSuccessful;
    }

    bool presentationComponentSlide(const QString &elementPath,
                                    Q3DStudio::CPresentation *&presentation,
                                    TElement *&component,
                                    QString &slideName,
                                    int &index)
    {
        presentation = GetPrimaryPresentation();
        slideName = elementPath;
        QString componentName;
        if (elementPath.contains(QLatin1Char(':'))) {
            // presentation : component : slide
            QStringList splits = elementPath.split(QLatin1Char(':'));
            if (splits.size() == 3) {
                presentation = GetPresentationById(qPrintable(splits[0]));
                componentName = splits[1];
                slideName = splits[2];
            } else {
                componentName = splits[0];
                slideName = splits[1];
            }
            // else assume main presentation and component:slide
        }
        component = presentation->GetRoot();
        if (!componentName.isNull() && componentName != component->m_Name)
            component = component->FindChild(CHash::HashString(qPrintable(componentName)));
        if (!component) {
            qCWarning(WARNING) << "Could not find slide: " << elementPath;
            return false;
        }
        ISlideSystem &s = presentation->GetSlideSystem();
        index = s.FindSlide(*component, qPrintable(slideName));
        return true;
    }

    void preloadSlide(const QString &slide) override
    {
        CPresentation *pres = nullptr;
        TElement *component = nullptr;
        QString slideName;
        int index;
        if (presentationComponentSlide(slide, pres, component, slideName, index))
            loadComponentSlideResources(component, pres, index, slideName);
    }

    void unloadSlide(const QString &slide) override
    {
        CPresentation *pres = nullptr;
        TElement *component = nullptr;
        QString slideName;
        int index;
        if (presentationComponentSlide(slide, pres, component, slideName, index))
            unloadComponentSlideResources(component, pres, index, slideName);
    }

    void setDelayedLoading(bool enable)
    {
        m_RuntimeFactory->GetQt3DSRenderContext().GetBufferManager()
                .enableReloadableResources(enable);
    }

    void ComponentSlideEntered(Q3DStudio::CPresentation *presentation,
                               Q3DStudio::TElement *component,
                               const QString &elementPath, int slideIndex,
                               const QString &slideName) override
    {
        loadComponentSlideResources(component, presentation, slideIndex, slideName);
    }

    void ComponentSlideExited(Q3DStudio::CPresentation *presentation,
                              Q3DStudio::TElement *component,
                              const QString &elementPath, int slideIndex,
                              const QString &slideName) override
    {
        unloadComponentSlideResources(component, presentation, slideIndex, slideName);
    }

    // will force loading to end if endLoad hasn't been called yet.  Will fire off loading
    // of resources that need to be uploaded to opengl.  Maintains reference to runtime factory
    IApplication &CreateApplication(Q3DStudio::CInputEngine &inInputEngine,
                                    Q3DStudio::IAudioPlayer *inAudioPlayer,
                                    Q3DStudio::IRuntimeFactory &inFactory) override
    {
        {
            SStackPerfTimer __loadTimer(m_CoreFactory->GetPerfTimer(),
                                        "Application: Initialize Graphics");

            {
                SStackPerfTimer __timer(m_CoreFactory->GetPerfTimer(), "Application: EndLoad");
                EndLoad();
            }
            m_InputEnginePtr = &inInputEngine;
            m_RuntimeFactory = inFactory;

            {
                SStackPerfTimer __timer(m_CoreFactory->GetPerfTimer(),
                                        "Application: Load Context Graphics Initialized");
                if (m_AppLoadContext)
                    m_createSuccessful = m_AppLoadContext->OnGraphicsInitialized(inFactory);
                // Guarantees the end of the multithreaded access to the various components
                m_AppLoadContext = NULL;
                if (!m_createSuccessful)
                    return *this;
            }

            {
                SStackPerfTimer __loadTimer(m_CoreFactory->GetPerfTimer(),
                                            "Application: End Font Preload");
                if (m_CoreFactory->GetRenderContextCore().GetTextRendererCore())
                    m_CoreFactory->GetRenderContextCore()
                            .GetTextRendererCore()
                            ->EndPreloadFonts();
            }

            RunAllRunnables();
            // Moving set application to the end ensures that the application load context is not
            // accessing
            // the lua state in another thread while we are calling set application.  This
            // apparently may cause
            // the call to set application to fail miserably.
            m_RuntimeFactory->SetApplication(this);
            m_RuntimeFactory->GetStringTable().DisableMultithreadedAccess();

            for (QT3DSU32 idx = 0, end = m_OrderedAssets.size(); idx < end; ++idx) {
                if (m_OrderedAssets[idx].second->getType() == AssetValueTypes::Presentation) {
                    SPresentationAsset &theAsset(
                                *m_OrderedAssets[idx].second->getDataPtr<SPresentationAsset>());
                    CPresentation *thePresentation = theAsset.m_Presentation;
                    if (thePresentation) {
                        SStackPerfTimer __loadTimer(m_CoreFactory->GetPerfTimer(),
                                                    "Application: SetActivityZone");
                        thePresentation->SetActivityZone(
                                    &m_ActivityZoneManager->CreateActivityZone(*thePresentation));
                        thePresentation->SetActive(theAsset.m_Active);
                    }
                }
            }

            inInputEngine.SetApplication(this);
        }
        SApplicationSettings finalSettings(/*m_CommandLineSettings, */m_UIAFileSettings);
        if (finalSettings.m_LayerCacheEnabled.hasValue()) {
            inFactory.GetQt3DSRenderContext().GetRenderer().EnableLayerCaching(
                        *finalSettings.m_LayerCacheEnabled);
        }
        if (finalSettings.m_LayerGpuProfilingEnabled.hasValue()) {
            inFactory.GetQt3DSRenderContext().GetRenderer().EnableLayerGpuProfiling(
                        *finalSettings.m_LayerGpuProfilingEnabled);
        }

        m_CoreFactory->GetPerfTimer().OutputTimerData();

        m_AudioPlayer.SetPlayer(inAudioPlayer);

        m_uploadRenderTask.reset(new STextureUploadRenderTask(
                                     m_RuntimeFactory->GetQt3DSRenderContext().GetBufferManager()));
        m_uploadRenderTask->add(m_createSet);
        m_RuntimeFactory->GetQt3DSRenderContext().GetRenderList()
                                                                .AddRenderTask(*m_uploadRenderTask);
        m_createSet.clear();
        return *this;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //	Getters/Setters
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    CRegisteredString RegisterStr(const char8_t *inStr)
    {
        return m_CoreFactory->GetStringTable().RegisterStr(inStr);
    }

    // The directory that contains the executable and the root resource path
    CRegisteredString GetApplicationDirectory() const override
    {
        return const_cast<SApp &>(*this).m_CoreFactory->GetStringTable().RegisterStr(
                    m_ApplicationDir.c_str());
    }
    // Directory that contained the XIF file.
    CRegisteredString GetProjectDirectory() const override
    {
        QT3DS_ASSERT(m_ProjectDir.size());
        return const_cast<SApp &>(*this).m_CoreFactory->GetStringTable().RegisterStr(
                    m_ProjectDir.c_str());
    }

    CRegisteredString GetDllDir() const override
    {
        if (m_DLLDirectory.size()) {
            return const_cast<SApp &>(*this).m_CoreFactory->GetStringTable().RegisterStr(
                        m_DLLDirectory.c_str());
        }
        return CRegisteredString();
    }

    void SetDllDir(const char *inDllDir) override
    {
        m_DLLDirectory.assign(nonNull(inDllDir));
        m_CoreFactory->SetDllDir(inDllDir);
    }

    Q3DStudio::IRuntimeFactory &GetRuntimeFactory() const override { return *m_RuntimeFactory.mPtr; }
    Q3DStudio::IRuntimeFactoryCore &GetRuntimeFactoryCore() override { return *m_CoreFactory; }

    Q3DStudio::CPresentation *GetPrimaryPresentation() override
    {
        return GetPresentationById(m_InitialPresentationId.c_str());
    }

    Q3DStudio::CPresentation *GetPresentationById(const char8_t *inId) override
    {
        if (!isTrivial(inId)) {
            TIdAssetMap::iterator iter
                    = m_AssetMap.find(m_CoreFactory->GetStringTable().RegisterStr(inId));
            if (iter != m_AssetMap.end()
                    && iter->second->getType() == AssetValueTypes::Presentation) {
                return iter->second->getData<SPresentationAsset>().m_Presentation;
            }
        }
        return NULL;
    }

    // Returns a list of all presentations in the application
    // The primary presentation is returned at index 0
    QList<Q3DStudio::CPresentation *> GetPresentationList() override
    {
        QList<Q3DStudio::CPresentation *> list;
        for (TIdAssetMap::iterator iter = m_AssetMap.begin(); iter != m_AssetMap.end(); ++iter) {
            if (iter->second->getType() == AssetValueTypes::Presentation) {
                Q3DStudio::CPresentation *presentation
                        = iter->second->getData<SPresentationAsset>().m_Presentation;
                if (presentation) {
                    if (iter->first == m_InitialPresentationId)
                        list.prepend(presentation);
                    else
                        list.append(presentation);
                }
            }
        }
        return list;
    }

    template <typename TAssetType>
    void RegisterAsset(const TAssetType &inAsset)
    {
        NVScopedRefCounted<SRefCountedAssetValue> theValue(
                    QT3DS_NEW(m_CoreFactory->GetFoundation().getAllocator(),
                              SRefCountedAssetValue(m_CoreFactory->GetFoundation(), inAsset)));
        if (inAsset.m_Id.IsValid())
            m_AssetMap.insert(eastl::make_pair(inAsset.m_Id, theValue));

        m_OrderedAssets.push_back(eastl::make_pair(inAsset.m_Id, theValue));

        if (m_visitor) {
            m_visitor->visit(inAsset.Type(), inAsset.m_Id.c_str(), inAsset.m_Src.c_str(),
                             inAsset.m_Args.c_str());
        }
    }

    THashValue HashString(const char *inStr) override
    {
        if (inStr == NULL)
            inStr = "";
        THashValue retval = CHash::HashString(inStr);
        eastl::pair<THashStrMap::iterator, bool> insertResult
                = m_HashStrMap.insert(eastl::make_pair(retval, CRegisteredString()));
        if (insertResult.second)
            insertResult.first->second = m_CoreFactory->GetStringTable().RegisterStr(inStr);
        return retval;
    }

    const char *ReverseHash(THashValue theValue) override
    {
        THashStrMap::iterator find = m_HashStrMap.find(theValue);
        if (find != m_HashStrMap.end())
            return find->second.c_str();
        return "";
    }

    void SetFrameCount(Q3DStudio::INT32 inFrameCount) override { m_FrameCount = inFrameCount; }

    Q3DStudio::INT32 GetFrameCount() override { return m_FrameCount; }

    void SetTimeMilliSecs(Q3DStudio::INT64 inMilliSecs) override { m_ManualTime = inMilliSecs; }

    Q3DStudio::INT64 GetTimeMilliSecs() override
    {
        return m_ManualTime == 0 ? m_Timer.GetTimeMilliSecs() : m_ManualTime;
    }

    void ResetTime() override
    {
        m_Timer.Reset();
        m_ManualTime = 0;
    }

    void HideFPS(bool flag) override { m_HideFPS = flag; }

    Q3DStudio::CInputEngine &GetInputEngine() override
    {
        QT3DS_ASSERT(m_InputEnginePtr);
        return *m_InputEnginePtr;
    }

    Q3DStudio::IRuntimeMetaData &GetMetaData() override
    {
        if (!m_MetaData) {
            m_MetaData = &IRuntimeMetaData::Create(m_CoreFactory->GetInputStreamFactory());
            if (!m_MetaData) {
                qCCritical(qt3ds::INVALID_OPERATION)
                        << "IRuntimeMetaData::Create: Failed to create meta data";
            }
        }
        return *m_MetaData;
    }

    IActivityZoneManager &GetActivityZoneManager() override { return *m_ActivityZoneManager; }

    IElementAllocator &GetElementAllocator() override { return *m_ElementAllocator; }

    Q3DStudio::UINT32 GetHandleForElement(Q3DStudio::TElement *inElement) override
    {
        return inElement->GetHandle();
    }

    Q3DStudio::TElement *GetElementByHandle(Q3DStudio::UINT32 inHandle) override
    {
        return GetElementAllocator().FindElementByHandle(inHandle);
    }
};

struct SXMLLoader : public IAppLoadContext
{
    SApp &m_App;
    eastl::string m_ScaleMode;
    QT3DSI32 mRefCount;

    SXMLLoader(SApp &inApp, const char8_t *sc)
        : m_App(inApp)
        , m_ScaleMode(nonNull(sc))
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_App.m_CoreFactory->GetFoundation().getAllocator())

    void EndLoad() override {}

    bool HasCompletedLoading() override { return true; }

    bool OnGraphicsInitialized(IRuntimeFactory &inFactory) override
    {
        eastl::vector<SElementAttributeReference> theUIPReferences;
        eastl::string tempString;
        for (QT3DSU32 idx = 0, end = m_App.m_OrderedAssets.size(); idx < end; ++idx) {
            SAssetValue &theAsset = *m_App.m_OrderedAssets[idx].second;
            eastl::string thePathStr;

            CFileTools::CombineBaseAndRelative(m_App.GetProjectDirectory().c_str(),
                                               theAsset.GetSource(), thePathStr);
            switch (theAsset.getType()) {
            case AssetValueTypes::Presentation: {
                QDir::addSearchPath(QStringLiteral("qt3dstudio"),
                                    QFileInfo(QString(thePathStr.c_str()))
                                    .absoluteDir().absolutePath());
                SPresentationAsset &thePresentationAsset
                        = *theAsset.getDataPtr<SPresentationAsset>();
                theUIPReferences.clear();

                if (!m_App.LoadUIP(thePresentationAsset,
                                   toConstDataRef(theUIPReferences.data(),
                                                  (QT3DSU32)theUIPReferences.size()))) {
                    qCCritical(INVALID_OPERATION, "Unable to load presentation %s",
                               thePathStr.c_str());
                }
            } break;
            case AssetValueTypes::Behavior: {
                SBehaviorAsset &theBehaviorAsset = *theAsset.getDataPtr<SBehaviorAsset>();
                Q3DStudio::INT32 scriptId
                        = m_App.m_CoreFactory->GetScriptEngineQml().InitializeApplicationBehavior(
                            theBehaviorAsset.m_Src);
                if (scriptId == 0) {
                    qCCritical(INVALID_OPERATION, "Unable to load application behavior %s",
                               theBehaviorAsset.m_Src.c_str());
                } else {
                    theBehaviorAsset.m_Handle = scriptId;
                    m_App.m_Behaviors.push_back(eastl::make_pair(theBehaviorAsset, false));
                }
            } break;
            case AssetValueTypes::RenderPlugin: {
                SRenderPluginAsset &thePluginAsset = *theAsset.getDataPtr<SRenderPluginAsset>();

                inFactory.GetSceneManager().LoadRenderPlugin(
                            thePluginAsset.m_Id, thePathStr.c_str(), thePluginAsset.m_Args);
            } break;

            case AssetValueTypes::QmlPresentation: {
                SQmlPresentationAsset &asset = *theAsset.getDataPtr<SQmlPresentationAsset>();
                inFactory.GetSceneManager().LoadQmlStreamerPlugin(asset.m_Id);
            } break;
                // SCXML, NoAssetValue do not need processing here
            default:
                break;
            }
        }
        if (m_ScaleMode.empty() == false) {
            const char8_t *initialScaleMode(m_ScaleMode.c_str());
            // Force loading to finish here, just like used to happen.
            if (AreEqual(initialScaleMode, "center")) {
                inFactory.GetQt3DSRenderContext().SetScaleMode(qt3ds::render::ScaleModes::ExactSize);
            } else if (AreEqual(initialScaleMode, "fit")) {
                inFactory.GetQt3DSRenderContext().SetScaleMode(qt3ds::render::ScaleModes::ScaleToFit);
            } else if (AreEqual(initialScaleMode, "fill")) {
                inFactory.GetQt3DSRenderContext().SetScaleMode(qt3ds::render::ScaleModes::ScaleToFill);
            } else {
                qCCritical(INVALID_PARAMETER, "Unrecognized scale mode attribute value: ",
                           initialScaleMode);
            }
        }
        return true;
    }

    virtual void OnFirstRender() {}
};

IAppLoadContext &IAppLoadContext::CreateXMLLoadContext(
        SApp &inApp, const char8_t *inScaleMode)
{
    return *QT3DS_NEW(inApp.m_CoreFactory->GetFoundation().getAllocator(),
                      SXMLLoader)(inApp, inScaleMode);
}

CAppStr::CAppStr(NVAllocatorCallback &alloc, const char8_t *inStr)
    : TBase(inStr, ForwardingAllocator(alloc, "CAppStr"))
{
}

CAppStr::CAppStr(const CAppStr &inOther)
    : TBase(inOther)
{
}

CAppStr::CAppStr()
    : TBase()
{
}

CAppStr &CAppStr::operator=(const CAppStr &inOther)
{
    TBase::operator=(inOther);
    return *this;
}

IApplication &IApplication::CreateApplicationCore(Q3DStudio::IRuntimeFactoryCore &inFactory,
                                                          const char8_t *inApplicationDirectory)
{
    return *QT3DS_NEW(inFactory.GetFoundation().getAllocator(), SApp)(inFactory,
                                                                      inApplicationDirectory);
}

// Checks if the event is one that can cause picking
bool IApplication::isPickingEvent(TEventCommandHash event)
{
    return (event == ON_MOUSEDOWN
            || event == ON_MOUSEUP
            || event == ON_MIDDLEMOUSEDOWN
            || event == ON_MIDDLEMOUSEUP
            || event == ON_RIGHTMOUSEDOWN
            || event == ON_RIGHTMOUSEUP
            || event == ON_MOUSECLICK
            || event == ON_MIDDLEMOUSECLICK
            || event == ON_RIGHTMOUSECLICK
            || event == ON_MOUSEOVER
            || event == ON_MOUSEOUT
            || event == ON_GROUPEDMOUSEOVER
            || event == ON_GROUPEDMOUSEOUT);
}
