/****************************************************************************
**
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

#ifndef QT3DS_RENDER_BACKEND_GLES2_H
#define QT3DS_RENDER_BACKEND_GLES2_H

#include "foundation/Qt3DSAtomic.h"
#include "render/backends/gl/Qt3DSRenderBackendGLBase.h"
#include "render/backends/gl/Qt3DSOpenGLExtensions.h"

#include <QtGui/qopenglextrafunctions.h>
#include <QtOpenGLExtensions/QtOpenGLExtensions>

namespace qt3ds {
namespace render {

    ///< forward declaration
    class NVRenderBackendMiscStateGL;

    using namespace foundation;

    class NVRenderBackendGLES2Impl : public NVRenderBackendGLBase
    {
    public:
        /// constructor
        NVRenderBackendGLES2Impl(NVFoundationBase &fnd,
                                 qt3ds::foundation::IStringTable &stringTable,
                                 const QSurfaceFormat &format);
        /// destructor
        virtual ~NVRenderBackendGLES2Impl();

        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation)

    public:
        QT3DSU32 GetDepthBits() const override;
        QT3DSU32 GetStencilBits() const override;
        void GenerateMipMaps(NVRenderBackendTextureObject to,
                             NVRenderTextureTargetType::Enum target,
                             NVRenderHint::Enum genType) override;

        void SetMultisampledTextureData2D(NVRenderBackendTextureObject to,
                                          NVRenderTextureTargetType::Enum target,
                                          size_t samples,
                                          NVRenderTextureFormats::Enum internalFormat,
                                          size_t width, size_t height,
                                          bool fixedsamplelocations) override;

        void SetTextureData3D(NVRenderBackendTextureObject to,
                              NVRenderTextureTargetType::Enum target, QT3DSU32 level,
                              NVRenderTextureFormats::Enum internalFormat, size_t width,
                              size_t height, size_t depth, QT3DSI32 border,
                              NVRenderTextureFormats::Enum format,
                              const void *hostPtr = nullptr) override;

        void SetTextureData2D(NVRenderBackendTextureObject to,
                              NVRenderTextureTargetType::Enum target, QT3DSU32 level,
                              NVRenderTextureFormats::Enum internalFormat, size_t width,
                              size_t height, QT3DSI32 border,
                              NVRenderTextureFormats::Enum format,
                              const void *hostPtr = nullptr) override;

        void UpdateSampler(
            NVRenderBackendSamplerObject so, NVRenderTextureTargetType::Enum target,
            NVRenderTextureMinifyingOp::Enum minFilter = NVRenderTextureMinifyingOp::Linear,
            NVRenderTextureMagnifyingOp::Enum magFilter = NVRenderTextureMagnifyingOp::Linear,
            NVRenderTextureCoordOp::Enum wrapS = NVRenderTextureCoordOp::ClampToEdge,
            NVRenderTextureCoordOp::Enum wrapT = NVRenderTextureCoordOp::ClampToEdge,
            NVRenderTextureCoordOp::Enum wrapR = NVRenderTextureCoordOp::ClampToEdge,
            QT3DSF32 minLod = -1000.0f, QT3DSF32 maxLod = 1000.0f, QT3DSF32 lodBias = 0.0f,
            NVRenderTextureCompareMode::Enum compareMode = NVRenderTextureCompareMode::NoCompare,
            NVRenderTextureCompareOp::Enum compareFunc = NVRenderTextureCompareOp::LessThanOrEqual,
            QT3DSF32 anisotropy = 1.0f, QT3DSF32 *borderColor = nullptr) override;

        void UpdateTextureObject(NVRenderBackendTextureObject to,
                                 NVRenderTextureTargetType::Enum target, QT3DSI32 baseLevel,
                                 QT3DSI32 maxLevel) override;

        void UpdateTextureSwizzle(NVRenderBackendTextureObject to,
                                  NVRenderTextureTargetType::Enum target,
                                  NVRenderTextureSwizzleMode::Enum swizzleMode) override;

        bool SetInputAssembler(NVRenderBackendInputAssemblerObject iao,
                               NVRenderBackendShaderProgramObject po) override;

        void SetDrawBuffers(NVRenderBackendRenderTargetObject rto,
                            NVConstDataRef<QT3DSI32> inDrawBufferSet) override;
        void SetReadBuffer(NVRenderBackendRenderTargetObject rto,
                           NVReadFaces::Enum inReadFace) override;

        void BlitFramebuffer(QT3DSI32 srcX0, QT3DSI32 srcY0, QT3DSI32 srcX1, QT3DSI32 srcY1,
                             QT3DSI32 dstX0, QT3DSI32 dstY0, QT3DSI32 dstX1, QT3DSI32 dstY1,
                             NVRenderClearFlags flags,
                             NVRenderTextureMagnifyingOp::Enum filter) override;


        NVRenderBackendRenderTargetObject CreateRenderTarget() override;
        void ReleaseRenderTarget(NVRenderBackendRenderTargetObject rto) override;
        void RenderTargetAttach(NVRenderBackendRenderTargetObject rto,
                                NVRenderFrameBufferAttachments::Enum attachment,
                                NVRenderBackendRenderbufferObject rbo) override;
        void RenderTargetAttach(NVRenderBackendRenderTargetObject rto,
                                NVRenderFrameBufferAttachments::Enum attachment,
                                NVRenderBackendTextureObject to,
                                NVRenderTextureTargetType::Enum target
                                    = NVRenderTextureTargetType::Texture2D) override;
        void RenderTargetAttach(NVRenderBackendRenderTargetObject rto,
                                NVRenderFrameBufferAttachments::Enum attachment,
                                NVRenderBackendTextureObject to, QT3DSI32 level,
                                QT3DSI32 layer) override;
        void SetRenderTarget(NVRenderBackendRenderTargetObject rto) override;
        bool RenderTargetIsValid(NVRenderBackendRenderTargetObject rto) override;

        virtual NVRenderBackendRenderbufferObject
        CreateRenderbuffer(NVRenderRenderBufferFormats::Enum storageFormat, size_t width,
                           size_t height) override;
        void SetReadTarget(NVRenderBackendRenderTargetObject rto) override;
        void ReleaseRenderbuffer(NVRenderBackendRenderbufferObject rbo) override;
        bool ResizeRenderbuffer(NVRenderBackendRenderbufferObject rbo,
                                NVRenderRenderBufferFormats::Enum storageFormat,
                                size_t width, size_t height) override;

        void *MapBuffer(NVRenderBackendBufferObject bo, NVRenderBufferBindFlags bindFlags,
                        size_t offset, size_t length,
                        NVRenderBufferAccessFlags accessFlags) override;
        bool UnmapBuffer(NVRenderBackendBufferObject bo, NVRenderBufferBindFlags bindFlags) override;

        QT3DSI32 GetConstantBufferCount(NVRenderBackendShaderProgramObject po) override;
        QT3DSI32 GetConstantBufferInfoByID(NVRenderBackendShaderProgramObject po, QT3DSU32 id,
                                           QT3DSU32 nameBufSize, QT3DSI32 *paramCount,
                                           QT3DSI32 *bufferSize, QT3DSI32 *length,
                                           char *nameBuf) override;
        void GetConstantBufferParamIndices(NVRenderBackendShaderProgramObject po, QT3DSU32 id,
                                           QT3DSI32 *indices) override;
        void GetConstantBufferParamInfoByIndices(NVRenderBackendShaderProgramObject po,
                                                 QT3DSU32 count, QT3DSU32 *indices,
                                                 QT3DSI32 *type,
                                                 QT3DSI32 *size, QT3DSI32 *offset) override;
        void ProgramSetConstantBlock(NVRenderBackendShaderProgramObject po,
                                     QT3DSU32 blockIndex, QT3DSU32 binding) override;
        void ProgramSetConstantBuffer(QT3DSU32 index, NVRenderBackendBufferObject bo) override;

        NVRenderBackendQueryObject CreateQuery() override;
        void ReleaseQuery(NVRenderBackendQueryObject qo) override;
        void BeginQuery(NVRenderBackendQueryObject qo, NVRenderQueryType::Enum type) override;
        void EndQuery(NVRenderBackendQueryObject qo, NVRenderQueryType::Enum type) override;
        void GetQueryResult(NVRenderBackendQueryObject qo,
                            NVRenderQueryResultType::Enum resultType, QT3DSU32 *params) override;
        void GetQueryResult(NVRenderBackendQueryObject qo,
                            NVRenderQueryResultType::Enum resultType, QT3DSU64 *params) override;
        void SetQueryTimer(NVRenderBackendQueryObject qo) override;

        NVRenderBackendSyncObject CreateSync(NVRenderSyncType::Enum tpye,
                                             NVRenderSyncFlags syncFlags) override;
        void ReleaseSync(NVRenderBackendSyncObject so) override;
        void WaitSync(NVRenderBackendSyncObject so, NVRenderCommandFlushFlags syncFlags,
                      QT3DSU64 timeout) override;

    protected:
        NVRenderBackendMiscStateGL *m_pCurrentMiscState; ///< this holds the current misc state
#if defined(QT_OPENGL_ES) || defined(QT_OPENGL_ES_2_ANGLE)
        Qt3DSOpenGLES2Extensions *m_qt3dsExtensions;
#endif
    };
}
}

#endif
