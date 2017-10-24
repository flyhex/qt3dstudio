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
#pragma once
#ifndef UIC_RENDERER_IMPL_LAYER_RENDER_PREPARATION_DATA_H
#define UIC_RENDERER_IMPL_LAYER_RENDER_PREPARATION_DATA_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSFlags.h"
#include "Qt3DSRendererImplLayerRenderHelper.h"
#include "Qt3DSRenderShaderCache.h"
#include "Qt3DSRenderableObjects.h"
#include "Qt3DSRenderClippingFrustum.h"
#include "Qt3DSRenderResourceTexture2D.h"
#include "Qt3DSOffscreenRenderManager.h"
#include "Qt3DSRenderProfiler.h"
#include "Qt3DSRenderShadowMap.h"
#include "foundation/Qt3DSPool.h"
#include "Qt3DSRenderableObjects.h"

namespace qt3ds {
namespace render {
    struct SLayerRenderData;
    class CUICRendererImpl;
    struct SRenderableObject;

    struct LayerRenderPreparationResultFlagValues
    {
        enum Enum {
            // Was the data in this layer dirty (meaning re-render to texture, possibly)
            WasLayerDataDirty = 1,
            // Was the data in this layer dirty *or* this layer *or* any effect dirty.
            WasDirty = 1 << 1,
            // An effect or flag or rotation on the layer dictates this object should
            // render to the texture.
            ShouldRenderToTexture = 1 << 2,
            // Some effects require depth texturing, this should be set on the effect
            // instance.
            RequiresDepthTexture = 1 << 3,

            // Should create independent viewport
            // If we aren't rendering to texture we still may have width/height manipulations
            // that require our own viewport.
            ShouldCreateIndependentViewport = 1 << 4,

            // SSAO should be done in a separate pass
            // Note that having an AO pass necessitates a DepthTexture so this flag should
            // never be set without the RequiresDepthTexture flag as well.
            RequiresSsaoPass = 1 << 5,

            // if some light cause shadow
            // we need a separate per light shadow map pass
            RequiresShadowMapPass = 1 << 6,

            // Currently we use a stencil-cover algorithm to render bezier curves.
            RequiresStencilBuffer = 1 << 7
        };
    };

    struct SLayerRenderPreparationResultFlags
        : public NVFlags<LayerRenderPreparationResultFlagValues::Enum, QT3DSU32>
    {
        bool WasLayerDataDirty() const
        {
            return this->operator&(LayerRenderPreparationResultFlagValues::WasLayerDataDirty);
        }
        void SetLayerDataDirty(bool inValue)
        {
            clearOrSet(inValue, LayerRenderPreparationResultFlagValues::WasLayerDataDirty);
        }

        bool WasDirty() const
        {
            return this->operator&(LayerRenderPreparationResultFlagValues::WasDirty);
        }
        void SetWasDirty(bool inValue)
        {
            clearOrSet(inValue, LayerRenderPreparationResultFlagValues::WasDirty);
        }

        bool ShouldRenderToTexture() const
        {
            return this->operator&(LayerRenderPreparationResultFlagValues::ShouldRenderToTexture);
        }
        void SetShouldRenderToTexture(bool inValue)
        {
            clearOrSet(inValue, LayerRenderPreparationResultFlagValues::ShouldRenderToTexture);
        }

        bool RequiresDepthTexture() const
        {
            return this->operator&(LayerRenderPreparationResultFlagValues::RequiresDepthTexture);
        }
        void SetRequiresDepthTexture(bool inValue)
        {
            clearOrSet(inValue, LayerRenderPreparationResultFlagValues::RequiresDepthTexture);
        }

        bool ShouldCreateIndependentViewport() const
        {
            return this->operator&(
                LayerRenderPreparationResultFlagValues::ShouldCreateIndependentViewport);
        }
        void SetShouldCreateIndependentViewport(bool inValue)
        {
            clearOrSet(inValue,
                       LayerRenderPreparationResultFlagValues::ShouldCreateIndependentViewport);
        }

        bool RequiresSsaoPass() const
        {
            return this->operator&(LayerRenderPreparationResultFlagValues::RequiresSsaoPass);
        }
        void SetRequiresSsaoPass(bool inValue)
        {
            clearOrSet(inValue, LayerRenderPreparationResultFlagValues::RequiresSsaoPass);
        }

        bool RequiresShadowMapPass() const
        {
            return this->operator&(LayerRenderPreparationResultFlagValues::RequiresShadowMapPass);
        }
        void SetRequiresShadowMapPass(bool inValue)
        {
            clearOrSet(inValue, LayerRenderPreparationResultFlagValues::RequiresShadowMapPass);
        }

        bool RequiresStencilBuffer() const
        {
            return this->operator&(LayerRenderPreparationResultFlagValues::RequiresStencilBuffer);
        }
        void SetRequiresStencilBuffer(bool inValue)
        {
            clearOrSet(inValue, LayerRenderPreparationResultFlagValues::RequiresStencilBuffer);
        }
    };

    struct SLayerRenderPreparationResult : public SLayerRenderHelper
    {
        SEffect *m_LastEffect;
        SLayerRenderPreparationResultFlags m_Flags;
        QT3DSU32 m_MaxAAPassIndex;
        SLayerRenderPreparationResult()
            : m_LastEffect(NULL)
            , m_MaxAAPassIndex(0)
        {
        }
        SLayerRenderPreparationResult(const SLayerRenderHelper &inHelper)
            : SLayerRenderHelper(inHelper)
            , m_LastEffect(NULL)
            , m_MaxAAPassIndex(0)
        {
        }
    };

    struct SRenderableNodeEntry
    {
        SNode *m_Node;
        TNodeLightEntryList m_Lights;
        SRenderableNodeEntry()
            : m_Node(NULL)
        {
        }
        SRenderableNodeEntry(SNode &inNode)
            : m_Node(&inNode)
        {
        }
    };

    struct SScopedLightsListScope
    {
        nvvector<SLight *> &m_LightsList;
        nvvector<QT3DSVec3> &m_LightDirList;
        QT3DSU32 m_ListOriginalSize;
        SScopedLightsListScope(nvvector<SLight *> &inLights, nvvector<QT3DSVec3> &inDestLightDirList,
                               nvvector<QT3DSVec3> &inSrcLightDirList,
                               TNodeLightEntryList &inScopedLights)
            : m_LightsList(inLights)
            , m_LightDirList(inDestLightDirList)
            , m_ListOriginalSize(m_LightsList.size())
        {
            for (TNodeLightEntryList::iterator iter = inScopedLights.begin(),
                                               end = inScopedLights.end();
                 iter != end; ++iter) {
                m_LightsList.push_back(iter->m_Light);
                m_LightDirList.push_back(inSrcLightDirList[iter->m_LightIndex]);
            }
        }
        ~SScopedLightsListScope()
        {
            m_LightsList.resize(m_ListOriginalSize);
            m_LightDirList.resize(m_ListOriginalSize);
        }
    };

    struct SDefaultMaterialPreparationResult
    {
        SRenderableImage *m_FirstImage;
        QT3DSF32 m_Opacity;
        SRenderableObjectFlags m_RenderableFlags;
        SShaderDefaultMaterialKey m_MaterialKey;
        bool m_Dirty;

        SDefaultMaterialPreparationResult(SShaderDefaultMaterialKey inMaterialKey);
    };

    // Data used strictly in the render preparation step.
    struct SLayerRenderPreparationData
    {
        typedef void (*TRenderRenderableFunction)(SLayerRenderData &inData,
                                                  SRenderableObject &inObject,
                                                  const QT3DSVec2 &inCameraProps,
                                                  TShaderFeatureSet inShaderFeatures,
                                                  QT3DSU32 lightIndex, const SCamera &inCamera);
        typedef nvhash_map<SLight *, SNode *> TLightToNodeMap;
        typedef Pool<SNodeLightEntry, ForwardingAllocator> TNodeLightEntryPoolType;

        enum Enum {
            MAX_AA_LEVELS = 8,
            MAX_TEMPORAL_AA_LEVELS = 2,
        };

        SLayer &m_Layer;
        CUICRendererImpl &m_Renderer;
        NVAllocatorCallback &m_Allocator;
        // List of nodes we can render, not all may be active.  Found by doing a depth-first
        // search through m_FirstChild if length is zero.

        TNodeLightEntryPoolType m_RenderableNodeLightEntryPool;
        nvvector<SRenderableNodeEntry> m_RenderableNodes;
        TLightToNodeMap m_LightToNodeMap; // map of lights to nodes to cache if we have looked up a
                                          // given scoped light yet.
        // Built at the same time as the renderable nodes map.
        // these are processed so they are available when the shaders for the models
        // are being generated.
        nvvector<SNode *> m_CamerasAndLights;

        // Results of prepare for render.
        SCamera *m_Camera;
        nvvector<SLight *> m_Lights; // Only contains lights that are global.
        TRenderableObjectList m_OpaqueObjects;
        TRenderableObjectList m_TransparentObjects;
        // Sorted lists of the rendered objects.  There may be other transforms applied so
        // it is simplest to duplicate the lists.
        TRenderableObjectList m_RenderedOpaqueObjects;
        TRenderableObjectList m_RenderedTransparentObjects;
        QT3DSMat44 m_ViewProjection;
        SClippingFrustum m_ClippingFrustum;
        Option<SLayerRenderPreparationResult> m_LayerPrepResult;
        // Widgets drawn at particular times during the rendering process
        nvvector<IRenderWidget *> m_IRenderWidgets;
        Option<QT3DSVec3> m_CameraDirection;
        // Scoped lights need a level of indirection into a light direction list.  The source light
        // directions list is as long as there are lights on the layer.  It holds invalid
        // information for
        // any lights that are not both active and scoped; but the relative position for a given
        // light
        // in this list is completely constant and immutable; this relative position is saved on a
        // structure
        // and used when looking up the light direction for a given light.
        nvvector<QT3DSVec3> m_SourceLightDirections;
        nvvector<QT3DSVec3> m_LightDirections;
        TModelContextPtrList m_ModelContexts;
        NVScopedRefCounted<IOffscreenRenderer> m_LastFrameOffscreenRenderer;

        eastl::vector<SShaderPreprocessorFeature> m_Features;
        CRegisteredString m_CGLightingFeatureName;
        bool m_FeaturesDirty;
        size_t m_FeatureSetHash;
        bool m_TooManyLightsError;

        // shadow mapps
        NVScopedRefCounted<UICShadowMap> m_ShadowMapManager;

        SLayerRenderPreparationData(SLayer &inLayer, CUICRendererImpl &inRenderer);
        virtual ~SLayerRenderPreparationData();
        bool GetOffscreenRenderer();
        bool GetShadowMapManager();
        bool NeedsWidgetTexture() const;

        SShaderDefaultMaterialKey GenerateLightingKey(DefaultMaterialLighting::Enum inLightingType);

        void PrepareImageForRender(SImage &inImage, ImageMapTypes::Enum inMapType,
                                   SRenderableImage *&ioFirstImage, SRenderableImage *&ioNextImage,
                                   SRenderableObjectFlags &ioFlags,
                                   SShaderDefaultMaterialKey &ioGeneratedShaderKey,
                                   QT3DSU32 inImageIndex);

        SDefaultMaterialPreparationResult
        PrepareDefaultMaterialForRender(SDefaultMaterial &inMaterial,
                                        SRenderableObjectFlags &inExistingFlags, QT3DSF32 inOpacity,
                                        bool inClearMaterialFlags);

        SDefaultMaterialPreparationResult
        PrepareCustomMaterialForRender(SCustomMaterial &inMaterial,
                                       SRenderableObjectFlags &inExistingFlags, QT3DSF32 inOpacity);

        bool PrepareModelForRender(SModel &inModel, const QT3DSMat44 &inViewProjection,
                                   const Option<SClippingFrustum> &inClipFrustum,
                                   TNodeLightEntryList &inScopedLights);

        bool PrepareTextForRender(SText &inText, const QT3DSMat44 &inViewProjection,
                                  QT3DSF32 inTextScaleFactor,
                                  SLayerRenderPreparationResultFlags &ioFlags);
        bool PreparePathForRender(SPath &inPath, const QT3DSMat44 &inViewProjection,
                                  const Option<SClippingFrustum> &inClipFrustum,
                                  SLayerRenderPreparationResultFlags &ioFlags);
        // Helper function used during PRepareForRender and PrepareAndRender
        bool PrepareRenderablesForRender(const QT3DSMat44 &inViewProjection,
                                         const Option<SClippingFrustum> &inClipFrustum,
                                         QT3DSF32 inTextScaleFactor,
                                         SLayerRenderPreparationResultFlags &ioFlags);

        // returns true if this object will render something different than it rendered the last
        // time.
        virtual void PrepareForRender(const SWindowDimensions &inViewportDimensions);
        bool CheckLightProbeDirty(SImage &inLightProbe);
        void AddRenderWidget(IRenderWidget &inWidget);
        void SetShaderFeature(const char *inName, bool inValue);
        void SetShaderFeature(CRegisteredString inName, bool inValue);
        NVConstDataRef<SShaderPreprocessorFeature> GetShaderFeatureSet();
        size_t GetShaderFeatureSetHash();
        // The graph object is not const because this traversal updates dirty state on the objects.
        eastl::pair<bool, SGraphObject *> ResolveReferenceMaterial(SGraphObject *inMaterial);

        QT3DSVec3 GetCameraDirection();
        // Per-frame cache of renderable objects post-sort.
        NVDataRef<SRenderableObject *> GetOpaqueRenderableObjects();
        // If layer depth test is false, this may also contain opaque objects.
        NVDataRef<SRenderableObject *> GetTransparentRenderableObjects();

        virtual void ResetForFrame();

        // The render list and gl context are setup for what the embedded item will
        // need.
        virtual SOffscreenRendererEnvironment CreateOffscreenRenderEnvironment() = 0;

        virtual IRenderTask &CreateRenderToTextureRunnable() = 0;
    };
}
}
#endif
