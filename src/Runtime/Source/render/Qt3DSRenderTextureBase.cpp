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

#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "EASTL/vector.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderSampler.h"
#include "render/Qt3DSRenderTextureBase.h"
#include "foundation/Qt3DSDataRef.h"

namespace qt3ds {
namespace render {

    NVRenderTextureBase::NVRenderTextureBase(NVRenderContextImpl &context, NVFoundationBase &fnd,
                                             NVRenderTextureTargetType::Enum texTarget)
        : m_Context(context)
        , m_Foundation(fnd)
        , mRefCount(0)
        , m_Backend(context.GetBackend())
        , m_TextureHandle(NULL)
        , m_TextureUnit(QT3DS_MAX_U32)
        , m_SamplerParamsDirty(true)
        , m_TexStateDirty(false)
        , m_SampleCount(1)
        , m_Format(NVRenderTextureFormats::Unknown)
        , m_TexTarget(texTarget)
        , m_BaseLevel(0)
        , m_MaxLevel(1000)
        , m_MaxMipLevel(0)
        , m_Immutable(false)
    {
        m_TextureHandle = m_Backend->CreateTexture();
        m_Sampler = QT3DS_NEW(m_Context.GetFoundation().getAllocator(),
                           NVRenderTextureSampler)(context, context.GetFoundation());
    }

    NVRenderTextureBase::~NVRenderTextureBase()
    {
        if (m_Sampler)
            QT3DS_FREE(m_Context.GetFoundation().getAllocator(), m_Sampler);
        if (m_TextureHandle)
            m_Backend->ReleaseTexture(m_TextureHandle);
    }

    void NVRenderTextureBase::SetBaseLevel(QT3DSI32 value)
    {
        if (m_BaseLevel != value) {
            m_BaseLevel = value;
            m_TexStateDirty = true;
        }
    }

    void NVRenderTextureBase::SetMaxLevel(QT3DSI32 value)
    {
        if (m_MaxLevel != value) {
            m_MaxLevel = value;
            m_TexStateDirty = true;
        }
    }

    void NVRenderTextureBase::SetMinFilter(NVRenderTextureMinifyingOp::Enum value)
    {
        if (m_Sampler->m_MinFilter != value) {
            m_Sampler->m_MinFilter = value;
            m_SamplerParamsDirty = true;
        }
    }

    void NVRenderTextureBase::SetMagFilter(NVRenderTextureMagnifyingOp::Enum value)
    {
        if (m_Sampler->m_MagFilter != value) {
            m_Sampler->m_MagFilter = value;
            m_SamplerParamsDirty = true;
        }
    }

    void NVRenderTextureBase::SetTextureWrapS(NVRenderTextureCoordOp::Enum value)
    {
        if (m_Sampler->m_WrapS != value) {
            m_Sampler->m_WrapS = value;
            m_SamplerParamsDirty = true;
        }
    }

    void NVRenderTextureBase::SetTextureWrapT(NVRenderTextureCoordOp::Enum value)
    {
        if (m_Sampler->m_WrapT != value) {
            m_Sampler->m_WrapT = value;
            m_SamplerParamsDirty = true;
        }
    }

    void NVRenderTextureBase::SetTextureCompareMode(NVRenderTextureCompareMode::Enum value)
    {
        if (m_Sampler->m_CompareMode != value) {
            m_Sampler->m_CompareMode = value;
            m_SamplerParamsDirty = true;
        }
    }

    void NVRenderTextureBase::SetTextureCompareFunc(NVRenderTextureCompareOp::Enum value)
    {
        if (m_Sampler->m_CompareOp != value) {
            m_Sampler->m_CompareOp = value;
            m_SamplerParamsDirty = true;
        }
    }

    static GLfloat getMaxAnisotropy()
    {
        static GLfloat val = (GLfloat)qEnvironmentVariableIntValue("QT3DS_ANISOTROPIC_VALUE");
        return val > 0 ? val : 1.0f;
    }

    void NVRenderTextureBase::applyTexParams()
    {
        if (m_SamplerParamsDirty) {
            m_Backend->UpdateSampler(m_Sampler->GetSamplerHandle(), m_TexTarget,
                                     m_Sampler->m_MinFilter, m_Sampler->m_MagFilter,
                                     m_Sampler->m_WrapS, m_Sampler->m_WrapT, m_Sampler->m_WrapR,
                                     m_Sampler->m_MinLod, m_Sampler->m_MaxLod, m_Sampler->m_LodBias,
                                     m_Sampler->m_CompareMode, m_Sampler->m_CompareOp,
                                     getMaxAnisotropy());

            m_SamplerParamsDirty = false;
        }

        if (m_TexStateDirty) {
            m_Backend->UpdateTextureObject(m_TextureHandle, m_TexTarget, m_BaseLevel, m_MaxLevel);
            m_TexStateDirty = false;
        }
    }

    void NVRenderTextureBase::applyTexSwizzle()
    {
        NVRenderTextureSwizzleMode::Enum theSwizzleMode =
            m_Backend->GetTextureSwizzleMode(m_Format);
        if (theSwizzleMode != m_Sampler->m_SwizzleMode) {
            m_Sampler->m_SwizzleMode = theSwizzleMode;
            m_Backend->UpdateTextureSwizzle(m_TextureHandle, m_TexTarget, theSwizzleMode);
        }
    }
}
}
