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

#include "q3dsruntime2api_p.h"
#include "qmath.h"
#include "Qt3DCore/qtransform.h"
#include <Qt3DRender/qcamera.h>

namespace Q3DStudio {

void adjustRotationLeftToRight(QMatrix4x4 *m)
{
    float *p = m->data();
    p[2] *= -1;
    p[6] *= -1;
    p[8] *= -1;
    p[9] *= -1;
}

QQuaternion calculateRotationQuaternion(const QVector3D &rotation,
                                        Q3DSNode::Orientation orientation)
{
    if (orientation == Q3DSNode::Orientation::LeftHanded)
        return QQuaternion::fromEulerAngles(rotation);

    auto rotX = QQuaternion::fromAxisAndAngle(QVector3D(-1, 0, 0), rotation.x());
    auto rotY = QQuaternion::fromAxisAndAngle(QVector3D(0, -1, 0), rotation.y());
    auto rotZ = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), rotation.z());
    return rotZ * rotY * rotX;
}

// Copied from q3dsscenemanager.cpp
QMatrix4x4 generateRotationMatrix(const QVector3D &nodeRotation, Q3DSNode::RotationOrder order)
{
    QMatrix4x4 rotationMatrix;
    switch (order) {
    case Q3DSNode::XYZ:
    case Q3DSNode::ZYXr:
        rotationMatrix.rotate(nodeRotation.x(), QVector3D(1, 0, 0));
        rotationMatrix.rotate(nodeRotation.y(), QVector3D(0, 1, 0));
        rotationMatrix.rotate(nodeRotation.z(), QVector3D(0, 0, 1));
        break;
    case Q3DSNode::XYZr: // XYZr is what the editor outputs in practice
    case Q3DSNode::ZYX:
        rotationMatrix.rotate(nodeRotation.z(), QVector3D(0, 0, 1));
        rotationMatrix.rotate(nodeRotation.y(), QVector3D(0, 1, 0));
        rotationMatrix.rotate(nodeRotation.x(), QVector3D(1, 0, 0));
        break;
    case Q3DSNode::YZX:
    case Q3DSNode::XZYr:
        rotationMatrix.rotate(nodeRotation.y(), QVector3D(0, 1, 0));
        rotationMatrix.rotate(nodeRotation.z(), QVector3D(0, 0, 1));
        rotationMatrix.rotate(nodeRotation.x(), QVector3D(1, 0, 0));
        break;
    case Q3DSNode::ZXY:
    case Q3DSNode::YXZr:
        rotationMatrix.rotate(nodeRotation.z(), QVector3D(0, 0, 1));
        rotationMatrix.rotate(nodeRotation.x(), QVector3D(1, 0, 0));
        rotationMatrix.rotate(nodeRotation.y(), QVector3D(0, 1, 0));
        break;
    case Q3DSNode::XZY:
    case Q3DSNode::YZXr:
        rotationMatrix.rotate(nodeRotation.x(), QVector3D(1, 0, 0));
        rotationMatrix.rotate(nodeRotation.z(), QVector3D(0, 0, 1));
        rotationMatrix.rotate(nodeRotation.y(), QVector3D(0, 1, 0));
        break;
    case Q3DSNode::YXZ:
    case Q3DSNode::ZXYr:
        rotationMatrix.rotate(nodeRotation.y(), QVector3D(0, 1, 0));
        rotationMatrix.rotate(nodeRotation.x(), QVector3D(1, 0, 0));
        rotationMatrix.rotate(nodeRotation.z(), QVector3D(0, 0, 1));
        break;
    default:
        break;
    }
    return rotationMatrix;
}

// Need custom calculation for camera view matrix to properly handle top/bottom edit cameras,
// where default upvector doesn't work.
QMatrix4x4 calculateCameraViewMatrix(const QMatrix4x4 &cameraWorldTransform)
{
    const QVector4D position = cameraWorldTransform * QVector4D(0.0f, 0.0f, 0.0f, 1.0f);
    const QVector4D viewDirection = cameraWorldTransform * QVector4D(0.0f, 0.0f, -1.0f, 0.0f);
    const QVector4D upVector = cameraWorldTransform * QVector4D(0.0f, 1.0f, 0.0f, 0.0f);

    QMatrix4x4 m;
    m.lookAt(QVector3D(position),
             QVector3D(position + viewDirection),
             QVector3D(upVector));
    return QMatrix4x4(m);
}

// Mostly copied from setNodeProperties in q3dsscenemanager.cpp
QMatrix4x4 composeTransformMatrix(const Q3DSNode *node)
{
    const bool leftHanded = node->orientation() == Q3DSNode::LeftHanded;
    QMatrix4x4 rot = generateRotationMatrix(node->rotation(), node->rotationOrder());

    QMatrix4x4 m;
    float *mp = m.data();
    const QVector3D pos = node->position();
    const QVector3D scale = node->scale();
    const QVector3D scaledPivot = -node->pivot() * scale;
    mp[0] = scale.x();
    mp[5] = scale.y();
    mp[10] = scale.z();
    mp[12] = scaledPivot.x();
    mp[13] = scaledPivot.y();
    mp[14] = leftHanded ? scaledPivot.z() : -scaledPivot.z();
    m = rot * m;
    mp[12] += pos.x();
    mp[13] += pos.y();
    mp[14] += leftHanded ? pos.z() : -pos.z();
    if (leftHanded) {
        adjustRotationLeftToRight(&m);
        mp[14] *= -1;
    }

    if (node->type() == Q3DSGraphObject::Text)
        m.rotate(90, 1, 0, 0); // adjust for QPlaneMesh's X-Z default

    return m;
}

void calculateGlobalProperties(const Q3DSNode *node, QVector3D &position,
                               QVector3D &rotation, QVector3D &scale)
{
    QMatrix4x4 matrix = composeTransformMatrix(node);
    auto parent = static_cast<Q3DSNode *>(node->parent());
    while (parent) {
        if (!parent->isNode())
            break;
        QMatrix4x4 parentMatrix = composeTransformMatrix(parent);
        matrix = parentMatrix * matrix;
        parent = static_cast<Q3DSNode *>(parent->parent());
    }

    Qt3DCore::QTransform transform;
    transform.setMatrix(matrix);
    position = transform.translation();
    position.setZ(-position.z());
    rotation = QVector3D(-transform.rotationX(), -transform.rotationY(), transform.rotationZ());
    scale = transform.scale3D();
}

QPointF normalizePointToRect(const QPoint &inPoint, const QRectF &rect)
{
    qreal x = qreal(inPoint.x() - rect.x());
    qreal y = qreal(inPoint.y() - rect.y());
    x = x / rect.width() * 2 - 1;
    y = y / rect.height() * 2 - 1;
    // OpenGL has inverted Y
    y = -y;

    return QPointF(x, y);
}

// Qt3D and editor have mirrored Z-axes. This function can be used to convert coordinates
// between the two systems.
void flipZTranslation(QVector3D &vec)
{
    vec.setZ(-vec.z());
}

// Returns the intersection point of a plane and a ray.
// Parameter t returns the distance in ray lengths. If t is negative, intersection
// is behind rayOrigin.
// If there is no intersection, i.e. plane and the ray are parallel, t is set to -1 and
// rayOrigin is returned.
static QVector3D findIntersection(const QVector3D &rayOrigin, const QVector3D &ray,
                                  float planeOffset, const QVector3D &planeNormal, float &t)
{
    float divisor = QVector3D::dotProduct(ray, planeNormal);
    if (qFuzzyCompare(1.0f, 1.0f + divisor)) {
        t = -1.0f;
        return rayOrigin;
    }

    t = -(QVector3D::dotProduct(rayOrigin, planeNormal) - planeOffset) / divisor;

    return rayOrigin + ray * t;
}

// A copy of QVector3D::unproject with the difference that if obj.w() is nearly zero, we don't
// set it to one as that is extremely wrong, at least for our purposes.
// For determining plane intersections, nearly zero values are good enough even if they may not
// result in completely pixel-accurate positions.
// This allows much larger far clip values to be used in cameras before things break.
static QVector3D unproject(const QVector3D &vector,
                           const QMatrix4x4 &modelView, const QMatrix4x4 &projection,
                           const QRect &viewport)
{
    QMatrix4x4 inverse = QMatrix4x4( projection * modelView ).inverted();

    QVector4D tmp(vector, 1.0f);
    tmp.setX((tmp.x() - float(viewport.x())) / float(viewport.width()));
    tmp.setY((tmp.y() - float(viewport.y())) / float(viewport.height()));
    tmp = tmp * 2.0f - QVector4D(1.0f, 1.0f, 1.0f, 1.0f);

    QVector4D obj = inverse * tmp;
    // Don't change the w unless it is actually zero
    if (obj.w() == 0.f)
        obj.setW(0.000000001f);
    obj /= obj.w();

    return obj.toVector3D();
}

QVector3D calcRay(const QPointF &point, const QMatrix4x4 &viewMatrix,
                  const QMatrix4x4 &projectionMatrix, QVector3D &outNearPos)
{
    QRect viewPort(-1, -1, 2, 2);
    outNearPos = QVector3D(float(point.x()), float(point.y()), 0.0f);
    outNearPos = unproject(outNearPos, viewMatrix, projectionMatrix, viewPort);
    QVector3D farPos(float(point.x()), float(point.y()), 1.0f);
    farPos = unproject(farPos, viewMatrix, projectionMatrix, viewPort);

    QVector3D ray = (farPos - outNearPos).normalized();

    return ray;
}

// Calculates the intersection of a ray through camera position and mouse point with
// the defined plane that goes through the given node if it has
// the given local position (which can be different than node's actual position).
// If globalIntersection is true, the value is returned in global 3D space coordinates.
// Otherwise local coordinates in editor space (i.e. Z flipped) are returned.
QVector3D mousePointToPlaneIntersection(const QPoint &mousePos,
                                        Q3DSCameraNode *cameraNode,
                                        Q3DSNode *node,
                                        const QVector3D &nodePosition,
                                        const QVector3D &planeNormal,
                                        bool globalIntersection)
{
    Q3DSNode *parentNode = static_cast<Q3DSNode *>(node->parent());
    Q3DSCameraAttached *cameraAttached = cameraNode->attached<Q3DSCameraAttached>();
    Q3DSNodeAttached *nodeAttached = node->attached<Q3DSNodeAttached>();
    Q3DSNodeAttached *parentAttached = parentNode->attached<Q3DSNodeAttached>();
    Q3DSLayerAttached *layerAttached = nodeAttached->layer3DS->attached<Q3DSLayerAttached>();

    QMatrix4x4 parentMatrix = parentAttached->globalTransform;
    QMatrix4x4 cameraMatrix = cameraAttached->globalTransform;

    QRectF layerRect = QRectF(layerAttached->layerPos, layerAttached->layerSize);
    QPointF newPoint = normalizePointToRect(mousePos, layerRect);

    auto viewMatrix = calculateCameraViewMatrix(cameraMatrix);
    auto projectionMatrix = cameraAttached->camera->projectionMatrix();

    QVector3D cameraPos = cameraNode->position();
    flipZTranslation(cameraPos);

    QVector3D nearPos;
    QVector3D newRay = calcRay(newPoint, viewMatrix, projectionMatrix, nearPos);

    // Find intersections of newRay and oldRay on camera plane that goes through oldPos
    // Operations are done in Qt3D space, i.e. Z-axis flipped
    QVector3D beginPos = nodePosition;
    flipZTranslation(beginPos);
    QVector3D nodeWorldPos = parentMatrix * beginPos;
    float distance = -1.f;
    float cosAngle = QVector3D::dotProduct(nodeWorldPos.normalized(), planeNormal);
    float planeOffset = nodeWorldPos.length() * cosAngle;

    QVector3D intersect = findIntersection(nearPos, newRay, planeOffset, planeNormal, distance);

    if (!globalIntersection) {
        intersect = parentMatrix.inverted() * intersect;
        flipZTranslation(intersect); // Flip back to editor coords
    }

    return intersect;
}


// Pulls the normalized 1st column out of the global transform.
QVector3D getXAxis(const QMatrix4x4 &matrix)
{
    const float *data = matrix.data();
    QVector3D retval(data[0], data[1], data[2]);
    retval.normalize();
    return retval;
}

// Pulls the normalized 2nd column out of the global transform.
QVector3D getYAxis(const QMatrix4x4 &matrix)
{
    const float *data = matrix.data();
    QVector3D retval(data[4], data[5], data[6]);
    retval.normalize();
    return retval;
}

// Pulls the normalized 3rd column out of the global transform.
QVector3D getZAxis(const QMatrix4x4 &matrix)
{
    const float *data = matrix.data();
    QVector3D retval(data[8], data[9], data[10]);
    retval.normalize();
    return retval;
}

Q3DSModelNode *createWidgetModel(Q3DSUipPresentation *presentation, Q3DSGraphObject *parent,
                                 const QString &name, const QString &mesh,
                                 const QVector3D &scale, bool wireframe)
{
    Q3DSModelNode *model = presentation->newObject<Q3DSModelNode>(
                (name + QLatin1Char('_')).toUtf8().constData());
    parent->appendChildNode(model);
    presentation->masterSlide()->addObject(model);

    model->setMesh(mesh);
    model->resolveReferences(*presentation);

    if (wireframe) {
        MeshList meshList = model->mesh();
        if (!meshList.empty())
            meshList[0]->setPrimitiveType(Qt3DRender::QGeometryRenderer::PrimitiveType::LineLoop);
    }

    Q3DSPropertyChangeList list;
    list.append(model->setScale(scale));
    model->notifyPropertyChanges(list);

    return model;
}

Q3DSCustomMaterialInstance *createWidgetCustomMaterial(Q3DSUipPresentation *presentation,
                                                       const QString &name,
                                                       const QString &materialData,
                                                       const QColor &color,
                                                       float opacity)
{
    const QByteArray matName = (name + QLatin1String("Material_")).toUtf8();
    const QByteArray matId = '#' + matName;

    const QString matData = QLatin1String("<Material name=\"") + matName
            + QLatin1String("\" version=\"1.0\">\n") + materialData
            + QLatin1String("</Material>\n");

    Q3DSCustomMaterial material = presentation->customMaterial(matId, matData.toUtf8());
    if (!material.isNull()) {
        Q3DSCustomMaterialInstance *customMat
                = presentation->newObject<Q3DSCustomMaterialInstance>(matId);
        customMat->setSourcePath(matId);
        customMat->resolveReferences(*presentation);

        Q3DSPropertyChangeList propChanges = { Q3DSPropertyChange::fromVariant(
                                               QStringLiteral("color"), color),
                                               Q3DSPropertyChange::fromVariant(
                                               QStringLiteral("opacity"), opacity) };
        customMat->applyPropertyChanges(propChanges);
        customMat->notifyPropertyChanges(propChanges);

        return customMat;
    }
    return nullptr;
}

Q3DSDefaultMaterial *createWidgetDefaultMaterial(Q3DSUipPresentation *presentation,
                                                 const QString &name,
                                                 const QColor &color,
                                                 float opacity)
{
    const QByteArray matId = (name + QLatin1String("Material_")).toUtf8();

    Q3DSPropertyChangeList list;
    Q3DSDefaultMaterial *defMat = presentation->newObject<Q3DSDefaultMaterial>(matId);
    list.append(defMat->setDiffuse(color));
    list.append(defMat->setOpacity(opacity * 100.0f));
    list.append(defMat->setShaderLighting(Q3DSDefaultMaterial::ShaderLighting::NoShaderLighting));
    defMat->notifyPropertyChanges(list);
    return defMat;
}

}
