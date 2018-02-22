/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef ROWTYPES_H
#define ROWTYPES_H

#include <qglobal.h>

//namespace timeline {

enum class RowType {
    Scene = 90,
    Layer,
    Camera,
    Light,
    Object,
    Text,
    Alias,
    Group,
    Component,
    Property
};

enum class PropertyType {
    None = 540,
    Position,
    Rotation,
    Scale,
    Pivot,
    Opacity,
    EdgeTessellation,
    InnerTessellation,
    TextColor,          // Text
    Leading,            // Text
    Tracking,           // Text
    LightColor,         // Light
    SpecularColor,      // Light
    AmbientColor,       // Light
    Brightness,         // Light
    ShadowDarkness,     // Light
    ShadowSoftness,     // Light
    ShadowDepthBias,    // Light
    ShadowFarClip,      // Light
    ShadowFOV,          // Light
    FieldOfView,        // Camera
    ClippingStart,      // Camera
    ClippingEnd,        // Camera
    Left,               // Layer
    Top,                // Layer
    Width,              // Layer
    Height,             // Layer
    AO,                 // Layer
    AODistance,         // Layer
    AOSoftness,         // Layer
    AOThreshold,        // Layer
    AOSamplingRate,     // Layer
    IBLBrightness,      // Layer
    IBLHorizonCutoff,   // Layer
    IBLFOVAngle,        // Layer
    ProbeCrossfade,     // Layer
};

inline uint qHash(RowType key) {
    return static_cast<uint>(key);
}

#endif // ROWTYPES_H
