/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef Q3DSWIDGETUTILS_H
#define Q3DSWIDGETUTILS_H

#include "q3dsruntime2api_p.h"

namespace Q3DStudio {

struct BoundingBox
{
    QVector3D min = QVector3D(std::numeric_limits<float>::max(),
                              std::numeric_limits<float>::max(),
                              std::numeric_limits<float>::max());
    QVector3D max = QVector3D(-std::numeric_limits<float>::max(),
                              -std::numeric_limits<float>::max(),
                              -std::numeric_limits<float>::max());
    bool isEmpty() { return min == max; }
    void setEmpty() {
        min = QVector3D(0, 0, 0);
        max = QVector3D(0, 0, 0);
    }
    QVector3D center() { return min + (max - min) / 2.f; }
    QVector3D extents() { return max - min; }
};

void adjustRotationLeftToRight(QMatrix4x4 *m);
QQuaternion calculateRotationQuaternion(const QVector3D &rotation,
                                        Q3DSNode::Orientation orientation);
void calculateGlobalProperties(const Q3DSNode *node, QVector3D &position,
                               QVector3D &rotation, QVector3D &scale);
QMatrix4x4 generateRotationMatrix(const QVector3D &nodeRotation, Q3DSNode::RotationOrder order);
QMatrix4x4 calculateCameraViewMatrix(const QMatrix4x4 &cameraWorldTransform);
QMatrix4x4 composeTransformMatrix(const Q3DSNode *node);
QPointF normalizePointToRect(const QPoint &inPoint, const QRectF &rect);
void flipZTranslation(QVector3D &vec);
QVector3D calcRay(const QPointF &point, const QMatrix4x4 &viewMatrix,
                  const QMatrix4x4 &projectionMatrix, QVector3D &outNearPos);
QVector3D mousePointToPlaneIntersection(const QPoint &mousePos, Q3DSCameraNode *cameraNode,
                                        Q3DSNode *node, const QVector3D &nodePosition,
                                        const QVector3D &planeNormal,
                                        bool globalIntersection);
QVector3D getXAxis(const QMatrix4x4 &matrix);
QVector3D getYAxis(const QMatrix4x4 &matrix);
QVector3D getZAxis(const QMatrix4x4 &matrix);

Q3DSModelNode *createWidgetModel(Q3DSUipPresentation *presentation, Q3DSGraphObject *parent,
                                 const QString &name, const QString &mesh,
                                 const QVector3D &scale, bool wireframe = false);
Q3DSCustomMaterialInstance *createWidgetCustomMaterial(Q3DSUipPresentation *presentation,
                                                       const QString &name,
                                                       const QString &materialData,
                                                       const QColor &color = Qt::white,
                                                       float opacity = 1.0f);
Q3DSDefaultMaterial *createWidgetDefaultMaterial(Q3DSUipPresentation *presentation,
                                                 const QString &name,
                                                 const QColor &color = Qt::white,
                                                 float opacity = 1.0f);
BoundingBox calculateLocalBoundingBox(Q3DSModelNode *model);
BoundingBox calculateBoundingBox(Q3DSGraphObject *graphObject);
void rotateBoundingBox(BoundingBox &box, const QQuaternion &rotation);
void calculateBoundingBoxOffsetAndSize(const Q3DSNode *node, const BoundingBox &boundingBox,
                                       QVector3D &outOffset, QVector3D &outSize);
}

#endif // Q3DSWIDGETUTILS_H
