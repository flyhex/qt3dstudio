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
#ifndef QT3DS_RENDER_GRAPH_OBJECT_TYPES_H
#define QT3DS_RENDER_GRAPH_OBJECT_TYPES_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSAssert.h"

namespace qt3ds {
namespace render {

// If you need a generic switch statement, then these macros will ensure
// you get all the types the first time.
#define QT3DS_RENDER_ITERATE_GRAPH_OBJECT_TYPES                                                      \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(Presentation)                                               \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(Scene)                                                      \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(Node)                                                       \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(Layer)                                                      \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(Light)                                                      \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(Camera)                                                     \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(Model)                                                      \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(DefaultMaterial)                                            \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(Image)                                                      \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(Text)                                                       \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(Effect)                                                     \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(RenderPlugin)                                               \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(CustomMaterial)                                             \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(ReferencedMaterial)                                         \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(Path)                                                       \
    QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(PathSubPath)

    struct GraphObjectTypes
    {
        enum Enum {
            Unknown = 0,
            Presentation,
            Scene,
            Node,
            Layer,
            Light,
            Camera,
            Model,
            DefaultMaterial,
            Image,
            Text,
            Effect,
            CustomMaterial,
            RenderPlugin,
            ReferencedMaterial,
            Path,
            PathSubPath,
            Lightmaps,
            LastKnownGraphObjectType,
        };

        static bool IsMaterialType(Enum type)
        {
            switch (type) {
            case ReferencedMaterial:
            case CustomMaterial:
            case DefaultMaterial:
                return true;
            default:
                return false;
            }
        }

        static bool IsLightmapType(Enum type)
        {
            switch (type) {
            case Lightmaps:
            case DefaultMaterial:
                return true;
            default:
                return false;
            }
        }

        static bool IsNodeType(Enum type)
        {
            switch (type) {
            case Node:
            case Layer:
            case Light:
            case Camera:
            case Model:
            case Text:
            case Path:
                return true;

            default:
                break;
            }
            return false;
        }

        static bool IsRenderableType(Enum type)
        {
            switch (type) {
            case Model:
            case Text:
            case Path:
                return true;
            default:
                break;
            }
            return false;
        }

        static bool IsLightCameraType(Enum type)
        {
            switch (type) {
            case Camera:
            case Light:
                return true;
            default:
                break;
            }
            return false;
        }
        static const char *GetObjectTypeName(Enum inType)
        {
            switch (inType) {
#define QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(type)                                                   \
    case type:                                                                                     \
        return #type;
                QT3DS_RENDER_ITERATE_GRAPH_OBJECT_TYPES
#undef QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return "";
        }
    };
}
}

#endif