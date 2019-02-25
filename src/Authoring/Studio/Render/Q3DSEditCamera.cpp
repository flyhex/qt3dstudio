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

static const qreal defaultEditCameraRadius = qSin(qDegreesToRadians(g_editCameraFOV) / 2.) * 600.;

QMatrix4x4 SEditCameraPersistentInformation::rotationMatrix() const
{
    QMatrix4x4 rotationMatrix;
    float rx, ry;
    if (m_cameraType == EditCameraTypes::Directional) {
        rx = !qFuzzyIsNull(m_direction.x()) ? (m_direction.x() < 0.f ? 90.f : 270.f) : 0.f;
        rx += !qFuzzyIsNull(m_direction.z()) ? (m_direction.z() < 0.f ? 180.f : 0.f) : 0.f;
        ry = !qFuzzyIsNull(m_direction.y()) ? (m_direction.y() < 0.f ? 90.f : -90.f) : 0.f;
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
    changeList.append(camera.setClipFar(2000000.f));
    changeList.append(camera.setClipNear(1.f));

    QVector3D posAdjust = front() * 600.f;
    if (m_cameraType == EditCameraTypes::Perspective) {
        changeList.append(camera.setFov(g_editCameraFOV));
        changeList.append(camera.setOrthographic(false));
        changeList.append(camera.setZoom(1.f));
        // Handle perspective zooming by adjusting the camera position, as scaling projection
        // matrix after the fact doesn't work correctly for our purposes
        qreal factor = zoomFactor(viewport);
        if (factor > 1)
            posAdjust *= float(factor);
        else
            posAdjust *= -1.f + (2.f * factor);
    } else {
        changeList.append(camera.setOrthographic(true));
        // Orthogonal views can't zoom by adjusting position
        changeList.append(camera.setZoom(zoomFactor(viewport)));
    }

    QVector3D pos = m_position - posAdjust;

    float rx, ry;
    if (m_cameraType == EditCameraTypes::Directional) {
        rx = !qFuzzyIsNull(m_direction.x()) ? (m_direction.x() < 0.f ? 90.f : 270.f) : 0.f;
        rx += !qFuzzyIsNull(m_direction.z()) ? (m_direction.z() < 0.f ? 180.f : 0.f) : 0.f;
        ry = !qFuzzyIsNull(m_direction.y()) ? (m_direction.y() < 0.f ? 90.f : -90.f) : 0.f;
    } else {
        rx = float(m_xRotation);
        ry = float(-m_yRotation);
    }

    changeList.append(camera.setPosition(pos));
    changeList.append(camera.setRotation(QVector3D(ry, rx, 0)));

    if (!changeList.isEmpty())
        camera.notifyPropertyChanges(changeList);
}

qreal SEditCameraPersistentInformation::zoomFactor(const QSizeF &viewport) const
{
    qreal zoom = 1.;
    if (m_cameraType != EditCameraTypes::Perspective) {
        qreal theViewport = qMin(viewport.width(), viewport.height());
        zoom = 2. * m_viewRadius / theViewport;
    } else {
        zoom = m_viewRadius / defaultEditCameraRadius;
    }
    return zoom;
}

}
