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

#include "Q3DSManipulationWidget.h"
#include "Q3DSWidgetUtils.h"
#include <QtCore/qmath.h>
#include <Qt3DCore/qtransform.h>
#include "StudioPreferences.h"

namespace Q3DStudio {

bool Q3DSManipulationWidget::hasManipulators() const
{
    return m_manipulators.size() > 0;
}

bool Q3DSManipulationWidget::isXAxis(Q3DSGraphObject *obj) const
{
    if (m_type == ManipulationWidgetType::Rotation || m_manipulators.size() < 6)
        return false;
    return obj == m_manipulators[2];
}

bool Q3DSManipulationWidget::isYAxis(Q3DSGraphObject *obj) const
{
    if (m_type == ManipulationWidgetType::Rotation || m_manipulators.size() < 6)
        return false;
    return obj == m_manipulators[0];
}

bool Q3DSManipulationWidget::isZAxis(Q3DSGraphObject *obj) const
{
    if (m_type == ManipulationWidgetType::Rotation || m_manipulators.size() < 6)
        return false;
    return obj == m_manipulators[1];
}

bool Q3DSManipulationWidget::isXYPlane(Q3DSGraphObject *obj) const
{
    if (m_type == ManipulationWidgetType::Rotation || m_manipulators.size() < 6)
        return false;
    return obj == m_manipulators[4];
}

bool Q3DSManipulationWidget::isYZPlane(Q3DSGraphObject *obj) const
{
    if (m_type == ManipulationWidgetType::Rotation || m_manipulators.size() < 6)
        return false;
    return obj == m_manipulators[5];
}

bool Q3DSManipulationWidget::isZXPlane(Q3DSGraphObject *obj) const
{
    if (m_type == ManipulationWidgetType::Rotation || m_manipulators.size() < 6)
        return false;
    return obj == m_manipulators[3];
}

bool Q3DSManipulationWidget::isXYCircle(Q3DSGraphObject *obj) const
{
    if (m_type != ManipulationWidgetType::Rotation || m_manipulators.size() < 4)
        return false;
    return obj == m_manipulators[1];
}

bool Q3DSManipulationWidget::isYZCircle(Q3DSGraphObject *obj) const
{
    if (m_type != ManipulationWidgetType::Rotation || m_manipulators.size() < 4)
        return false;
    return obj == m_manipulators[2];
}

bool Q3DSManipulationWidget::isZXCircle(Q3DSGraphObject *obj) const
{
    if (m_type != ManipulationWidgetType::Rotation || m_manipulators.size() < 4)
        return false;
    return obj == m_manipulators[0];
}

bool Q3DSManipulationWidget::isCameraCircle(Q3DSGraphObject *obj) const
{
    if (m_type != ManipulationWidgetType::Rotation || m_manipulators.size() < 4)
        return false;
    return obj == m_manipulators[3];
}

void Q3DSManipulationWidget::setScale(Q3DSGraphObject *obj, const QVector3D &scale)
{
    for (int i = 0; i < m_manipulators.size(); ++i) {
        if (m_manipulators[i] == obj) {
            m_manipulatorScales[i] = scale;
            break;
        }
    }
}

void Q3DSManipulationWidget::resetScale(Q3DSGraphObject *obj)
{
    for (int i = 0; i < m_manipulators.size(); ++i) {
        if (m_manipulators[i] == obj) {
            m_manipulatorScales[i] = QVector3D(1, 1, 1);
            break;
        }
    }
}

void Q3DSManipulationWidget::setColor(Q3DSGraphObject *obj, const QColor &color)
{
    for (int i = 0; i < m_manipulators.size(); ++i) {
        if (m_manipulators[i] == obj) {
            Q3DSPropertyChangeList colorChange = { Q3DSPropertyChange::fromVariant(
                                                   QStringLiteral("color"), color) };
            m_manipulatorMaterials[i]->applyPropertyChanges(colorChange);
            m_manipulatorMaterials[i]->notifyPropertyChanges(colorChange);
            break;
        }
    }
}

void Q3DSManipulationWidget::resetColor(Q3DSGraphObject *obj)
{
    for (int i = 0; i < m_manipulators.size(); ++i) {
        if (m_manipulators[i] == obj) {
            Q3DSPropertyChangeList colorChange = { Q3DSPropertyChange::fromVariant(
                                                   QStringLiteral("color"),
                                                   m_manipulatorColors[i]) };
            m_manipulatorMaterials[i]->applyPropertyChanges(colorChange);
            m_manipulatorMaterials[i]->notifyPropertyChanges(colorChange);
            break;
        }
    }
}

void Q3DSManipulationWidget::setEyeballEnabled(bool value)
{
    for (auto &model : qAsConst(m_manipulators)) {
        Q3DSPropertyChangeList list;
        list.append(model->setEyeballEnabled(value));
        model->notifyPropertyChanges(list);
    }
}

void Q3DSManipulationWidget::createManipulator(Q3DSUipPresentation *presentation,
                                               Q3DSLayerNode *layer, const QString &name,
                                               const QString &mesh, const QColor &color,
                                               const QVector3D &scale)
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
        m_manipulatorMaterials.append(material);
        m_manipulators.append(createWidgetModel(presentation, layer, name, mesh, scale));
        m_manipulators.back()->appendChildNode(material);
        m_manipulatorColors.append(color);
        m_manipulatorScales.append(QVector3D(1, 1, 1));
    }
}

void Q3DSManipulationWidget::createManipulators(Q3DSUipPresentation *presentation,
                                                Q3DSLayerNode *layer, ManipulationWidgetType type)
{
    if (hasManipulators())
        return;

    m_type = type;
    m_presentation = presentation;
    if (m_type == ManipulationWidgetType::Translation) {
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetGreenPointer"),
                          QStringLiteral(":/res/GreenArrow.mesh"),
                          CStudioPreferences::GetYAxisColor(), QVector3D(50, 50, 50));
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetBluePointer"),
                          QStringLiteral(":/res/BlueArrow.mesh"),
                          CStudioPreferences::GetZAxisColor(), QVector3D(50, 50, 50));
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetRedPointer"),
                          QStringLiteral(":/res/RedArrow.mesh"),
                          CStudioPreferences::GetXAxisColor(), QVector3D(50, 50, 50));
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetGreenCircle"),
                          QStringLiteral(":/res/GreenCircle.mesh"),
                          CStudioPreferences::GetYAxisColor(), QVector3D(50, 50, 50));
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetBlueCircle"),
                          QStringLiteral(":/res/BlueCircle.mesh"),
                          CStudioPreferences::GetZAxisColor(), QVector3D(50, 50, 50));
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetRedCircle"),
                          QStringLiteral(":/res/RedCircle.mesh"),
                          CStudioPreferences::GetXAxisColor(), QVector3D(50, 50, 50));
    } else if (m_type == ManipulationWidgetType::Rotation) {
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetGreenCircle"),
                          QStringLiteral(":/res/GreenRotation.mesh"),
                          CStudioPreferences::GetYAxisColor(), QVector3D(50, 50, 50));
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetBlueCircle"),
                          QStringLiteral(":/res/BlueRotation.mesh"),
                          CStudioPreferences::GetZAxisColor(), QVector3D(50, 50, 50));
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetRedCircle"),
                          QStringLiteral(":/res/RedRotation.mesh"),
                          CStudioPreferences::GetXAxisColor(), QVector3D(50, 50, 50));
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetGrayCircle"),
                          QStringLiteral(":/res/BlueRotation.mesh"),
                          QColor(Qt::gray), QVector3D(50, 50, 50));
    } else if (m_type == ManipulationWidgetType::Scale) {
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetGreenPointer"),
                          QStringLiteral(":/res/GreenScale.mesh"),
                          CStudioPreferences::GetYAxisColor(), QVector3D(50, 50, 50));
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetBluePointer"),
                          QStringLiteral(":/res/BlueScale.mesh"),
                          CStudioPreferences::GetZAxisColor(), QVector3D(50, 50, 50));
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetRedPointer"),
                          QStringLiteral(":/res/RedScale.mesh"),
                          CStudioPreferences::GetXAxisColor(), QVector3D(50, 50, 50));
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetGreenCircle"),
                          QStringLiteral(":/res/GreenCircle.mesh"),
                          CStudioPreferences::GetYAxisColor(), QVector3D(50, 50, 50));
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetBlueCircle"),
                          QStringLiteral(":/res/BlueCircle.mesh"),
                          CStudioPreferences::GetZAxisColor(), QVector3D(50, 50, 50));
        createManipulator(presentation, layer,
                          QStringLiteral("StudioManipulationWidgetRedCircle"),
                          QStringLiteral(":/res/RedCircle.mesh"),
                          CStudioPreferences::GetXAxisColor(), QVector3D(50, 50, 50));
    }
}

void Q3DSManipulationWidget::destroyManipulators()
{
    for (auto &model : qAsConst(m_manipulators)) {
        m_presentation->masterSlide()->removeObject(model);
        m_presentation->unlinkObject(model);
        delete model;
    }

    m_manipulators.clear();
    m_manipulatorMaterials.clear();
    m_manipulatorColors.clear();
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

void Q3DSManipulationWidget::applyProperties(Q3DSGraphObject *node, Q3DSCameraNode *camera,
                                             Q3DSLayerNode *layer, const QSize &size)
{
    for (int i = 0; i < m_manipulators.size(); ++i) {
        if (node->type() == Q3DSGraphObject::Model
                || node->type() == Q3DSGraphObject::Alias
                || node->type() == Q3DSGraphObject::Group
                || node->type() == Q3DSGraphObject::Light
                || node->type() == Q3DSGraphObject::Camera
                || node->type() == Q3DSGraphObject::Text
                || node->type() == Q3DSGraphObject::Component) {
            applyNodeProperties(node, camera, layer, size, m_manipulators[i],
                                m_manipulatorScales[i]);
        }
    }

    if (m_type == ManipulationWidgetType::Rotation && m_manipulators.size() >= 4) {
        // The fourth model of the rotation widget has to look towards the camera
        QQuaternion rotation = QQuaternion::fromDirection(
                    (camera->position() - m_manipulators[3]->position()).normalized(),
                    QVector3D(0, 1, 0));

        Q3DSPropertyChangeList list;
        list.append(m_manipulators[3]->setRotation(rotation.toEulerAngles()));
        m_manipulators[3]->notifyPropertyChanges(list);
    }
}

}
