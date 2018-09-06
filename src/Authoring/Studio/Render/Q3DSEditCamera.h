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

#ifndef Q3DS_EDIT_CAMERA_H
#define Q3DS_EDIT_CAMERA_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "q3dsruntime2api_p.h"

#include <QtCore/qmath.h>

namespace Q3DStudio {

enum class EditCameraTypes
{
    SceneCamera = 0,
    Perspective,
    Orthographic,
    Directional,
};

const qreal g_editCameraFOV = 45.0;
const qreal g_rotationScaleFactor = 2.0 * qreal(M_PI) / 180.0;

struct SEditCameraPersistentInformation
{
    QString m_name;
    QVector3D m_position;
    QVector3D m_direction;
    qreal m_viewRadius;
    qreal m_xRotation = 0.;
    qreal m_yRotation = 0.;
    EditCameraTypes m_cameraType;
    bool m_initialized = false;

    SEditCameraPersistentInformation()
        : m_viewRadius(600.f)
        , m_cameraType(EditCameraTypes::Perspective)
    {
    }

    QMatrix4x4 rotationMatrix() const;

    QVector3D left() const;
    QVector3D up() const;
    QVector3D front() const;

    void applyToCamera(Q3DSCameraNode &inCamera, const QSizeF &inViewport);

    bool orthographic() const { return m_cameraType != EditCameraTypes::Perspective; }

    bool supportsRotation() const { return m_cameraType != EditCameraTypes::Directional; }
};

}

#endif
