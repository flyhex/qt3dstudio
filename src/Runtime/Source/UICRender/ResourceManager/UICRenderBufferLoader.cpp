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
#include "UICRenderBufferLoader.h"
#include "foundation/Qt3DSInvasiveLinkedList.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSSync.h"
#include "UICRenderInputStreamFactory.h"
#include "UICRenderThreadPool.h"

using namespace uic::render;

namespace {
struct SBufferLoader;
struct SBufferLoadResult : public ILoadedBuffer
{
    NVFoundationBase &m_Foundation;
    CRegisteredString m_Path;
    IBufferLoaderCallback *m_UserData;
    NVDataRef<QT3DSU8> m_Data;
    QT3DSI32 mRefCount;

    SBufferLoadResult(NVFoundationBase &fnd, CRegisteredString p, IBufferLoaderCallback *ud,
                      NVDataRef<QT3DSU8> data)
        : m_Foundation(fnd)
        , m_Path(p)
        , m_UserData(ud)
        , m_Data(data)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation.getAllocator())

    CRegisteredString Path() override { return m_Path; }
    // Data is released when the buffer itself is released.
    NVDataRef<QT3DSU8> Data() override { return m_Data; }
    IBufferLoaderCallback *UserData() override { return m_UserData; }

    static SBufferLoadResult *Allocate(QT3DSU32 inBufferSize, NVFoundationBase &fnd,
                                       CRegisteredString p,
                                       NVScopedRefCounted<IBufferLoaderCallback> ud)
    {
        size_t allocSize = sizeof(SBufferLoadResult) + inBufferSize;
        QT3DSU8 *allocMem =
            (QT3DSU8 *)fnd.getAllocator().allocate(allocSize, "ILoadedBuffer", __FILE__, __LINE__);
        if (allocMem == NULL)
            return NULL;
        QT3DSU8 *bufferStart = allocMem + sizeof(SBufferLoadResult);
        NVDataRef<QT3DSU8> dataBuffer = toDataRef(bufferStart, inBufferSize);
        return new (allocMem) SBufferLoadResult(fnd, p, ud, dataBuffer);
    }
};
struct SLoadedBufferImpl
{
    SBufferLoader &m_Loader;
    QT3DSU64 m_JobId;
    QT3DSU64 m_LoadId;
    CRegisteredString m_Path;
    NVScopedRefCounted<IBufferLoaderCallback> m_UserData;
    bool m_Quiet;
    volatile bool m_Cancel;
    NVScopedRefCounted<SBufferLoadResult> m_Result;
    SLoadedBufferImpl *m_NextBuffer;
    SLoadedBufferImpl *m_PreviousBuffer;

    SLoadedBufferImpl(SBufferLoader &l, CRegisteredString inPath,
                      NVScopedRefCounted<IBufferLoaderCallback> ud, bool inQuiet, QT3DSU64 loadId)
        : m_Loader(l)
        , m_JobId(0)
        , m_LoadId(loadId)
        , m_Path(inPath)
        , m_UserData(ud)
        , m_Quiet(inQuiet)
        , m_Cancel(false)
        , m_NextBuffer(NULL)
        , m_PreviousBuffer(NULL)
    {
    }
};

DEFINE_INVASIVE_LIST(LoadedBufferImpl);
IMPLEMENT_INVASIVE_LIST(LoadedBufferImpl, m_PreviousBuffer, m_NextBuffer);

struct SBufferLoader : public IBufferLoader
{
    NVFoundationBase &m_Foundation;
    NVScopedRefCounted<IInputStreamFactory> m_Factory;
    NVScopedRefCounted<IThreadPool> m_ThreadPool;

    Mutex m_BuffersToLoadMutex;
    TLoadedBufferImplList m_BuffersToLoad;

    Mutex m_BuffersLoadingMutex;
    TLoadedBufferImplList m_BuffersLoading;

    Mutex m_LoadedBuffersMutex;
    TLoadedBufferImplList m_LoadedBuffers;

    Sync m_BufferLoadedEvent;

    QT3DSU64 m_NextBufferId;

    QT3DSI32 mRefCount;

    SBufferLoader(NVFoundationBase &fnd, IInputStreamFactory &fac, IThreadPool &tp)
        : m_Foundation(fnd)
        , m_Factory(fac)
        , m_ThreadPool(tp)
        , m_BuffersToLoadMutex(fnd.getAllocator())
        , m_BuffersLoadingMutex(fnd.getAllocator())
        , m_LoadedBuffersMutex(fnd.getAllocator())
        , m_BufferLoadedEvent(fnd.getAllocator())
        , m_NextBufferId(1)
        , mRefCount(0)
    {
        m_BufferLoadedEvent.reset();
    }

    virtual ~SBufferLoader()
    {
        {
            Mutex::ScopedLock __locker(m_BuffersToLoadMutex);
            for (TLoadedBufferImplList::iterator iter = m_BuffersToLoad.begin(),
                                                 end = m_BuffersToLoad.end();
                 iter != end; ++iter) {
                m_ThreadPool->CancelTask(iter->m_JobId);
            }
        }

        // Pull any remaining buffers out of the thread system.
        while (WillLoadedBuffersBeAvailable()) {
            NextLoadedBuffer();
        }
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    static void InitializeActiveLoadingBuffer(SLoadedBufferImpl &theBuffer)
    {
        Mutex::ScopedLock theLocker(theBuffer.m_Loader.m_BuffersToLoadMutex);
        Mutex::ScopedLock theSecondLocker(theBuffer.m_Loader.m_BuffersLoadingMutex);
        theBuffer.m_Loader.m_BuffersToLoad.remove(theBuffer);
        theBuffer.m_Loader.m_BuffersLoading.push_back(theBuffer);
    }

    static void SetBufferAsLoaded(SLoadedBufferImpl &theBuffer)
    {
        Mutex::ScopedLock theSecondLocker(theBuffer.m_Loader.m_BuffersLoadingMutex);
        Mutex::ScopedLock theLocker(theBuffer.m_Loader.m_LoadedBuffersMutex);
        theBuffer.m_Loader.m_BuffersLoading.remove(theBuffer);
        theBuffer.m_Loader.m_LoadedBuffers.push_back(theBuffer);
        theBuffer.m_Loader.m_BufferLoadedEvent.set();
        theBuffer.m_Loader.m_BufferLoadedEvent.reset();
    }

    static void LoadNextBuffer(void *loader)
    {
        SLoadedBufferImpl &theBuffer = *reinterpret_cast<SLoadedBufferImpl *>(loader);

        InitializeActiveLoadingBuffer(theBuffer);
        NVScopedRefCounted<IRefCountedInputStream> theStream =
            theBuffer.m_Loader.m_Factory->GetStreamForFile(theBuffer.m_Path.c_str(),
                                                           theBuffer.m_Quiet);
        if (theStream && theBuffer.m_Cancel == false) {
            theStream->SetPosition(0, SeekPosition::End);
            QT3DSI64 theFileLen = theStream->GetPosition();
            if (theFileLen > 0 && theFileLen < (QT3DSU32)QT3DS_MAX_U32) {
                QT3DSU32 required = (QT3DSU32)theFileLen;
                theBuffer.m_Result =
                    SBufferLoadResult::Allocate(required, theBuffer.m_Loader.m_Foundation,
                                                theBuffer.m_Path, theBuffer.m_UserData);
                QT3DSU32 amountRead = 0;
                QT3DSU32 total = amountRead;
                if (theBuffer.m_Result && theBuffer.m_Cancel == false) {
                    NVDataRef<QT3DSU8> theDataBuffer(theBuffer.m_Result->m_Data);
                    theStream->SetPosition(0, SeekPosition::Begin);
                    amountRead = theStream->Read(theDataBuffer);
                    total += amountRead;
                    // Ensure we keep trying, not all file systems allow huge reads.
                    while (total < required && amountRead > 0 && theBuffer.m_Cancel == false) {
                        NVDataRef<QT3DSU8> newBuffer(theDataBuffer.mData + total, required - total);
                        amountRead = theStream->Read(newBuffer);
                        total += amountRead;
                    }
                }
                if (theBuffer.m_Cancel || total != required) {
                    theBuffer.m_Result->m_Data = NVDataRef<QT3DSU8>();
                }
            }
        }

        // only callback if the file was successfully loaded.
        if (theBuffer.m_UserData) {
            if (theBuffer.m_Cancel == false && theBuffer.m_Result.mPtr
                && theBuffer.m_Result->m_Data.size()) {
                theBuffer.m_UserData->OnBufferLoaded(*theBuffer.m_Result.mPtr);
            } else {
                if (theBuffer.m_Cancel)
                    theBuffer.m_UserData->OnBufferLoadCancelled(theBuffer.m_Path);
                else
                    theBuffer.m_UserData->OnBufferLoadFailed(theBuffer.m_Path);
            }
        }

        SetBufferAsLoaded(theBuffer);
    }
    static void CancelNextBuffer(void *loader)
    {
        SLoadedBufferImpl &theBuffer = *reinterpret_cast<SLoadedBufferImpl *>(loader);
        theBuffer.m_Cancel = true;
        InitializeActiveLoadingBuffer(theBuffer);

        if (theBuffer.m_UserData)
            theBuffer.m_UserData->OnBufferLoadCancelled(theBuffer.m_Path);

        SetBufferAsLoaded(theBuffer);
    }

    // nonblocking.  Quiet failure is passed to the input stream factory.
    QT3DSU64 QueueForLoading(CRegisteredString inPath,
                                  IBufferLoaderCallback *inUserData = NULL,
                                  bool inQuietFailure = false) override
    {
        SLoadedBufferImpl &theBuffer = *QT3DS_NEW(m_Foundation.getAllocator(), SLoadedBufferImpl)(
            *this, inPath, inUserData, inQuietFailure, m_NextBufferId);
        ++m_NextBufferId;
        {
            Mutex::ScopedLock theLocker(m_BuffersToLoadMutex);
            m_BuffersToLoad.push_back(theBuffer);
        }
        theBuffer.m_JobId = m_ThreadPool->AddTask(&theBuffer, LoadNextBuffer, CancelNextBuffer);
        return theBuffer.m_LoadId;
    }

    void CancelBufferLoad(QT3DSU64 inBufferId) override
    {
        {
            Mutex::ScopedLock theLocker(m_BuffersToLoadMutex);
            SLoadedBufferImpl *theLoadedBuffer = NULL;
            for (TLoadedBufferImplList::iterator iter = m_BuffersToLoad.begin(),
                                                 end = m_BuffersToLoad.end();
                 iter != end && theLoadedBuffer == NULL; ++iter) {
                if (iter->m_LoadId == inBufferId) {
                    theLoadedBuffer = &(*iter);
                    // both cancellation attempts are necessary.  The user will still get
                    // a load result, it will just have no data.
                    theLoadedBuffer->m_Cancel = true;
                    m_ThreadPool->CancelTask(theLoadedBuffer->m_JobId);
                }
            }
        }
    }

    // If we were will to wait, will we ever get another buffer
    bool WillLoadedBuffersBeAvailable() override
    {
        Mutex::ScopedLock theLocker(m_BuffersToLoadMutex);
        Mutex::ScopedLock theSecondLocker(m_BuffersLoadingMutex);
        return AreLoadedBuffersAvailable() || m_BuffersToLoad.empty() == false
            || m_BuffersLoading.empty() == false;
    }
    // Will nextLoadedBuffer block or not?
    bool AreLoadedBuffersAvailable() override
    {
        Mutex::ScopedLock theLocker(m_LoadedBuffersMutex);
        return m_LoadedBuffers.empty() == false;
    }

    // blocking, be careful with this.  No order guarantees here.
    NVScopedRefCounted<ILoadedBuffer> NextLoadedBuffer() override
    {
        while (!AreLoadedBuffersAvailable()) {
            m_BufferLoadedEvent.wait();
        }
        SLoadedBufferImpl *theBuffer;
        {
            Mutex::ScopedLock theLocker(m_LoadedBuffersMutex);
            theBuffer = m_LoadedBuffers.back_ptr();
            m_LoadedBuffers.remove(*theBuffer);
        }
        NVScopedRefCounted<ILoadedBuffer> retval(theBuffer->m_Result);
        if (retval.mPtr == NULL) {
            retval = SBufferLoadResult::Allocate(0, m_Foundation, theBuffer->m_Path,
                                                 theBuffer->m_UserData);
        }
        NVDelete(m_Foundation.getAllocator(), theBuffer);
        return retval;
    }
};
}

IBufferLoader &IBufferLoader::Create(NVFoundationBase &fnd, IInputStreamFactory &inFactory,
                                     IThreadPool &inThreadPool)
{
    return *QT3DS_NEW(fnd.getAllocator(), SBufferLoader)(fnd, inFactory, inThreadPool);
}
