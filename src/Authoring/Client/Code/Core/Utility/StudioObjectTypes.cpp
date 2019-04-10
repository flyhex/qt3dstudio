/****************************************************************************
**
** Copyright (C) 2000 NVIDIA Corporation.
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

#include "StudioObjectTypes.h"

bool CStudioObjectTypes::AcceptableParent(EStudioObjectType inChild, EStudioObjectType inParent)
{
    bool theAcceptible = false;
    switch (inChild) {
    case OBJTYPE_SCENE:
        theAcceptible = false;
        break;

    case OBJTYPE_LAYER:
        theAcceptible = inParent == OBJTYPE_SCENE;
        break;

    case OBJTYPE_BEHAVIOR:
        theAcceptible = (inParent == OBJTYPE_SCENE) || (inParent == OBJTYPE_LAYER)
            || (inParent == OBJTYPE_BEHAVIOR) || (inParent == OBJTYPE_CAMERA)
            || (inParent == OBJTYPE_LIGHT) || (inParent == OBJTYPE_MODEL)
            || (inParent == OBJTYPE_GROUP) || (inParent == OBJTYPE_COMPONENT)
            || (inParent == OBJTYPE_IMAGE) || (inParent == OBJTYPE_TEXT)
            || (inParent == OBJTYPE_PATH);
        break;
    case OBJTYPE_MATERIAL:
        theAcceptible = false;
        break;

    case OBJTYPE_TEXT:
    case OBJTYPE_CAMERA:
    case OBJTYPE_LIGHT:
    case OBJTYPE_MODEL:
    case OBJTYPE_GROUP:
    case OBJTYPE_PATH:
    case OBJTYPE_COMPONENT:
        theAcceptible = (inParent == OBJTYPE_LAYER) || (inParent == OBJTYPE_CAMERA)
            || (inParent == OBJTYPE_LIGHT) || (inParent == OBJTYPE_MODEL)
            || (inParent == OBJTYPE_GROUP) || (inParent == OBJTYPE_COMPONENT)
            || (inParent == OBJTYPE_PATH);
        break;

    case OBJTYPE_ALIAS:
        theAcceptible = (inParent == OBJTYPE_LAYER) || (inParent == OBJTYPE_GROUP)
            || (inParent == OBJTYPE_COMPONENT) || (inParent == OBJTYPE_PATH);
        break;

    case OBJTYPE_IMAGE:
        theAcceptible = false;
        break;

    case OBJTYPE_EFFECT:
        theAcceptible = (inParent == OBJTYPE_LAYER) || (inParent == OBJTYPE_EFFECT);
        break;

    case OBJTYPE_PRESENTATION:
    case OBJTYPE_QML_STREAM:
        theAcceptible = inParent & (OBJTYPE_LAYER | OBJTYPE_SCENE);
        break;

    case OBJTYPE_CUSTOMMATERIAL:
        theAcceptible = false; // TODO add drag and drop support
        break;

    case OBJTYPE_MATERIALDATA:
        theAcceptible = true;
        break;

    default:
        // Do not accept unexpected type
        break;
    }

    return theAcceptible;
}

//==============================================================================
//	 Object Icon Lookups
//==============================================================================

//=============================================================================
/**
 * Get the normal state object specific icon.
 * @return the icon to be used in the 'normal' state.
 */
const QString &CStudioObjectTypes::GetNormalIconName(EStudioObjectType inType)
{
    switch (inType) {
    case OBJTYPE_CAMERA: {
        static QString theString = QStringLiteral("Objects-Camera-Normal.png");
        return theString;
    }
    case OBJTYPE_IMAGE: {
        static QString theString = QStringLiteral("Objects-Image-Normal.png");
        return theString;
    }
    case OBJTYPE_LIGHT: {
        static QString theString = QStringLiteral("Objects-Light-Normal.png");
        return theString;
    }
    case OBJTYPE_MODEL: {
        static QString theString = QStringLiteral("Objects-Model-Normal.png");
        return theString;
    }
    case OBJTYPE_GROUP: {
        static QString theString = QStringLiteral("Objects-Group-Normal.png");
        return theString;
    }
    case OBJTYPE_ALIAS: {
        static QString theString = QStringLiteral("Objects-Alias-Normal.png");
        return theString;
    }
    case OBJTYPE_PATH: {
        static QString theString = QStringLiteral("Objects-Path-Normal.png");
        return theString;
    }
    case OBJTYPE_PATHANCHORPOINT: {
        static QString theString = QStringLiteral("Objects-Anchor-Normal.png");
        return theString;
    }
    case OBJTYPE_SUBPATH: {
        static QString theString = QStringLiteral("Objects-SubPath-Normal.png");
        return theString;
    }
    case OBJTYPE_COMPONENT: {
        static QString theString = QStringLiteral("Objects-Component-Normal.png");
        return theString;
    }
    case OBJTYPE_LAYER: {
        static QString theString = QStringLiteral("Objects-Layer-Normal.png");
        return theString;
    }
    case OBJTYPE_TEXT: {
        static QString theString = QStringLiteral("Objects-Text-Normal.png");
        return theString;
    }
    case OBJTYPE_MATERIAL:
    case OBJTYPE_CUSTOMMATERIAL:
    case OBJTYPE_REFERENCEDMATERIAL: {
        static QString theString = QStringLiteral("Objects-Material-Normal.png");
        return theString;
    }
    case OBJTYPE_SCENE:
    case OBJTYPE_SLIDE: {
        static QString theString = QStringLiteral("Objects-Scene-Normal.png");
        return theString;
    }
    case OBJTYPE_BEHAVIOR: {
        static QString theString = QStringLiteral("Objects-Behavior-Normal.png");
        return theString;
    }
    case OBJTYPE_EFFECT: {
        static QString theString = QStringLiteral("Objects-Effect-Normal.png");
        return theString;
    }
    case OBJTYPE_SOUND: {
        static QString theString = QStringLiteral("Objects-Sound-Normal.png");
        return theString;
    }
    case OBJTYPE_DATAINPUT: {
        static QString theString = QStringLiteral("Objects-DataInput-Active.png");
        return theString;
    }
    case OBJTYPE_PRESENTATION: {
        static QString theString = QStringLiteral("presentation_used.png");
        return theString;
    }
    case OBJTYPE_QML_STREAM: {
        static QString theString = QStringLiteral("qml_used.png");
        return theString;
    }
    case OBJTYPE_MATERIALDATA: {
        static QString theString = QStringLiteral("matdef-active.png");
        return theString;
    }
    default: {
        static QString theString = QStringLiteral("Objects-Model-Normal.png");
        return theString;
    }
    }
}

//=============================================================================
/**
 * Get the disabled state object specific icon.
 * @return the icon to be used in the disabled state.
 */
const QString &CStudioObjectTypes::GetDisabledIconName(EStudioObjectType inType)
{
    switch (inType) {
    case OBJTYPE_CAMERA: {
        static QString theString = QStringLiteral("Objects-Camera-Disabled.png");
        return theString;
    }
    case OBJTYPE_IMAGE: {
        static QString theString = QStringLiteral("Objects-Image-Disabled.png");
        return theString;
    }
    case OBJTYPE_LIGHT: {
        static QString theString = QStringLiteral("Objects-Light-Disabled.png");
        return theString;
    }
    case OBJTYPE_MODEL: {
        static QString theString = QStringLiteral("Objects-Model-Disabled.png");
        return theString;
    }
    case OBJTYPE_GROUP: {
        static QString theString = QStringLiteral("Objects-Group-Disabled.png");
        return theString;
    }
    case OBJTYPE_ALIAS: {
        static QString theString = QStringLiteral("Objects-Alias-Disabled.png");
        return theString;
    }
    case OBJTYPE_PATH: {
        static QString theString = QStringLiteral("Objects-Path-Normal.png");
        return theString;
    }
    case OBJTYPE_PATHANCHORPOINT: {
        static QString theString = QStringLiteral("Objects-Anchor-Normal.png");
        return theString;
    }
    case OBJTYPE_SUBPATH: {
        static QString theString = QStringLiteral("Objects-SubPath-Normal.png");
        return theString;
    }
    case OBJTYPE_COMPONENT: {
        static QString theString = QStringLiteral("Objects-Component-Disabled.png");
        return theString;
    }
    case OBJTYPE_LAYER: {
        static QString theString = QStringLiteral("Objects-Layer-Disabled.png");
        return theString;
    }
    case OBJTYPE_TEXT: {
        static QString theString = QStringLiteral("Objects-Text-Disabled.png");
        return theString;
    }
    case OBJTYPE_MATERIAL:
    case OBJTYPE_CUSTOMMATERIAL:
    case OBJTYPE_REFERENCEDMATERIAL: {
        static QString theString = QStringLiteral("Objects-Material-Disabled.png");
        return theString;
    }
    case OBJTYPE_SCENE: {
        static QString theString = QStringLiteral("Objects-Scene-Disabled.png");
        return theString;
    }
    case OBJTYPE_BEHAVIOR: {
        static QString theString = QStringLiteral("Objects-Behavior-Disabled.png");
        return theString;
    }
    case OBJTYPE_EFFECT: {
        static QString theString = QStringLiteral("Objects-Effect-Disabled.png");
        return theString;
    }
    case OBJTYPE_SOUND: {
        static QString theString = QStringLiteral("Objects-Sound-Disabled.png");
        return theString;
    }
    case OBJTYPE_DATAINPUT: {
        static QString theString = QStringLiteral("Objects-DataInput-Inactive.png");
        return theString;
    }
    case OBJTYPE_PRESENTATION: {
        static QString theString = QStringLiteral("presentation_notUsed.png");
        return theString;
    }
    case OBJTYPE_QML_STREAM: {
        static QString theString = QStringLiteral("qml_notUsed.png");
        return theString;
    }
    case OBJTYPE_MATERIALDATA: {
        static QString theString = QStringLiteral("matdef-disabled.png");
        return theString;
    }
    default: {
        static QString theString = QStringLiteral("Objects-Model-Disabled.png");
        return theString;
    }
    }
}

//=============================================================================
/**
 * Get the highlight state object specific icon.
 * @return the icon to be used in the highlighted state.
 */
const QString &CStudioObjectTypes::GetHighlightIconName(EStudioObjectType inType)
{
    switch (inType) {
    case OBJTYPE_CAMERA: {
        static QString theString = QStringLiteral("Objects-Camera-Normal.png");
        return theString;
    }
    case OBJTYPE_IMAGE: {
        static QString theString = QStringLiteral("Objects-Image-Normal.png");
        return theString;
    }
    case OBJTYPE_LIGHT: {
        static QString theString = QStringLiteral("Objects-Light-Normal.png");
        return theString;
    }
    case OBJTYPE_MODEL: {
        static QString theString = QStringLiteral("Objects-Model-Normal.png");
        return theString;
    }
    case OBJTYPE_GROUP: {
        static QString theString = QStringLiteral("Objects-Group-Normal.png");
        return theString;
    }
    case OBJTYPE_PATH: {
        static QString theString = QStringLiteral("Objects-Path-Normal.png");
        return theString;
    }
    case OBJTYPE_PATHANCHORPOINT: {
        static QString theString = QStringLiteral("Objects-Anchor-Normal.png");
        return theString;
    }
    case OBJTYPE_SUBPATH: {
        static QString theString = QStringLiteral("Objects-SubPath-Normal.png");
        return theString;
    }
    case OBJTYPE_ALIAS: {
        static QString theString = QStringLiteral("Objects-Alias-Normal.png");
        return theString;
    }
    case OBJTYPE_COMPONENT: {
        static QString theString = QStringLiteral("Objects-Component-Normal.png");
        return theString;
    }
    case OBJTYPE_LAYER: {
        static QString theString = QStringLiteral("Objects-Layer-Normal.png");
        return theString;
    }
    case OBJTYPE_TEXT: {
        static QString theString = QStringLiteral("Objects-Text-Normal.png");
        return theString;
    }
    case OBJTYPE_MATERIAL:
    case OBJTYPE_CUSTOMMATERIAL:
    case OBJTYPE_REFERENCEDMATERIAL: {
        static QString theString = QStringLiteral("Objects-Material-Normal.png");
        return theString;
    }
    case OBJTYPE_SCENE: {
        static QString theString = QStringLiteral("Objects-Scene-Normal.png");
        return theString;
    }
    case OBJTYPE_BEHAVIOR: {
        static QString theString = QStringLiteral("Objects-Behavior-Normal.png");
        return theString;
    }
    case OBJTYPE_EFFECT: {
        static QString theString = QStringLiteral("Objects-Effect-Normal.png");
        return theString;
    }
    case OBJTYPE_SOUND: {
        static QString theString = QStringLiteral("Objects-Sound-Normal.png");
        return theString;
    }
    case OBJTYPE_PRESENTATION: {
        static QString theString = QStringLiteral("presentation_used.png");
        return theString;
    }
    case OBJTYPE_MATERIALDATA: {
        static QString theString = QStringLiteral("matdef-active.png");
        return theString;
    }
    default: {
        static QString theString = QStringLiteral("Objects-Model-Normal.png");
        return theString;
    }
    }
}
