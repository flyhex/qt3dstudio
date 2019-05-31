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
#ifndef QT3DS_RENDER_QT3DS_RENDER_DRAWABLE_H
#define QT3DS_RENDER_QT3DS_RENDER_DRAWABLE_H
#include "Qt3DSRenderBaseTypes.h"

namespace qt3ds {
namespace render {
    class NVRenderDrawable
    {
    protected:
        virtual ~NVRenderDrawable() {}

    public:
        /**
         *	Draw using this object.
         *	offset is in num elements, not in number of bytes
         *	because the various different draw functions differ in what datatype
         *	offset is (gl drawarrays vs gl drawindexedarras).
         */
        virtual void Draw(NVRenderDrawMode::Enum drawMode, QT3DSU32 count, QT3DSU32 offset) = 0;
    };
}
}

#endif
