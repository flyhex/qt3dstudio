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

#include "Q3DSSelectionWidget.h"
#include "Q3DSWidgetUtils.h"
#include <QtCore/qmath.h>
#include <Qt3DCore/qtransform.h>
#include <Qt3DRender/qattribute.h>
#include <Qt3DRender/qbuffer.h>

namespace Q3DStudio {

Q3DSSelectionWidget::BoundingBox Q3DSSelectionWidget::calculateLocalBoundingBox(
        Q3DSModelNode *model)
{
    // TODO: Pre-calculate bounding boxes to prevent lag upon first selection
    const QString srcPath = model->sourcePath();
    if (m_boundingBoxCache.contains(srcPath))
        return m_boundingBoxCache[srcPath];

    BoundingBox box;

    const MeshList meshList = model->mesh();
    if (meshList.empty())
        return box;

    const auto boundingAttribute = meshList[0]->geometry()->boundingVolumePositionAttribute();
    const auto data = boundingAttribute->buffer()->data();
    const uint stride = boundingAttribute->byteStride();
    const uint offset = boundingAttribute->byteOffset();
    const uint size = boundingAttribute->vertexSize();
    const Qt3DRender::QAttribute::VertexBaseType type = boundingAttribute->vertexBaseType();

    if (type != Qt3DRender::QAttribute::VertexBaseType::Float || size != 3) {
        // Other types and sizes are not handled at the moment
        Q_ASSERT(false);
        return box;
    }

    for (uint i = 0; i < data.size(); i += stride) {
        uint index = i + offset;
        const float x = *reinterpret_cast<float *>(data.mid(index, 4).data());
        const float y = *reinterpret_cast<float *>(data.mid(index + 4, 4).data());
        const float z = *reinterpret_cast<float *>(data.mid(index + 8, 4).data());
        if (box.min.x() > x)
            box.min.setX(x);
        if (box.min.y() > y)
            box.min.setY(y);
        if (box.min.z() > z)
            box.min.setZ(z);

        if (box.max.x() < x)
            box.max.setX(x);
        if (box.max.y() < y)
            box.max.setY(y);
        if (box.max.z() < z)
            box.max.setZ(z);
    }

    m_boundingBoxCache[srcPath] = box;
    return box;
}

Q3DSSelectionWidget::BoundingBox Q3DSSelectionWidget::calculateBoundingBox(
        Q3DSGraphObject *graphObject)
{
    BoundingBox boundingBox;
    if (graphObject->type() == Q3DSNode::Type::Model) {
        boundingBox = calculateLocalBoundingBox(static_cast<Q3DSModelNode *>(graphObject));
    } else if (graphObject->childCount() == 0
               || graphObject->type() != Q3DSGraphObject::Type::Group) {
        boundingBox.min = QVector3D(0, 0, 0);
        boundingBox.max = QVector3D(0, 0, 0);
    }

    for (Q3DSGraphObject *child = graphObject->firstChild(); child != nullptr;
         child = child->nextSibling()) {
        Q3DSNode *childNode = static_cast<Q3DSNode *>(child);
        BoundingBox childBB = calculateBoundingBox(child);
        if ((childBB.max - childBB.min).length() > 0) {
            QVector3D rotation = childNode->rotation();
            QQuaternion quat;
            if (childNode->orientation() == Q3DSNode::Orientation::LeftHanded) {
                auto rotX = QQuaternion::fromAxisAndAngle(QVector3D(-1, 0, 0), rotation.x());
                auto rotY = QQuaternion::fromAxisAndAngle(QVector3D(0, -1, 0), rotation.y());
                auto rotZ = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), rotation.z());
                quat = rotY * rotX * rotZ;
            } else {
                auto rotX = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), rotation.x());
                auto rotY = QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), rotation.y());
                auto rotZ = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), rotation.z());
                quat = rotZ * rotY * rotX;
            }

            rotateBoundingBox(childBB, quat);

            const QVector3D scale = childNode->scale();
            childBB.min *= scale;
            childBB.max *= scale;

            QVector3D position = childNode->position();
            position.setZ(-position.z());
            childBB.min += position;
            childBB.max += position;

            if (childBB.min.x() < boundingBox.min.x())
                boundingBox.min.setX(childBB.min.x());
            if (childBB.min.y() < boundingBox.min.y())
                boundingBox.min.setY(childBB.min.y());
            if (childBB.min.z() < boundingBox.min.z())
                boundingBox.min.setZ(childBB.min.z());

            if (childBB.max.x() > boundingBox.max.x())
                boundingBox.max.setX(childBB.max.x());
            if (childBB.max.y() > boundingBox.max.y())
                boundingBox.max.setY(childBB.max.y());
            if (childBB.max.z() > boundingBox.max.z())
                boundingBox.max.setZ(childBB.max.z());
        }
    }

    return boundingBox;
}

void Q3DSSelectionWidget::rotateBoundingBox(BoundingBox &box, const QQuaternion &rotation)
{
    QVector3D points[8];
    points[0] = QVector3D(box.min.x(), box.min.y(), box.min.z());
    points[1] = QVector3D(box.max.x(), box.min.y(), box.min.z());
    points[2] = QVector3D(box.max.x(), box.max.y(), box.min.z());
    points[3] = QVector3D(box.min.x(), box.max.y(), box.min.z());
    points[4] = QVector3D(box.max.x(), box.max.y(), box.max.z());
    points[5] = QVector3D(box.min.x(), box.max.y(), box.max.z());
    points[6] = QVector3D(box.min.x(), box.min.y(), box.max.z());
    points[7] = QVector3D(box.max.x(), box.min.y(), box.max.z());
    box.min = QVector3D(std::numeric_limits<float>::max(),
                            std::numeric_limits<float>::max(),
                            std::numeric_limits<float>::max());
    box.max = QVector3D(-std::numeric_limits<float>::max(),
                            -std::numeric_limits<float>::max(),
                            -std::numeric_limits<float>::max());
    for (auto &point : points) {
        point = rotation * point;

        if (point.x() < box.min.x())
            box.min.setX(point.x());
        if (point.y() < box.min.y())
            box.min.setY(point.y());
        if (point.z() < box.min.z())
            box.min.setZ(point.z());

        if (point.x() > box.max.x())
            box.max.setX(point.x());
        if (point.y() > box.max.y())
            box.max.setY(point.y());
        if (point.z() > box.max.z())
            box.max.setZ(point.z());
    }
}


void Q3DSSelectionWidget::select(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer,
                                 Q3DSNode *node)
{
    for (auto &selection : qAsConst(m_selections)) {
        for (auto &model : qAsConst(selection.models))
            model->notifyPropertyChanges({ model->setEyeballEnabled(false) });
    }
    int index = 0;
    selectRecursive(presentation, layer, node, index, 0);
}

void Q3DSSelectionWidget::selectRecursive(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer,
                                          Q3DSNode *node, int &index, int depth)
{
    if (depth > 1)
        return;

    if (node->type() != Q3DSGraphObject::Type::Model
            && node->type() != Q3DSGraphObject::Type::Group) {
        return;
    }

    if (m_selections[layer].nodes.size() == index)
        m_selections[layer].nodes.resize(index + 1);
    m_selections[layer].nodes[index] = node;

    if (m_selections[layer].boundingBoxes.size() == index)
        m_selections[layer].boundingBoxes.resize(index + 1);
    m_selections[layer].boundingBoxes[index] = calculateBoundingBox(node);

    if (m_selections[layer].models.size() == index) {
        const QString name = QStringLiteral("StudioSelectionWidgetBoundingBox") + layer->id()
                + QString::number(m_selections[layer].nodes.size());
        auto model = createWidgetModel(presentation, layer, name,
                                       QStringLiteral(":/res/Selection.mesh"),
                                       QVector3D(1, 1, 1), true);
        auto material = createWidgetDefaultMaterial(presentation, name, Qt::red);
        model->appendChildNode(material);
        m_selections[layer].models.resize(index + 1);
        m_selections[layer].models[index] = model;
    }
    const auto model = m_selections[layer].models[index];
    model->notifyPropertyChanges({ model->setEyeballEnabled(true) });

    index++;

    for (Q3DSGraphObject *child = node->firstChild(); child != nullptr;
         child = child->nextSibling()) {
        selectRecursive(presentation, layer, static_cast<Q3DSNode *>(child), index, depth + 1);
    }
}

void Q3DSSelectionWidget::update()
{
    for (auto &selection : qAsConst(m_selections)) {
        for (int i = 0; i < selection.models.size(); ++i) {
            auto node = selection.nodes[i];
            auto model = selection.models[i];
            auto boundingBox = selection.boundingBoxes[i];

            QVector3D bbSize(qAbs(boundingBox.max.x() - boundingBox.min.x()),
                             qAbs(boundingBox.max.y() - boundingBox.min.y()),
                             qAbs(boundingBox.max.z() - boundingBox.min.z()));

            if (bbSize.length() > 0) {
                Q3DSNodeAttached *attached = node->attached<Q3DSNodeAttached>();
                if (!attached)
                    continue;

                Qt3DCore::QTransform transform;
                QMatrix4x4 globalMatrix = attached->globalTransform;
                globalMatrix.translate(boundingBox.min + bbSize / 2.0f);
                adjustRotationLeftToRight(&globalMatrix);
                transform.setMatrix(globalMatrix);

                QVector3D position = transform.translation();
                position.setZ(-position.z());

                Q3DSPropertyChangeList list;
                list.append(model->setPosition(position));
                list.append(model->setRotation(transform.rotation().toEulerAngles()));
                list.append(model->setScale(transform.scale() * bbSize * 0.5f));
                model->notifyPropertyChanges(list);
            }
        }
    }
}

}
