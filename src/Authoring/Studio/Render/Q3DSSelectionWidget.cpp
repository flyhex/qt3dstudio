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

namespace Q3DStudio {

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

            QVector3D bbSize;
            QVector3D bbOffset;
            calculateBoundingBoxOffsetAndSize(node, boundingBox, bbOffset, bbSize);

            QVector3D scale = node->scale() * bbSize;
            if (qFuzzyIsNull(scale.x()))
                scale.setX(0.00000001f);
            if (qFuzzyIsNull(scale.y()))
                scale.setY(0.00000001f);
            if (qFuzzyIsNull(scale.z()))
                scale.setZ(0.00000001f);
            model->notifyPropertyChanges({
                model->setPosition(node->position() + bbOffset),
                model->setRotation(node->rotation()),
                model->setScale(node->scale() * bbSize),
                model->setRotationOrder(node->rotationOrder()),
                model->setOrientation(node->orientation()),
                model->setEyeballEnabled(true)
            });
        } else if (model->eyeballEnabled()) {
            model->notifyPropertyChanges({ model->setEyeballEnabled(false) });
        }
    }
}

}
