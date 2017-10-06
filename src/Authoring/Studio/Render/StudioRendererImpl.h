/****************************************************************************
**
** Copyright (C) 2006 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
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
#ifndef UIC_STUDIO_RENDERER_IMPL_H
#define UIC_STUDIO_RENDERER_IMPL_H
#pragma once
#include "IStudioRenderer.h"
#include "WGLRenderContext.h"
#include "UICDMDataTypes.h"
#include "UICDMSignals.h"
#include "UICRenderContext.h"
#include "StudioApp.h"
#include "Doc.h"
#include "UICDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "IDocumentReader.h"
#include "UICFileTools.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/Qt3DSVec4.h"
#include "DispatchListeners.h"
#include "Dispatch.h"
#include "Core.h"
#include "foundation/Qt3DSInvasiveSet.h"
#include "UICRenderer.h"
#include "UICRenderScene.h"
#include "UICRenderNode.h"
#include "UICRenderLayer.h"
#include "UICRenderModel.h"
#include "UICRenderDefaultMaterial.h"
#include "UICRenderLight.h"
#include "UICRenderCamera.h"
#include "UICRenderImage.h"
#include "UICRenderPresentation.h"
#include "StudioProjectSettings.h"
#include "UICRenderUIPSharedTranslation.h"
#include "UICRenderBufferManager.h"
#include "StudioFullSystem.h"
#include "UICDMSignals.h"
#include "CoreConst.h"
#include "IDocumentEditor.h"
#include "foundation/Qt3DSPlane.h"
#include "foundation/Qt3DSQuat.h"
#include "UICTextRenderer.h"
#include "foundation/Qt3DSOption.h"
#include "foundation/Qt3DSMathUtils.h"
#include "UICRenderEffect.h"
#include "UICRenderPath.h"
#include "UICRenderPathSubPath.h"

namespace uic {
namespace studio {
    using Q3DStudio::IDocumentReader;
    using Q3DStudio::CUpdateableDocumentEditor;
    using Q3DStudio::TIdentifier;
    using Q3DStudio::IStudioRenderer;
    using qt3ds::foundation::NVScopedRefCounted;
    using qt3ds::QT3DSVec3;
    using qt3ds::QT3DSQuat;
    using qt3ds::QT3DSF32;
    using qt3ds::QT3DSMat44;
    using qt3ds::QT3DSMat33;
    using qt3ds::QT3DSI32;
    using qt3ds::QT3DSVec2;
    using qt3ds::QT3DSVec4;
    using qt3ds::NVMax;
    using qt3ds::NVMin;
    using qt3ds::QT3DSU32;
    using qt3ds::foundation::Empty;
    using qt3ds::foundation::InvasiveSet;
    using qt3ds::foundation::nvhash_map;
    using qt3ds::foundation::nvvector;
    using qt3ds::foundation::rotationArc;
    using qt3ds::foundation::Option;
    using qt3ds::foundation::Empty;
    using uic::render::IUICRenderContext;
    using uic::render::SScene;
    using uic::render::SLayer;
    using uic::render::SNode;
    using uic::render::SGraphObject;
    using uic::render::SLight;
    using uic::render::SCamera;
    using uic::render::SDefaultMaterial;
    using uic::render::SImage;
    using uic::render::SModel;
    using uic::render::SText;
    using uic::render::GraphObjectTypes;
    using uic::render::SRay;
    using uic::render::ITextRenderer;
    using uic::render::SEffect;
    using uic::render::IEffectSystem;
    using uic::render::SDynamicObject;
    using uic::render::SCustomMaterial;
    using uic::render::IDynamicObjectSystem;
    using uic::render::ICustomMaterialSystem;
    using uic::render::IBufferManager;
    using uic::render::IPathManager;
    using uic::render::SPath;
    using uic::render::SPathSubPath;
    using uic::render::SReferencedMaterial;
    using qt3ds::render::CRegisteredString;
    using qt3ds::render::IStringTable;
    using UICDM::SFloat3;
    using UICDM::SLong4;
    using UICDM::SComposerObjectDefinitions;
    using UICDM::CUICDMInstanceHandle;
    using UICDM::CUICDMPropertyHandle;
}
}
#endif
