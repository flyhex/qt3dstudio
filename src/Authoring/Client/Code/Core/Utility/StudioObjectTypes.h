/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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
#ifndef INCLUDED_STUDIO_OBJECTTYPES_H
#define INCLUDED_STUDIO_OBJECTTYPES_H

#pragma once

#include "PropertyPublishLevels.h"
#include "Qt3DSString.h"
//
//	*******************************************************************
//	IMPORTANT:	These object types (EStudioObjectType) line up with the
//				icons used for the timeline for each object type
//	*******************************************************************
//

enum EStudioObjectType {
    OBJTYPE_UNKNOWN = 0x00000000,
    OBJTYPE_SCENE = 0x00000001,
    OBJTYPE_LAYER = 0x00000002,
    OBJTYPE_BEHAVIOR = 0x00000004,
    OBJTYPE_MATERIAL = 0x00000008,
    OBJTYPE_CAMERA = 0x00000010,
    OBJTYPE_LIGHT = 0x00000020,
    OBJTYPE_MODEL = 0x00000040,
    OBJTYPE_GROUP = 0x00000080,
    OBJTYPE_IMAGE = 0x00000400,
    OBJTYPE_TEXT = 0x00001000,
    OBJTYPE_COMPONENT = 0x00002000,
    OBJTYPE_SLIDE = 0x00004000,
    OBJTYPE_EFFECT = 0x00008000,
    OBJTYPE_RENDERPLUGIN = 0x00010000,
    OBJTYPE_CUSTOMMATERIAL = 0x00020000,
    OBJTYPE_REFERENCEDMATERIAL = 0x00040000,
    OBJTYPE_GUIDE = 0x00080000,
    OBJTYPE_ALIAS = 0x00100000,
    OBJTYPE_PATH = 0x00200000,
    OBJTYPE_PATHANCHORPOINT = 0x00400000,
    OBJTYPE_SUBPATH = 0x00800000,
    OBJTYPE_SOUND = 0x01000000,
    OBJTYPE_LIGHTMAPS = 0x02000000,
    OBJTYPE_DATAINPUT = 0x04000000,
    OBJTYPE_PRESENTATION = 0x08000000,
    OBJTYPE_PROJECT = 0x10000000,
    OBJTYPE_QML_STREAM = 0x20000000
};

typedef enum {
    PRIMITIVETYPE_UNKNOWN = 0,
    PRIMITIVETYPE_RECT,
    PRIMITIVETYPE_SPHERE,
    PRIMITIVETYPE_CONE,
    PRIMITIVETYPE_CYLINDER,
    PRIMITIVETYPE_BOX,

} EPrimitiveType;

typedef enum {
    PRIMITIVEMODE_UNKNOWN = 0,
    PRIMITIVEMODE_SINGLESIDED = 1,
    PRIMITIVEMODE_DOUBLESIDED = 2,
    PRIMITIVEMODE_SEPARATE_MATERIALS = 4,

} EPrimitiveMode;

/**
Text box type.
 */
enum ETextTextType {
    ETextTextTypeLine, ///<Line
    ETextTextTypeParagraph, ///<Paragraph

    ETextTextType_Last
};

/**
Treatment of text by the rendering engine
 */
enum ETextRenderStyle {
    ETextRenderStyleFlat, ///<The text will always face the camera and not scale
    ETextRenderStyleDepth, ///<The text will always face the camera, but grow larger or smaller
                           ///based on distance
    ETextRenderStyleFullPerspective, ///<The text is treated as full 3D

    ETextRenderStyle_Last
};

/**
Horizontal Alignment
 */
enum ETextHorzAlign {
    ETextHorzAlignLeft, ///<Left
    ETextHorzAlignCenter, ///<Center
    ETextHorzAlignRight, ///<Right

    ETextHorzAlign_Last
};

/**
Vertical Alignment
 */
enum ETextVertAlign {
    ETextVertAlignTop, ///<Top
    ETextVertAlignMiddle, ///<Middle
    ETextVertAlignBottom, ///<Bottom

    ETextVertAlign_Last
};

class CStudioObjectTypes
{
public:
    static bool AcceptableParent(EStudioObjectType inChild, EStudioObjectType inParent);

    static const QString &GetNormalIconName(EStudioObjectType inType);
    static const QString &GetDisabledIconName(EStudioObjectType inType);
    static const QString &GetHighlightIconName(EStudioObjectType inType);

};

#endif // INCLUDED_STUDIO_OBJECTTYPES_H
