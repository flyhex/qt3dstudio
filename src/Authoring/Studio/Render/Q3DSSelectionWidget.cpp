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
        const float z = -*reinterpret_cast<float *>(data.mid(index + 8, 4).data());
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
        if (!child->isNode())
            continue;

        Q3DSNode *childNode = static_cast<Q3DSNode *>(child);
        BoundingBox childBB = calculateBoundingBox(child);
        if ((childBB.max - childBB.min).length() > 0) {
            const QVector3D scale = childNode->scale();
            childBB.min *= scale;
            childBB.max *= scale;

            rotateBoundingBox(childBB, calculateRotationQuaternion(childNode->rotation(),
                                                                   childNode->orientation()));

            const QVector3D position = childNode->position();
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


void Q3DSSelectionWidget::select(Q3DSUipPresentation *presentation, Q3DSNode *node)
{
    m_presentation = presentation;
    deselect();
    int index = 0;
    selectRecursive(presentation, node, index, 0);
}

void Q3DSSelectionWidget::selectRecursive(Q3DSUipPresentation *presentation, Q3DSNode *node,
                                          int &index, int depth)
{
    if (depth > 1)
        return;

    if (node->type() != Q3DSGraphObject::Type::Model
            && node->type() != Q3DSGraphObject::Type::Group) {
        return;
    }

    m_selections[node].node = node;
    m_selections[node].boundingBox = calculateBoundingBox(node);

    if (!m_selections[node].model) {
        const QString name = QStringLiteral("StudioSelectionWidgetBoundingBox") + node->id();

        auto model = createWidgetModel(presentation, node->parent(), name,
                                       QStringLiteral(":/res/Selection.mesh"),
                                       QVector3D(1, 1, 1), true);

        auto material = createWidgetDefaultMaterial(presentation, name, Qt::red);
        model->appendChildNode(material);
        m_selections[node].model = model;
    }
    m_selections[node].visible = true;

    index++;

    for (Q3DSGraphObject *child = node->firstChild(); child != nullptr;
         child = child->nextSibling()) {
        if (!child->id().contains("StudioSelectionWidget"))
            selectRecursive(presentation, static_cast<Q3DSNode *>(child), index, depth + 1);
    }
}

void Q3DSSelectionWidget::deselect()
{
    for (auto &selection : m_selections)
        selection.visible = false;
}

void Q3DSSelectionWidget::update()
{
    for (auto &selection : qAsConst(m_selections)) {
        const auto node = selection.node;
        const auto model = selection.model;

        if (selection.visible) {
            const auto boundingBox = selection.boundingBox;

            QVector3D bbSize(qAbs(boundingBox.max.x() - boundingBox.min.x()),
                             qAbs(boundingBox.max.y() - boundingBox.min.y()),
                             qAbs(boundingBox.max.z() - boundingBox.min.z()));

            if (bbSize.length() > 0) {
                QMatrix4x4 rotMat = generateRotationMatrix(node->rotation(),
                                                           node->rotationOrder());
                if (node->orientation() == Q3DSNode::RightHanded)
                    adjustRotationLeftToRight(&rotMat);
                QVector3D offset = rotMat * (boundingBox.min + bbSize * 0.5f) * node->scale();

                model->notifyPropertyChanges({
                    model->setPosition(node->position() + offset),
                    model->setRotation(node->rotation()),
                    model->setScale(node->scale() * bbSize * 0.5f),
                    model->setRotationOrder(node->rotationOrder()),
                    model->setOrientation(node->orientation()),
                    model->setEyeballEnabled(true)
                });
            }
        } else if (model->eyeballEnabled()) {
            model->notifyPropertyChanges({ model->setEyeballEnabled(false) });
        }
    }
}

}
