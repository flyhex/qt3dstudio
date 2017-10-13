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
#include "UICRenderSubpresentation.h"
#include "UICRenderRenderList.h"
#ifdef _WIN32
#pragma warning(disable : 4355) // this used in initializer list.  I have never seen this result in
                                // a physical error
#endif
namespace qt3ds {
namespace render {

    SUICRenderPickResult CSubPresentationPickQuery::Pick(const QT3DSVec2 &inMouseCoords,
                                                         const QT3DSVec2 &inViewportDimensions,
                                                         bool inPickEverything)
    {
        return m_Renderer.DoGraphQueryPick(inMouseCoords, inViewportDimensions, inPickEverything);
    }

    CSubPresentationRenderer::CSubPresentationRenderer(IUICRenderContext &inRenderContext,
                                                       SPresentation &inPresentation)
        : m_RenderContext(inRenderContext)
        , m_Presentation(inPresentation)
        , mRefCount(0)
        , m_PickQuery(*this)
        , m_OffscreenRendererType(inRenderContext.GetStringTable().RegisterStr(GetRendererName()))
    {
    }

    SOffscreenRendererEnvironment
        CSubPresentationRenderer::GetDesiredEnvironment(QT3DSVec2 /*inPresScale*/)
    {
        // If we aren't using a clear color, then we are expected to blend with the background
        bool hasTransparency = m_Presentation.m_Scene->m_UseClearColor ? false : true;
        NVRenderTextureFormats::Enum format =
            hasTransparency ? NVRenderTextureFormats::RGBA8 : NVRenderTextureFormats::RGB8;
        return SOffscreenRendererEnvironment((QT3DSU32)(m_Presentation.m_PresentationDimensions.x),
                                             (QT3DSU32)(m_Presentation.m_PresentationDimensions.y),
                                             format, OffscreenRendererDepthValues::Depth16, false,
                                             AAModeValues::NoAA);
    }

    SOffscreenRenderFlags
    CSubPresentationRenderer::NeedsRender(const SOffscreenRendererEnvironment & /*inEnvironment*/,
                                          QT3DSVec2 /*inPresScale*/)
    {
        bool hasTransparency = m_Presentation.m_Scene->m_UseClearColor ? false : true;
        NVRenderRect theViewportSize(m_RenderContext.GetRenderList().GetViewport());
        bool wasDirty = m_Presentation.m_Scene->PrepareForRender(
            QT3DSVec2((QT3DSF32)theViewportSize.m_Width, (QT3DSF32)theViewportSize.m_Height),
            m_RenderContext);
        return SOffscreenRenderFlags(hasTransparency, wasDirty);
    }

    // Returns true if the rendered result image has transparency, or false
    // if it should be treated as a completely opaque image.
    void CSubPresentationRenderer::Render(const SOffscreenRendererEnvironment &inEnvironment,
                                          NVRenderContext &inRenderContext, QT3DSVec2,
                                          SScene::RenderClearCommand inClearColorBuffer)
    {
        SSubPresentationHelper theHelper(
            m_RenderContext,
            SWindowDimensions((QT3DSU32)inEnvironment.m_Width, (QT3DSU32)inEnvironment.m_Height));
        NVRenderRect theViewportSize(inRenderContext.GetViewport());
        m_Presentation.m_Scene->Render(
            QT3DSVec2((QT3DSF32)theViewportSize.m_Width, (QT3DSF32)theViewportSize.m_Height),
            m_RenderContext, inClearColorBuffer);
        m_LastRenderedEnvironment = inEnvironment;
    }

    // You know the viewport dimensions because
    SUICRenderPickResult CSubPresentationRenderer::DoGraphQueryPick(
        const QT3DSVec2 &inMouseCoords, const QT3DSVec2 &inViewportDimensions, bool inPickEverything)
    {
        SUICRenderPickResult thePickResult;

        if (m_Presentation.m_Scene && m_Presentation.m_Scene->m_FirstChild) {
            thePickResult = m_RenderContext.GetRenderer().Pick(
                *m_Presentation.m_Scene->m_FirstChild, inViewportDimensions,
                QT3DSVec2(inMouseCoords.x, inMouseCoords.y), true, inPickEverything);
        }
        return thePickResult;
    }
}
}
