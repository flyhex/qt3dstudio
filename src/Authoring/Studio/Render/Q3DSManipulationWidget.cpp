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

void Q3DSManipulationWidget::setDefaultScale(const QVector3D &scale)
{
    m_defaultScale = scale;
    resetScale();
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
        if (!obj || m_manipulators[i] == obj) {
            m_manipulatorScales[i] = m_defaultScale;
            if (obj)
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
        m_manipulatorScales.append(m_defaultScale);
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

void applyNodeProperties(Q3DSNode *node, Q3DSCameraNode *camera, Q3DSLayerNode *layer,
                         Q3DSModelNode *model, const QVector3D &modelScale, bool globalSpace) {

    QVector3D position;
    QVector3D rotation;
    QVector3D dummyScale;
    calculateGlobalProperties(node, position, rotation, dummyScale);

    if (globalSpace)
        rotation = QVector3D(0, 0, 0);

    Q3DSPropertyChangeList list;
    list.append(model->setPosition(position));
    list.append(model->setRotation(rotation));

    Q3DSCameraAttached *cameraAttached = camera->attached<Q3DSCameraAttached>();
    QMatrix4x4 cameraMatrix = cameraAttached->globalTransform;
    QVector3D cameraPlane = getZAxis(cameraMatrix);

    QVector3D originPosition = mousePointToPlaneIntersection(QPoint(0, 0), camera, layer,
                                                             position, cameraPlane, false);
    QVector3D vertPosition = mousePointToPlaneIntersection(QPoint(0, 80), camera, layer,
                                                           position, cameraPlane, false);
    QVector3D horzPosition = mousePointToPlaneIntersection(QPoint(80, 0), camera, layer,
                                                           position, cameraPlane, false);

    QMatrix4x4 invCameraRotMat = generateRotationMatrix(camera->rotation(),
                                                        camera->rotationOrder()).inverted();
    QVector3D vertScale = invCameraRotMat * (vertPosition - originPosition);
    QVector3D horzScale = invCameraRotMat * (horzPosition - originPosition);

    QVector3D depthScale = QVector3D::crossProduct(vertScale.normalized(), horzScale.normalized())
            * (vertScale.length() + horzScale.length()) / 2.0f;

    QVector3D scaleFactor = vertScale + horzScale + depthScale;
    scaleFactor.setX(qAbs(scaleFactor.x()));
    scaleFactor.setY(qAbs(scaleFactor.y()));
    scaleFactor.setZ(qAbs(scaleFactor.z()));

    list.append(model->setScale(modelScale * scaleFactor));
    list.append(model->setEyeballEnabled(true));
    model->notifyPropertyChanges(list);
}

void Q3DSManipulationWidget::applyProperties(Q3DSGraphObject *node, Q3DSCameraNode *camera,
                                             Q3DSLayerNode *layer, bool globalSpace)
{
    for (int i = 0; i < m_manipulators.size(); ++i) {
        if (node->type() == Q3DSGraphObject::Model
                || node->type() == Q3DSGraphObject::Alias
                || node->type() == Q3DSGraphObject::Group
                || node->type() == Q3DSGraphObject::Light
                || node->type() == Q3DSGraphObject::Camera
                || node->type() == Q3DSGraphObject::Text
                || node->type() == Q3DSGraphObject::Component) {
            applyNodeProperties(static_cast<Q3DSNode *>(node), camera, layer, m_manipulators[i],
                                m_manipulatorScales[i], globalSpace);
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
