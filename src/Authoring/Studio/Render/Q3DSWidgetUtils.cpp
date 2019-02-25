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
