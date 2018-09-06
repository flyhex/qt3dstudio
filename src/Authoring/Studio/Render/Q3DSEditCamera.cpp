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

#include "Q3DSEditCamera.h"

namespace Q3DStudio {

QMatrix4x4 SEditCameraPersistentInformation::rotationMatrix() const
{
    QMatrix4x4 rotationMatrix;
    float rx, ry;
    if (m_cameraType == EditCameraTypes::Directional) {
        rx = !qIsNull(m_direction.x()) ? (m_direction.x() < 0.f ? 90.f : 270.f): 0.f;
        rx += m_direction.z() > 0.f ? 0.f : 180.f;
        ry = !qIsNull(m_direction.y()) ? (m_direction.y() < 0.f ? 90.f : -90.f): 0.f;
    } else {
        rx = float(m_xRotation);
        ry = float(-m_yRotation);
    }
    rotationMatrix.rotate(QQuaternion::fromEulerAngles(QVector3D(ry, rx, 0)));
    return rotationMatrix;
}

QVector3D SEditCameraPersistentInformation::left() const
{
    return (rotationMatrix() * QVector4D(1.f, 0.f, 0.f, 0.f)).toVector3D();
}

QVector3D SEditCameraPersistentInformation::up() const
{
    return (rotationMatrix() * QVector4D(0.f, 1.f, 0.f, 0.f)).toVector3D();
}

QVector3D SEditCameraPersistentInformation::front() const
{
    return (rotationMatrix() * QVector4D(0.f, 0.f, 1.f, 0.f)).toVector3D();
}

void SEditCameraPersistentInformation::applyToCamera(Q3DSCameraNode &camera,
                                                     const QSizeF &viewport)
{
    Q3DSPropertyChangeList changeList;
    changeList.append(camera.setClipFar(5000.f));
    changeList.append(camera.setClipNear(1.f));
    if (m_cameraType == EditCameraTypes::Perspective) {
        changeList.append(camera.setFov(g_editCameraFOV));
        changeList.append(camera.setOrthographic(false));
    } else {
        changeList.append(camera.setOrthographic(true));
    }

    // The view radius dictates the zoom.
    float zoom = 1.f;
    if (camera.orthographic()) {
        float theViewport = qMin(viewport.width(), viewport.height());
        zoom = (m_viewRadius * 2.f) / theViewport;
    } else {
        // We know the hypotenuse is 600.
        // So if we want to zoom the scene, we do this.
        zoom = m_viewRadius / (qSin(qDegreesToRadians(camera.fov()) / 2.f) * 600.f);
    }

    QVector3D pos = m_position - front() * 600.f;

    float rx, ry;
    if (m_cameraType == EditCameraTypes::Directional) {
        rx = !qIsNull(m_direction.x()) ? (m_direction.x() < 0.f ? 90.f : 270.f): 0.f;
        rx += m_direction.z() > 0.f ? 0.f : 180.f;
        ry = !qIsNull(m_direction.y()) ? (m_direction.y() < 0.f ? 90.f : -90.f): 0.f;
    } else {
        rx = float(m_xRotation);
        ry = float(-m_yRotation);
    }

    changeList.append(camera.setPosition(pos));
    changeList.append(camera.setZoom(zoom));
    changeList.append(camera.setRotation(QVector3D(ry, rx, 0)));

    if (!changeList.isEmpty())
        camera.notifyPropertyChanges(changeList);
}

}
