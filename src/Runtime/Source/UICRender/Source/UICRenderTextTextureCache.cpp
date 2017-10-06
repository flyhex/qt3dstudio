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
#include "UICRenderTextTextureCache.h"
#include "UICTextRenderer.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/Qt3DSInvasiveLinkedList.h"
#include "foundation/Qt3DSPool.h"

using namespace uic::render;

namespace eastl {
template <>
struct hash<STextRenderInfo>
{
    size_t operator()(const uic::render::STextRenderInfo &inInfo) const
    {
        size_t retval = hash<size_t>()(reinterpret_cast<size_t>(inInfo.m_Text.c_str()));
        retval = retval ^ hash<size_t>()(reinterpret_cast<size_t>(inInfo.m_Font.c_str()));
        retval = retval ^ hash<float>()(inInfo.m_FontSize);
        retval = retval ^ hash<int>()(static_cast<int>(inInfo.m_HorizontalAlignment));
        retval = retval ^ hash<int>()(static_cast<int>(inInfo.m_VerticalAlignment));
        retval = retval ^ hash<float>()(inInfo.m_Leading);
        retval = retval ^ hash<float>()(inInfo.m_Tracking);
        retval = retval ^ hash<bool>()(inInfo.m_EnableAcceleratedFont);
        return retval;
    }
};
}

namespace {
struct STextRenderInfoAndHash
{
    STextRenderInfo m_Info;
    QT3DSF32 m_ScaleFactor;
    size_t m_Hashcode;
    STextRenderInfoAndHash(const STextRenderInfo &inInfo, QT3DSF32 inScaleFactor)
        : m_Info(inInfo)
        , m_ScaleFactor(inScaleFactor)
        , m_Hashcode(eastl::hash<STextRenderInfo>()(inInfo) ^ eastl::hash<float>()(inScaleFactor))
    {
    }
    bool operator==(const STextRenderInfoAndHash &inOther) const
    {
        return m_Info.m_Text == inOther.m_Info.m_Text && m_Info.m_Font == inOther.m_Info.m_Font
            && m_Info.m_FontSize == inOther.m_Info.m_FontSize
            && m_Info.m_HorizontalAlignment == inOther.m_Info.m_HorizontalAlignment
            && m_Info.m_VerticalAlignment == inOther.m_Info.m_VerticalAlignment
            && m_Info.m_Leading == inOther.m_Info.m_Leading
            && m_Info.m_Tracking == inOther.m_Info.m_Tracking
            && m_Info.m_EnableAcceleratedFont == inOther.m_Info.m_EnableAcceleratedFont
            && m_ScaleFactor == inOther.m_ScaleFactor;
    }
};
}

namespace eastl {
template <>
struct hash<STextRenderInfoAndHash>
{
    size_t operator()(const STextRenderInfoAndHash &inInfo) const { return inInfo.m_Hashcode; }
};
};

namespace {

struct STextCacheNode
{
    STextCacheNode *m_PreviousSibling;
    STextCacheNode *m_NextSibling;
    STextRenderInfoAndHash m_RenderInfo;
    TTPathObjectAndTexture m_TextInfo;
    QT3DSU32 m_FrameCount;

    STextCacheNode(const STextRenderInfoAndHash &inRenderInfo,
                   const TTPathObjectAndTexture &inTextInfo)
        : m_PreviousSibling(NULL)
        , m_NextSibling(NULL)
        , m_RenderInfo(inRenderInfo)
        , m_TextInfo(inTextInfo)
        , m_FrameCount(0)
    {
    }
};

typedef nvhash_map<STextRenderInfoAndHash, STextCacheNode *> TTextureInfoHash;

DEFINE_INVASIVE_LIST(TextCacheNode);
IMPLEMENT_INVASIVE_LIST(TextCacheNode, m_PreviousSibling, m_NextSibling);

struct STextTextureCache : public ITextTextureCache
{
    typedef Pool<STextCacheNode, ForwardingAllocator> TPoolType;
    NVFoundationBase &m_Foundation;
    volatile QT3DSI32 mRefCount;
    NVScopedRefCounted<ITextRenderer> m_TextRenderer;
    TTextureInfoHash m_TextureCache;
    TTextCacheNodeList m_LRUList;
    TPoolType m_CacheNodePool;
    QT3DSU32 m_HighWaterMark;
    QT3DSU32 m_FrameCount;
    QT3DSU32 m_TextureTotalBytes;
    NVScopedRefCounted<NVRenderContext> m_RenderContext;
    bool m_CanUsePathRendering; ///< true if we use hardware accelerated font rendering

    STextTextureCache(NVFoundationBase &inFnd, ITextRenderer &inRenderer,
                      NVRenderContext &inRenderContext)
        : m_Foundation(inFnd)
        , mRefCount(0)
        , m_TextRenderer(inRenderer)
        , m_TextureCache(m_Foundation.getAllocator(), "STextTextureCache::m_TextureCache")
        , m_CacheNodePool(ForwardingAllocator(m_Foundation.getAllocator(),
                                              "STextTextureCache::m_CacheNodePool"))
        , m_HighWaterMark(0x100000)
        , m_FrameCount(0)
        , m_TextureTotalBytes(0)
        , m_RenderContext(inRenderContext)
    {
        // hardware accelerate font rendering not ready yet
        m_CanUsePathRendering = (m_RenderContext->IsPathRenderingSupported()
                                 && m_RenderContext->IsProgramPipelineSupported());
    }

    virtual ~STextTextureCache()
    {
        for (TTextCacheNodeList::iterator iter = m_LRUList.begin(), end = m_LRUList.end();
             iter != end; ++iter)
            iter->~STextCacheNode();
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation.getAllocator())

    static inline QT3DSU32 GetNumBytes(NVRenderTexture2D &inTexture)
    {
        STextureDetails theDetails(inTexture.GetTextureDetails());
        return theDetails.m_Width * theDetails.m_Height
            * NVRenderTextureFormats::getSizeofFormat(theDetails.m_Format);
    }

    NVScopedRefCounted<NVRenderTexture2D> InvalidateLastItem()
    {
        NVScopedRefCounted<NVRenderTexture2D> nextTexture;
        if (m_LRUList.empty() == false) {
            STextCacheNode &theEnd = m_LRUList.back();
            if (theEnd.m_FrameCount != m_FrameCount) {
                nextTexture = theEnd.m_TextInfo.second.second;
                STextureDetails theDetails = nextTexture->GetTextureDetails();
                m_TextureTotalBytes -= GetNumBytes(*nextTexture.mPtr);
                m_LRUList.remove(theEnd);
                // copy the key because the next statement will destroy memory
                m_TextureCache.erase(theEnd.m_RenderInfo);
                theEnd.~STextCacheNode();
                m_CacheNodePool.deallocate(&theEnd);
            }
        }
        return nextTexture;
    }

    TTPathObjectAndTexture RenderText(const STextRenderInfo &inText, QT3DSF32 inScaleFactor) override
    {
        STextRenderInfoAndHash theKey(inText, inScaleFactor);
        TTextureInfoHash::iterator theFind(
            m_TextureCache.find(STextRenderInfoAndHash(inText, inScaleFactor)));
        STextCacheNode *retval = NULL;
        if (theFind != m_TextureCache.end()) {
            retval = theFind->second;
            m_LRUList.remove(*retval);
        } else {
            NVScopedRefCounted<NVRenderTexture2D> nextTexture;
            if (m_TextureTotalBytes >= m_HighWaterMark && m_LRUList.empty() == false)
                nextTexture = InvalidateLastItem();

            if (nextTexture.mPtr == NULL)
                nextTexture = m_RenderContext->CreateTexture2D();

            NVScopedRefCounted<NVRenderPathFontItem> nextPathFontItemObject;
            NVScopedRefCounted<NVRenderPathFontSpecification> nextPathFontObject;
            // HW acceleration for fonts not supported
            //if (m_CanUsePathRendering && inText.m_EnableAcceleratedFont) {
            //    nextPathFontItemObject = m_RenderContext->CreatePathFontItem();
            //    nextPathFontObject = m_RenderContext->CreatePathFontSpecification(inText.m_Font);
            //}

            STextRenderInfo theTextInfo(inText);
            theTextInfo.m_FontSize *= inScaleFactor;
            STextTextureDetails theDetails;


            // HW acceleration for fonts not supported
            //if (!m_CanUsePathRendering || !inText.m_EnableAcceleratedFont)
                theDetails = m_TextRenderer->RenderText(theTextInfo, *nextTexture.mPtr);
            //else
            //    theDetails = m_TextRenderer->RenderText(theTextInfo, *nextPathFontItemObject.mPtr,
            //                                            *nextPathFontObject.mPtr);

            if (fabs(inScaleFactor - 1.0f) > .001f) {
                TTPathObjectAndTexture theCanonicalDetails = RenderText(inText, 1.0f);
                theDetails.m_ScaleFactor.x =
                    (QT3DSF32)theDetails.m_TextWidth / theCanonicalDetails.second.first.m_TextWidth;
                theDetails.m_ScaleFactor.y =
                    (QT3DSF32)theDetails.m_TextHeight / theCanonicalDetails.second.first.m_TextHeight;
            }
            theKey = STextRenderInfoAndHash(inText, inScaleFactor);
            retval = m_CacheNodePool.construct(
                theKey, TTPathObjectAndTexture(
                            TPathFontSpecAndPathObject(nextPathFontObject, nextPathFontItemObject),
                            TTextTextureDetailsAndTexture(theDetails, nextTexture)),
                __FILE__, __LINE__);
            TTextureInfoHash::iterator insert =
                m_TextureCache.insert(eastl::make_pair(theKey, retval)).first;
            if (!m_CanUsePathRendering)
                m_TextureTotalBytes += GetNumBytes(*(retval->m_TextInfo.second.second.mPtr));
        }
        retval->m_FrameCount = m_FrameCount;
        m_LRUList.push_front(*retval);
        return retval->m_TextInfo;
    }
    // We may have one more texture in cache than this byte count, but this will be the limiting
    // factor.
    QT3DSU32 GetCacheHighWaterBytes() const override { return m_HighWaterMark; }
    // default cache size is 10 MB.
    void SetCacheHighWaterBytes(QT3DSU32 inByteCount) override { m_HighWaterMark = inByteCount; }

    void BeginFrame() override {}
    void EndFrame() override
    {
        // algorithm is resistant to rollover.
        ++m_FrameCount;
        // Release any texture that put us over the limit.
        // This almost guarantees thrashing if the limit is set too low.  Enable at your
        // own risk at *TEST CAREFULLY*
        /*
        while( m_TextureTotalBytes >= m_HighWaterMark && m_LRUList.empty() == false )
                InvalidateLastItem();
        */
    }
};
}

ITextTextureCache &ITextTextureCache::CreateTextureCache(NVFoundationBase &inFnd,
                                                         ITextRenderer &inTextRenderer,
                                                         NVRenderContext &inRenderContext)
{
    return *QT3DS_NEW(inFnd.getAllocator(), STextTextureCache)(inFnd, inTextRenderer, inRenderContext);
}
