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
#ifndef UIC_RENDER_CAMERA_H
#define UIC_RENDER_CAMERA_H
#include "UICRenderNode.h"
#include "UICRenderRay.h"

namespace qt3ds {
namespace render {

    struct SCameraGlobalCalculationResult
    {
        bool m_WasDirty;
        bool m_ComputeFrustumSucceeded;
        SCameraGlobalCalculationResult(bool inWasDirty, bool inComputeSucceeded = true)
            : m_WasDirty(inWasDirty)
            , m_ComputeFrustumSucceeded(inComputeSucceeded)
        {
        }
    };

    struct CameraScaleModes
    {
        enum Enum {
            Fit = 0,
            SameSize,
            FitHorizontal,
            FitVertical,
        };
    };

    struct CameraScaleAnchors
    {
        enum Enum {
            Center = 0,
            North,
            NorthEast,
            East,
            SouthEast,
            South,
            SouthWest,
            West,
            NorthWest,
        };
    };

    struct SCuboidRect
    {
        QT3DSF32 m_Left;
        QT3DSF32 m_Top;
        QT3DSF32 m_Right;
        QT3DSF32 m_Bottom;
        SCuboidRect(QT3DSF32 l = 0.0f, QT3DSF32 t = 0.0f, QT3DSF32 r = 0.0f, QT3DSF32 b = 0.0f)
            : m_Left(l)
            , m_Top(t)
            , m_Right(r)
            , m_Bottom(b)
        {
        }
        void Translate(QT3DSVec2 inTranslation)
        {
            m_Left += inTranslation.x;
            m_Right += inTranslation.x;
            m_Top += inTranslation.y;
            m_Bottom += inTranslation.y;
        }
    };

    struct SCamera : public SNode
    {

        // Setting these variables should set dirty on the camera.
        QT3DSF32 m_ClipNear;
        QT3DSF32 m_ClipFar;

        QT3DSF32 m_FOV; // Radians

        QT3DSMat44 m_Projection;
        CameraScaleModes::Enum m_ScaleMode;
        CameraScaleAnchors::Enum m_ScaleAnchor;
        // Record some values from creating the projection matrix
        // to use during mouse picking.
        QT3DSVec2 m_FrustumScale;

        SCamera();

        QT3DSMat33 GetLookAtMatrix(const QT3DSVec3 &inUpDir, const QT3DSVec3 &inDirection) const;
        // Set our position, rotation member variables based on the lookat target
        // Marks this object as dirty.
        // Need to test this when the camera's local transform is null.
        // Assumes parent's local transform is the identity, meaning our local transform is
        // our global transform.
        void LookAt(const QT3DSVec3 &inCameraPos, const QT3DSVec3 &inUpDir, const QT3DSVec3 &inTargetPos);

        SCameraGlobalCalculationResult CalculateGlobalVariables(const NVRenderRectF &inViewport,
                                                                const QT3DSVec2 &inDesignDimensions);
        bool CalculateProjection(const NVRenderRectF &inViewport, const QT3DSVec2 &inDesignDimensions);
        bool ComputeFrustumOrtho(const NVRenderRectF &inViewport, const QT3DSVec2 &inDesignDimensions);
        // Used when rendering the widgets in studio.  This scales the widget when in orthographic
        // mode in order to have
        // constant size on screen regardless.
        // Number is always greater than one
        QT3DSF32 GetOrthographicScaleFactor(const NVRenderRectF &inViewport,
                                         const QT3DSVec2 &inDesignDimensions) const;
        bool ComputeFrustumPerspective(const NVRenderRectF &inViewport,
                                       const QT3DSVec2 &inDesignDimensions);
        // Text may be scaled so that it doesn't appear pixellated when the camera itself is doing
        // the scaling.
        QT3DSF32 GetTextScaleFactor(const NVRenderRectF &inViewport,
                                 const QT3DSVec2 &inDesignDimensions) const;

        void CalculateViewProjectionMatrix(QT3DSMat44 &outMatrix) const;

        // If this is an orthographic camera, the cuboid properties are the distance from the center
        // point
        // to the left, top, right, and bottom edges of the view frustum in world units.
        // If this is a perspective camera, the cuboid properties are the FOV angles
        // (left,top,right,bottom)
        // of the view frustum.

        // Return a normalized rect that describes the area the camera is rendering to.
        // This takes into account the various camera properties (scale mode, scale anchor).
        SCuboidRect GetCameraBounds(const NVRenderRectF &inViewport,
                                    const QT3DSVec2 &inDesignDimensions) const;

        // Setup a camera VP projection for rendering offscreen.
        static void SetupOrthographicCameraForOffscreenRender(NVRenderTexture2D &inTexture,
                                                              QT3DSMat44 &outVP);
        static void SetupOrthographicCameraForOffscreenRender(NVRenderTexture2D &inTexture,
                                                              QT3DSMat44 &outVP, SCamera &outCamera);

        // Unproject a point (x,y) in viewport relative coordinates meaning
        // left, bottom is 0,0 and values are increasing right,up respectively.
        SRay Unproject(const QT3DSVec2 &inLayerRelativeMouseCoords, const NVRenderRectF &inViewport,
                       const QT3DSVec2 &inDesignDimensions) const;

        // Unproject a given coordinate to a 3d position that lies on the same camera
        // plane as inGlobalPos.
        // Expects CalculateGlobalVariables has been called or doesn't need to be.
        QT3DSVec3 UnprojectToPosition(const QT3DSVec3 &inGlobalPos, const SRay &inRay) const;
    };
}
}

#endif
