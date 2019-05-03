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
#include "Qt3DSRenderImage.h"
#include "Qt3DSRenderBufferManager.h"
#include "Qt3DSOffscreenRenderManager.h"
#include "Qt3DSOffscreenRenderKey.h"
#include "Qt3DSRenderPlugin.h"
#include "Qt3DSRenderPluginGraphObject.h"

using namespace qt3ds::render;

SImage::SImage()
    : SGraphObject(GraphObjectTypes::Image)
    , m_RenderPlugin(nullptr)
    , m_LastFrameOffscreenRenderer(nullptr)
    , m_Parent(nullptr)
    , m_Scale(1, 1)
    , m_Pivot(0, 0)
    , m_Rotation(0)
    , m_Position(0, 0)
    , m_MappingMode(ImageMappingModes::Normal)
    , m_HorizontalTilingMode(NVRenderTextureCoordOp::ClampToEdge)
    , m_VerticalTilingMode(NVRenderTextureCoordOp::ClampToEdge)
{
    m_Flags.SetActive(true);
    m_Flags.SetDirty(true);
    m_Flags.SetTransformDirty(true);
}

static void HandleOffscreenResult(SImage &theImage, SImageTextureData &newImage,
                                  SOffscreenRenderResult &theResult, bool &replaceTexture,
                                  bool &wasDirty)
{
    newImage.m_Texture = theResult.m_Texture;
    newImage.m_TextureFlags.SetHasTransparency(theResult.m_HasTransparency);
    newImage.m_TextureFlags.SetPreMultiplied(true);
    wasDirty = wasDirty || theResult.m_HasChangedSinceLastFrame;
    theImage.m_LastFrameOffscreenRenderer = theResult.m_Renderer;
    replaceTexture = true;
}

bool SImage::ClearDirty(IBufferManager &inBufferManager, IOffscreenRenderManager &inRenderManager,
                        IRenderPluginManager &inPluginManager, bool forIbl)
{

    bool wasDirty = m_Flags.IsDirty();
    m_Flags.SetDirty(false);
    SImageTextureData newImage;
    bool replaceTexture(false);
    if (m_RenderPlugin && m_RenderPlugin->m_Flags.IsActive()) {
        IRenderPluginInstance *theInstance = inPluginManager.GetOrCreateRenderPluginInstance(
            m_RenderPlugin->m_PluginPath, m_RenderPlugin);
        if (theInstance) {
            inRenderManager.MaybeRegisterOffscreenRenderer(theInstance, *theInstance);
            SOffscreenRenderResult theResult = inRenderManager.GetRenderedItem(theInstance);
            HandleOffscreenResult(*this, newImage, theResult, replaceTexture, wasDirty);
        }
    }

    if (newImage.m_Texture == nullptr) {
        if (m_OffscreenRendererId.IsValid()) {
            SOffscreenRenderResult theResult =
                inRenderManager.GetRenderedItem(m_OffscreenRendererId);
            HandleOffscreenResult(*this, newImage, theResult, replaceTexture, wasDirty);
        }
    }

    if (newImage.m_Texture == nullptr) {
        m_LastFrameOffscreenRenderer = nullptr;
        if (m_ImagePath.IsValid()) {
            if (!m_LoadedTextureData
                    || m_LoadedTextureData->m_path != QString::fromUtf8(m_ImagePath.c_str())) {
                if (m_LoadedTextureData)
                    m_LoadedTextureData->m_callbacks.removeOne(this);
                m_LoadedTextureData = inBufferManager.CreateReloadableImage(m_ImagePath, false,
                                                                            forIbl);
                m_LoadedTextureData->m_callbacks.push_back(this);
            }
            if (m_LoadedTextureData) {
                if (m_LoadedTextureData->m_loaded) {
                    newImage.m_Texture = m_LoadedTextureData->m_Texture;
                    newImage.m_TextureFlags = m_LoadedTextureData->m_TextureFlags;
                    newImage.m_BSDFMipMap = m_LoadedTextureData->m_BSDFMipMap;
                }
                replaceTexture = m_TextureData.m_Texture != newImage.m_Texture;
            }
        }
    }

    if (replaceTexture) {
        wasDirty = true;
        m_TextureData = newImage;
    }

    if (m_Flags.IsTransformDirty()) {
        wasDirty = true;
        CalculateTextureTransform();
    }
    return wasDirty;
}

void SImage::CalculateTextureTransform()
{
    m_Flags.SetTransformDirty(false);

    m_TextureTransform = QT3DSMat44::createIdentity();

    QT3DSMat44 translation(QT3DSMat44::createIdentity());
    QT3DSMat44 rotation(QT3DSMat44::createIdentity());
    QT3DSMat44 scale(QT3DSMat44::createIdentity());

    translation.column3[0] = m_Position.x;
    translation.column3[1] = m_Position.y;
    scale.column0[0] = m_Scale.x;
    scale.column1[1] = m_Scale.y;
    rotation.rotate(m_Rotation, QT3DSVec3(0, 0, 1));

    // Setup the pivot.
    m_TextureTransform.column3[0] = m_Pivot.x;
    m_TextureTransform.column3[1] = m_Pivot.y;
    m_TextureTransform = m_TextureTransform * rotation;
    m_TextureTransform = m_TextureTransform * scale;
    m_TextureTransform = m_TextureTransform * translation;
}
