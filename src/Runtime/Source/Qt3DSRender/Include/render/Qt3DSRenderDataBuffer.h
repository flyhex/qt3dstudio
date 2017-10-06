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
#ifndef QT3DS_RENDER_DATA_BUFFER_H
#define QT3DS_RENDER_DATA_BUFFER_H
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/backends/Qt3DSRenderBackend.h"

namespace qt3ds {
class NVFoundationBase;
}

namespace qt3ds {
namespace render {

    // forward declaration
    class NVRenderContextImpl;
    class NVRenderBackend;

    ///< Base class
    class NVRenderDataBuffer : public NVRefCounted, public NVRenderImplemented
    {
    protected:
        NVRenderContextImpl &m_Context; ///< pointer to context
        NVFoundationBase &m_Foundation; ///< pointer to foundation
        volatile QT3DSI32 mRefCount; ///< Using foundations' naming convention to ease implementation
        NVRenderBackend *m_Backend; ///< pointer to backend
        NVRenderBufferUsageType::Enum m_UsageType; ///< usage type
        NVRenderBufferBindFlags m_BindFlags; ///< bind flags
        NVDataRef<QT3DSU8> m_BufferData; ///< buffer data pointer
        QT3DSU32 m_BufferCapacity; ///< size of internal backup buffer (m_BufferData)
        size_t m_BufferSize; ///< size of buffer
        bool m_OwnsData; ///< true when we own m_BufferData
        bool m_Mapped; ///< true when locked for reading or writing to m_BufferData
        NVRenderBackend::NVRenderBackendBufferObject m_BufferHandle; ///< opaque backend handle

    public:
        /**
         * @brief constructor
         *
         * @param[in] context		Pointer to context
         * @param[in] size			Size of the buffer
         * @param[in] bindFlags		Where to binf this buffer (e.g. vertex, index, ...)
         *							For OpenGL this should be a single
         *value
         * @param[in] usage			Usage of the buffer (e.g. static, dynamic...)
         * @param[in] data			A pointer to the buffer data that is allocated by the
         *application.
         *
         * @return No return.
         */
        NVRenderDataBuffer(NVRenderContextImpl &context, NVFoundationBase &fnd, size_t size,
                           NVRenderBufferBindFlags bindFlags,
                           NVRenderBufferUsageType::Enum usageType, NVDataRef<QT3DSU8> data);

        virtual ~NVRenderDataBuffer();

        // define refcount functions
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation)

        /**
         * @brief Get Buffer usage type
         *
         * @return Return usage tyoe
         */
        virtual NVRenderBufferUsageType::Enum GetBufferUsageType() const
        {
            return m_UsageType;
        }

        /**
         * @brief Get Buffer usage type
         *		  Should be overwritten
         *
         * @return Return usage tyoe
         */
        virtual NVRenderBufferBindFlags GetBufferBindings() const { return m_BindFlags; }

        /**
         * @brief Return buffer size in byte
         *
         * @return Return size
         */
        virtual QT3DSU32 Size() { return (QT3DSU32)m_BufferSize; }

        /**
         * @brief Get a pointer to the foundation
         *
         * @return pointer to foundation
         */
        NVFoundationBase &GetFoundation() { return m_Foundation; }

        /**
         * @brief bind the buffer bypasses the context state
         *
         * @return no return.
         */
        virtual void Bind() = 0;

        /**
         * @brief Map the buffer
         *		  Mapped buffers cannot be used for rendering
         *
         * @return Return mapped pointer to data
         */
        virtual NVDataRef<QT3DSU8> MapBuffer();

        /**
         * @brief Map a range of a  buffer
         *		  Mapped buffers cannot be used for rendering
         *
         * @param[in] offset	Offset in bytes into the buffer
         * @param[in] size		Range in bytes to map
         * @param[in] flags		Buffer access flags
         *
         * @return Return mapped pointer to data
         */
        virtual NVDataRef<QT3DSU8> MapBufferRange(size_t offset, size_t size,
                                               NVRenderBufferAccessFlags flags);

        /**
         * @brief Unmap the buffer
         *		  This updates the data to the hardware
         *
         * @return no return
         */
        virtual void UnmapBuffer();

        /**
         * @brief constructor
         *
         * @param[in] data			A pointer to the buffer data that is allocated by the
         * application.
         * @param[in] ownsMemory	If true data will be owned by this object and can therefore be
         * released
         *
         * @return No return.
         */
        virtual void UpdateBuffer(NVConstDataRef<QT3DSU8> data, bool ownsMemory = false);

        /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
        virtual NVRenderBackend::NVRenderBackendBufferObject GetBuffertHandle() const = 0;

        // this will be obsolete
        const void *GetImplementationHandle() const override = 0;

    private:
        void releaseMemory();
    };
}
}

#endif
