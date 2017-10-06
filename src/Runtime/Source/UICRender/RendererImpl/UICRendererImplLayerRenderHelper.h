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
#include "UICRender.h"
#include "foundation/Qt3DSVec2.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "UICRenderWindowDimensions.h"
#include "UICRenderCamera.h"
#include "UICRenderContext.h"

namespace uic {
namespace render {

    /**	An independent, testable entity to encapsulate taking at least:
     *  layer, current viewport rect, current scissor rect, presentation design dimensions
     *	and producing a set of rectangles:
     *	layer viewport rect (inside viewport rect and calculated using outer viewport rect info)
     *	layer scissor rect (inside current scissor rect)
     *	layer camera rect (may be the viewport rect)
     *
     *  In the case where we have to render offscreen for this layer then we need to handle produce
     *	a set of texture dimensions and the layer camera rect ends up being same size but with no
     *offsets.
     *
     *  This object should handle part of any translation from screenspace to global space.
     *	I am using language level access control on this object because it needs specific
     *	interface design that will enable future modifications.
     */
    struct SLayerRenderHelper
    {
    private:
        NVRenderRectF m_PresentationViewport;
        NVRenderRectF m_PresentationScissor;
        QT3DSVec2 m_PresentationDesignDimensions;
        SLayer *m_Layer;
        SCamera *m_Camera;
        bool m_Offscreen;

        NVRenderRectF m_Viewport;
        NVRenderRectF m_Scissor;

        ScaleModes::Enum m_ScaleMode;
        QT3DSVec2 m_ScaleFactor;

    public:
        SLayerRenderHelper();

        SLayerRenderHelper(const NVRenderRectF &inPresentationViewport,
                           const NVRenderRectF &inPresentationScissor,
                           const QT3DSVec2 &inPresentationDesignDimensions, SLayer &inLayer,
                           bool inOffscreen, uic::render::ScaleModes::Enum inScaleMode,
                           qt3ds::QT3DSVec2 inScaleFactor);

        NVRenderRectF GetPresentationViewport() const { return m_PresentationViewport; }
        NVRenderRectF GetPresentationScissor() const { return m_PresentationScissor; }
        QT3DSVec2 GetPresentationDesignDimensions() const { return m_PresentationDesignDimensions; }
        SLayer *GetLayer() const { return m_Layer; }
        SCamera *GetCamera() const { return m_Camera; }
        bool IsOffscreen() const { return m_Offscreen; }

        // Does not differ whether offscreen or not, simply states how this layer maps to the
        // presentation
        NVRenderRectF GetLayerToPresentationViewport() const { return m_Viewport; }
        // Does not differ whether offscreen or not, scissor rect of how this layer maps to
        // presentation.
        NVRenderRectF GetLayerToPresentationScissorRect() const { return m_Scissor; }

        SWindowDimensions GetTextureDimensions() const;

        SCameraGlobalCalculationResult SetupCameraForRender(SCamera &inCamera);

        Option<QT3DSVec2> GetLayerMouseCoords(const QT3DSVec2 &inMouseCoords,
                                           const QT3DSVec2 &inWindowDimensions,
                                           bool inForceIntersect) const;

        Option<SRay> GetPickRay(const QT3DSVec2 &inMouseCoords, const QT3DSVec2 &inWindowDimensions,
                                bool inForceIntersect) const;

        // Checks the various viewports and determines if the layer is visible or not.
        bool IsLayerVisible() const;

    private:
        // Viewport used when actually rendering.  In the case where this is an offscreen item then
        // it may be
        // different than the layer to presentation viewport.
        NVRenderRectF GetLayerRenderViewport() const;
    };
}
}
