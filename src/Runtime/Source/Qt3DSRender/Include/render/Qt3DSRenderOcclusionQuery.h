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
#ifndef QT3DS_RENDER_OCCLUSION_QUERY_H
#define QT3DS_RENDER_OCCLUSION_QUERY_H

#include "render/Qt3DSRenderQueryBase.h"

namespace qt3ds {
class NVFoundationBase;
}

namespace qt3ds {
namespace render {

    // forward declaration
    class NVRenderContextImpl;

    class NVRenderOcclusionQuery : public NVRenderQueryBase
    {
    public:
        /**
         * @brief constructor
         *
         * @param[in] context		Pointer to context
         * @param[in] fnd			Pointer to foundation
         *
         * @return No return.
         */
        NVRenderOcclusionQuery(NVRenderContextImpl &context, NVFoundationBase &fnd);

        ///< destructor
        ~NVRenderOcclusionQuery();

        /**
         * @brief Get query type
         *
         * @return Return query type
         */
        NVRenderQueryType::Enum GetQueryType() const override
        {
            return NVRenderQueryType::Samples;
        }

        /**
         * @brief Get a pointer to the foundation
         *
         * @return pointer to foundation
         */
        NVFoundationBase &GetFoundation() { return m_Foundation; }

        /**
         * @brief begin a query
         *
         * @return no return.
         */
        void Begin() override;

        /**
         * @brief end a query
         *
         * @return no return.
         */
        void End() override;

        /**
         * @brief Get the result of a query
         *
         * @param[out] params	Contains result of query regarding query type
         *
         * @return no return.
         */
        void GetResult(QT3DSU32 *params) override;

        /**
         * @brief query if a result is available
         *
         *
         * @return true if available.
         */
        virtual bool GetResultAvailable();

        /*
         * @brief static creation function
         *
         * * @return a occlusion query object on success
         */
        static NVRenderOcclusionQuery *Create(NVRenderContextImpl &context);
    };
}
}

#endif
