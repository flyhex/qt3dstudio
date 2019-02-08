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

namespace Q3DStudio {

bool Q3DSSelectionWidget::isCreated() const
{
    return m_models.size() > 0;
}

bool Q3DSSelectionWidget::isXAxis(Q3DSGraphObject *obj) const
{
    if (m_type == SelectionWidgetType::Rotation || m_models.size() < 6)
        return false;
    return obj == m_models[2];
}

bool Q3DSSelectionWidget::isYAxis(Q3DSGraphObject *obj) const
{
    if (m_type == SelectionWidgetType::Rotation || m_models.size() < 6)
        return false;
    return obj == m_models[0];
}

bool Q3DSSelectionWidget::isZAxis(Q3DSGraphObject *obj) const
{
    if (m_type == SelectionWidgetType::Rotation || m_models.size() < 6)
        return false;
    return obj == m_models[1];
}

bool Q3DSSelectionWidget::isXYPlane(Q3DSGraphObject *obj) const
{
    if (m_type == SelectionWidgetType::Rotation || m_models.size() < 6)
        return false;
    return obj == m_models[4];
}

bool Q3DSSelectionWidget::isYZPlane(Q3DSGraphObject *obj) const
{
    if (m_type == SelectionWidgetType::Rotation || m_models.size() < 6)
        return false;
    return obj == m_models[5];
}

bool Q3DSSelectionWidget::isZXPlane(Q3DSGraphObject *obj) const
{
    if (m_type == SelectionWidgetType::Rotation || m_models.size() < 6)
        return false;
    return obj == m_models[3];
}

bool Q3DSSelectionWidget::isXYCircle(Q3DSGraphObject *obj) const
{
    if (m_type != SelectionWidgetType::Rotation || m_models.size() < 4)
        return false;
    return obj == m_models[1];
}

bool Q3DSSelectionWidget::isYZCircle(Q3DSGraphObject *obj) const
{
    if (m_type != SelectionWidgetType::Rotation || m_models.size() < 4)
        return false;
    return obj == m_models[2];
}

bool Q3DSSelectionWidget::isZXCircle(Q3DSGraphObject *obj) const
{
    if (m_type != SelectionWidgetType::Rotation || m_models.size() < 4)
        return false;
    return obj == m_models[0];
}

bool Q3DSSelectionWidget::isCameraCircle(Q3DSGraphObject *obj) const
{
    if (m_type != SelectionWidgetType::Rotation || m_models.size() < 4)
        return false;
    return obj == m_models[3];
}

void Q3DSSelectionWidget::setScale(Q3DSGraphObject *obj, const QVector3D &scale)
{
    for (int i = 0; i < m_models.size(); ++i) {
        if (m_models[i] == obj) {
            m_scales[i] = scale;
            break;
        }
    }
}

void Q3DSSelectionWidget::resetScale(Q3DSGraphObject *obj)
{
    for (int i = 0; i < m_models.size(); ++i) {
        if (m_models[i] == obj) {
            m_scales[i] = QVector3D(1, 1, 1);
            break;
        }
    }
}

void Q3DSSelectionWidget::setColor(Q3DSGraphObject *obj, const QColor &color)
{
    for (int i = 0; i < m_models.size(); ++i) {
        if (m_models[i] == obj) {
            Q3DSPropertyChangeList colorChange = { Q3DSPropertyChange::fromVariant(
                                                   QStringLiteral("color"), color) };
            m_materials[i]->applyPropertyChanges(colorChange);
            m_materials[i]->notifyPropertyChanges(colorChange);
            break;
        }
    }
}

void Q3DSSelectionWidget::resetColor(Q3DSGraphObject *obj)
{
    for (int i = 0; i < m_models.size(); ++i) {
        if (m_models[i] == obj) {
            Q3DSPropertyChangeList colorChange = { Q3DSPropertyChange::fromVariant(
                                                   QStringLiteral("color"), m_colors[i]) };
            m_materials[i]->applyPropertyChanges(colorChange);
            m_materials[i]->notifyPropertyChanges(colorChange);
            break;
        }
    }
}

void Q3DSSelectionWidget::setEyeballEnabled(bool value)
{
    for (auto &model : qAsConst(m_models)) {
        Q3DSPropertyChangeList list;
        list.append(model->setEyeballEnabled(value));
        model->notifyPropertyChanges(list);
    }
}

void Q3DSSelectionWidget::createModel(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer,
                                      const QString &name, const QString &mesh,
                                      const QColor &color, const QVector3D &scale)
{
    const QString widgetMaterial = QStringLiteral(
        "<MetaData>\n"
        "<Property name=\"color\" type=\"Color\" default=\"1.0 0.0 0.0\" stage=\"fragment\" />\n"
        "</MetaData>\n"
        "<Shaders type=\"GLSL\" version=\"330\">\n"
        "<Shader>\n"
        "<VertexShader>\n"
        "attribute vec3 attr_pos;\n"
        "uniform mat4 modelViewProjection;\n"
        "void main() {\n"
        "gl_Position = modelViewProjection * vec4(attr_pos, 1.0);\n"
        "</VertexShader>\n"
        "<FragmentShader>\n"
        "void main() {\n"
        "fragOutput = vec4(color, 1.0);\n"
        "</FragmentShader>\n"
        "</Shader>\n"
        "</Shaders>\n"
        "<Passes><Pass></Pass></Passes>\n");

    auto material = createWidgetCustomMaterial(presentation, name, widgetMaterial, color);
    if (material) {
        m_materials.append(material);
        m_models.append(createWidgetModel(presentation, layer, name, mesh, scale));
        m_models.back()->appendChildNode(material);
        m_colors.append(color);
        m_scales.append(QVector3D(1, 1, 1));
    }
}

void Q3DSSelectionWidget::create(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer,
                                 SelectionWidgetType type)
{
    if (isCreated())
        return;

    m_type = type;
    if (m_type == SelectionWidgetType::Translation) {
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetGreenPointer"),
                    QStringLiteral(":/res/GreenArrow.mesh"),
                    QColor(Qt::green), QVector3D(50, 50, 50));
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetBluePointer"),
                    QStringLiteral(":/res/BlueArrow.mesh"),
                    QColor(Qt::blue), QVector3D(50, 50, 50));
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetRedPointer"),
                    QStringLiteral(":/res/RedArrow.mesh"),
                    QColor(Qt::red), QVector3D(50, 50, 50));
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetGreenCircle"),
                    QStringLiteral(":/res/GreenCircle.mesh"),
                    QColor(Qt::green), QVector3D(50, 50, 50));
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetBlueCircle"),
                    QStringLiteral(":/res/BlueCircle.mesh"),
                    QColor(Qt::blue), QVector3D(50, 50, 50));
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetRedCircle"),
                    QStringLiteral(":/res/RedCircle.mesh"),
                    QColor(Qt::red), QVector3D(50, 50, 50));
    } else if (m_type == SelectionWidgetType::Rotation) {
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetGreenCircle"),
                    QStringLiteral(":/res/GreenRotation.mesh"),
                    QColor(Qt::green), QVector3D(50, 50, 50));
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetBlueCircle"),
                    QStringLiteral(":/res/BlueRotation.mesh"),
                    QColor(Qt::blue), QVector3D(50, 50, 50));
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetRedCircle"),
                    QStringLiteral(":/res/RedRotation.mesh"),
                    QColor(Qt::red), QVector3D(50, 50, 50));
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetGrayCircle"),
                    QStringLiteral(":/res/BlueRotation.mesh"),
                    QColor(Qt::gray), QVector3D(50, 50, 50));
    } else if (m_type == SelectionWidgetType::Scale) {
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetGreenPointer"),
                    QStringLiteral(":/res/GreenScale.mesh"),
                    QColor(Qt::green), QVector3D(50, 50, 50));
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetBluePointer"),
                    QStringLiteral(":/res/BlueScale.mesh"),
                    QColor(Qt::blue), QVector3D(50, 50, 50));
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetRedPointer"),
                    QStringLiteral(":/res/RedScale.mesh"),
                    QColor(Qt::red), QVector3D(50, 50, 50));
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetGreenCircle"),
                    QStringLiteral(":/res/GreenCircle.mesh"),
                    QColor(Qt::green), QVector3D(50, 50, 50));
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetBlueCircle"),
                    QStringLiteral(":/res/BlueCircle.mesh"),
                    QColor(Qt::blue), QVector3D(50, 50, 50));
        createModel(presentation, layer,
                    QStringLiteral("StudioTranslationWidgetRedCircle"),
                    QStringLiteral(":/res/RedCircle.mesh"),
                    QColor(Qt::red), QVector3D(50, 50, 50));
    }
}

void Q3DSSelectionWidget::destroy(Q3DSUipPresentation *presentation)
{
    for (auto &model : qAsConst(m_models)) {
        presentation->masterSlide()->removeObject(model);
        presentation->unlinkObject(model);
        delete model;
    }

    m_models.clear();
    m_materials.clear();
    m_colors.clear();
}

void applyNodeProperties(Q3DSGraphObject *node, Q3DSCameraNode *camera, Q3DSLayerNode *layer,
                         const QSize &size, Q3DSModelNode *model, const QVector3D &modelScale) {
    Q3DSNodeAttached *attached = node->attached<Q3DSNodeAttached>();
    if (!attached)
        return;

    Qt3DCore::QTransform transform;
    QMatrix4x4 globalMatrix = attached->globalTransform;
    adjustRotationLeftToRight(&globalMatrix);
    transform.setMatrix(globalMatrix);

    QVector3D position = transform.translation();
    position.setZ(-position.z());

    Q3DSPropertyChangeList list;
    list.append(model->setPosition(position));
    list.append(model->setRotation(transform.rotation().toEulerAngles()));

    float distance = 400.0f;
    float fovScale = 2.0f;
    if (!camera->orthographic()) {
        distance = camera->position().distanceToPoint(position);
        fovScale = 2.0f * float(qTan(qDegreesToRadians(qreal(camera->fov())) / 2.0));
    }
    float scale = 125.0f * camera->zoom() * fovScale;
    float width = size.width();
    float height = size.height();
    if (layer->widthUnits() == Q3DSLayerNode::Units::Percent) {
        width *= layer->width() * 0.01f;
        height *= layer->height() * 0.01f;
    } else {
        width = layer->width();
        height = layer->height();
    }
    float length = qSqrt(width * width + height * height);
    scale /= length;
    list.append(model->setScale(modelScale * scale * distance));
    list.append(model->setEyeballEnabled(true));
    model->notifyPropertyChanges(list);
}

void Q3DSSelectionWidget::applyProperties(Q3DSGraphObject *node, Q3DSCameraNode *camera,
                                          Q3DSLayerNode *layer, const QSize &size)
{
    for (int i = 0; i < m_models.size(); ++i) {
        if (node->type() == Q3DSGraphObject::Model
                || node->type() == Q3DSGraphObject::Alias
                || node->type() == Q3DSGraphObject::Group
                || node->type() == Q3DSGraphObject::Light
                || node->type() == Q3DSGraphObject::Camera
                || node->type() == Q3DSGraphObject::Text
                || node->type() == Q3DSGraphObject::Component) {
            applyNodeProperties(node, camera, layer, size, m_models[i], m_scales[i]);
        }
    }

    if (m_type == SelectionWidgetType::Rotation && m_models.size() >= 4) {
        // The fourth model of the rotation widget has to look towards the camera
        QQuaternion rotation = QQuaternion::fromDirection(
                    (camera->position() - m_models[3]->position()).normalized(),
                    QVector3D(0, 1, 0));

        Q3DSPropertyChangeList list;
        list.append(m_models[3]->setRotation(rotation.toEulerAngles()));
        m_models[3]->notifyPropertyChanges(list);
    }
}

}
