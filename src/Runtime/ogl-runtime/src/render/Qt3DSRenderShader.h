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
#ifndef QT3DS_RENDER_SHADER_H
#define QT3DS_RENDER_SHADER_H

#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderContext.h"
#include "render/backends/Qt3DSRenderBackend.h"
#include <EASTL/string.h>

namespace qt3ds {
namespace render {
    using namespace foundation;

    class NVRenderContextImpl;

    ///< A shader program is an object composed of a multiple shaders (vertex, fragment,
    ///geometry,....)
    class NVRenderShader
    {
    protected:
        NVRenderContextImpl &m_Context; ///< pointer to context
        NVFoundationBase &m_Foundation; ///< pointer to foundation
        NVRenderBackend *m_Backend; ///< pointer to backend
        NVConstDataRef<QT3DSI8> m_Source; ///< shader source code
        bool m_Binary; ///< true for binary programs
        eastl::string m_ErrorMessage; ///< contains the error message if linking fails

    public:
        /**
         * @brief constructor
         *
         * @param[in] context		Pointer to render context
         * @param[in] fnd			Pointer to foundation
         *
         * @return No return.
         */
        NVRenderShader(NVRenderContextImpl &context, NVFoundationBase &fnd,
                       NVConstDataRef<QT3DSI8> source, bool binaryProgram)
            : m_Context(context)
            , m_Foundation(fnd)
            , m_Backend(context.GetBackend())
            , m_Source(source)
            , m_Binary(binaryProgram)
        {
        }

        /// @brief destructor
        ~NVRenderShader(){}

        /**
         * @brief Query if shader compiled succesfuly
         *
         * @return True if shader is valid.
         */
        virtual bool IsValid() = 0;

        /**
         * @brief Get Error Message
         *
         * @param[out] messageLength	Pointer to error string
         * @param[out] messageLength	Size of error meesage
         *
         * @return no return
         */
        virtual void GetErrorMessage(QT3DSI32 *messageLength, const char *errorMessage)
        {
            // Since we do not have any error message just generate a generic one
            if (m_Binary)
                m_ErrorMessage = "Binary shader compilation failed";

            *messageLength = m_ErrorMessage.size();
            errorMessage = m_ErrorMessage.c_str();
        }

        /**
         * @brief Get Error Message
         *
         *
         * @return error message.
         */
        virtual const char *GetErrorMessage()
        {
            if (m_Binary)
                m_ErrorMessage = "Binary shader compilation failed";

            return m_ErrorMessage.c_str();
        }
    };
}
}

#endif
