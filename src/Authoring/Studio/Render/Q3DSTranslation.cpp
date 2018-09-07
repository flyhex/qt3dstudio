/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "Q3DSTranslation.h"
#include "StudioApp.h"
#include "Core.h"
#include "ClientDataModelBridge.h"

namespace Q3DStudio
{
/*
    : m_Renderer(inRenderer)
    , m_Context(inContext)
    , m_Doc(*g_StudioApp.GetCore()->GetDoc())
    , m_Reader(m_Doc.GetDocumentReader())
    , m_ObjectDefinitions(
          m_Doc.GetStudioSystem()->GetClientDataModelBridge()->GetObjectDefinitions())
    , m_StudioSystem(*m_Doc.GetStudioSystem())
    , m_FullSystem(*m_Doc.GetStudioSystem()->GetFullSystem())
    , m_AssetGraph(*m_Doc.GetAssetGraph())
    , m_Allocator(inContext.GetRenderContext().GetFoundation())
    , m_TranslatorMap(inContext.GetAllocator(), "STranslation::m_TranslatorMap")
    , m_DirtySet(inContext.GetAllocator(), "STranslation::m_DirtySet")
    , m_Scene(nullptr)
    , m_SignalConnections(inContext.GetAllocator(), "STranslation::m_SignalConnections")
    , m_ComponentSecondsDepth(0)
    , m_KeyRepeat(0)
    , m_EditCameraEnabled(false)
    , m_EditLightEnabled(false)
    , m_Viewport(0, 0)
    , m_EditCameraLayerTranslator(nullptr)
    , m_PixelBuffer(inContext.GetAllocator(), "STranslation::m_PixelBuffer")
    , m_editModeCamerasAndLights(inContext.GetAllocator(),
                                 "STranslation::m_editModeCamerasAndLights")
    , m_GuideAllocator(inContext.GetAllocator(), "STranslation::m_GuideAllocator")
*/
Q3DSTranslation::Q3DSTranslation(IStudioRenderer &inRenderer, Q3DSEngine *engine)
    : m_studioRenderer(inRenderer)
    , m_doc(*g_StudioApp.GetCore()->GetDoc())
    , m_reader(m_doc.GetDocumentReader())
    , m_objectDefinitions(
          m_doc.GetStudioSystem()->GetClientDataModelBridge()->GetObjectDefinitions())
    , m_studioSystem(*m_doc.GetStudioSystem())
    , m_fullSystem(*m_doc.GetStudioSystem()->GetFullSystem())
    , m_assetGraph(*m_doc.GetAssetGraph())
    , m_engine(engine)
{

}

}
