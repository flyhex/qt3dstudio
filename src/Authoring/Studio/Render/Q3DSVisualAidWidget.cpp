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

#include "Q3DSVisualAidWidget.h"
#include "Q3DSWidgetUtils.h"
#include <Qt3DCore/qtransform.h>

namespace Q3DStudio {

Q3DSVisualAidWidget::Q3DSVisualAidWidget() {}

Q3DSVisualAidWidget::Q3DSVisualAidWidget(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer,
                                         Q3DSLayerNode *pickingLayer, VisualAidType type,
                                         Q3DSGraphObject *graphObject, quint64 id)
{
    m_type = type;
    m_graphObject = graphObject;
    m_presentation = presentation;

    const QString basicMaterial = QStringLiteral(
            "<MetaData>\n"
            "<Property name=\"color\" type=\"Color\" default=\"1.0 0.0 0.0\""
            " stage=\"fragment\" />\n"
            "<Property name=\"opacity\" type=\"Float\" default=\"1.0\""
            " stage=\"fragment\" />\n"
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
            "fragOutput = vec4(color, opacity);\n"
            "</FragmentShader>\n"
            "</Shader>\n"
            "</Shaders>\n"
            "<Passes><Pass></Pass></Passes>\n");

    const QString cameraMaterial = QStringLiteral(
            "<MetaData>\n"
            "<Property name=\"color\" type=\"Color\" default=\"1.0 0.0 0.0\""
            " stage=\"fragment\" />\n"
            "<Property name=\"opacity\" type=\"Float\" default=\"1.0\""
            " stage=\"fragment\" />\n"
            "<Property name=\"invProjMat\" type=\"Matrix4x4\" default=\""
            "0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0\""
            " stage=\"vertex\" />\n"
            "</MetaData>\n"
            "<Shaders type=\"GLSL\" version=\"330\">\n"
            "<Shader>\n"
            "<VertexShader>\n"
            "attribute vec3 attr_pos;\n"
            "uniform mat4 modelViewProjection;\n"
            "void main() {\n"
            "vec4 pos = vec4(0.0, 0.0, 0.0, 1.0);\n"
            "if (length(attr_pos) > 0.001) {"
            "pos = invProjMat * vec4(attr_pos, 1.0);\n"
            "pos = pos / pos.w;\n"
            "}\n"
            "gl_Position = modelViewProjection * vec4(pos.xyz, 1.0);\n"
            "</VertexShader>\n"
            "<FragmentShader>\n"
            "void main() {\n"
            "fragOutput = vec4(color, opacity);\n"
            "</FragmentShader>\n"
            "</Shader>\n"
            "</Shaders>\n"
            "<Passes><Pass></Pass></Passes>\n");

    const QString billboardMaterial = QStringLiteral(
            "<MetaData>\n"
            "<Property name=\"color\" type=\"Color\" default=\"1.0 0.0 0.0\""
            " stage=\"fragment\" />\n"
            "<Property name=\"image\" type=\"Texture\""
            " default=\":/images/Asset-Camera-Pick.png\" stage=\"fragment\" />\n"
            "<Property name=\"billboardMatrix\" type=\"Matrix4x4\" default=\""
            "0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0\""
            " stage=\"vertex\" />\n"
            "</MetaData>\n"
            "<Shaders type=\"GLSL\" version=\"330\">\n"
            "<Shader>\n"
            "<VertexShader>\n"
            "attribute vec3 attr_pos;\n"
            "attribute vec2 attr_uv0;\n"
            "varying vec2 texCoords;\n"
            "void main() {\n"
            "texCoords = attr_uv0;\n"
            "gl_Position = billboardMatrix * vec4(attr_pos.xyz, 1.0);\n"
            "</VertexShader>\n"
            "<FragmentShader>\n"
            "varying vec2 texCoords;\n"
            "void main() {\n"
            "fragOutput = texture(image, texCoords);\n"
            "</FragmentShader>\n"
            "</Shader>\n"
            "</Shaders>\n"
            "<Passes><Pass></Pass></Passes>\n");

    if (m_type == VisualAidType::Camera) {
        const QString name = QStringLiteral("StudioFrustum") + QString::number(id);
        m_wireframeMaterial = createWidgetCustomMaterial(presentation, name, cameraMaterial,
                                                         Qt::white, 0.25f);
        if (m_wireframeMaterial) {
            m_wireframe = createWidgetModel(presentation, layer, name,
                                            QStringLiteral(":/res/Frustum.mesh"),
                                            QVector3D(1, 1, 1), true);
            m_wireframe->appendChildNode(m_wireframeMaterial);
        }
    } else if (m_type == VisualAidType::DirectionalLight) {
        const QString name = QStringLiteral("StudioDirectionalLight") + QString::number(id);
        m_wireframeMaterial = createWidgetCustomMaterial(presentation, name, basicMaterial,
                                                         Qt::white, 0.25f);
        if (m_wireframeMaterial) {
            m_wireframe = createWidgetModel(presentation, layer, name,
                                            QStringLiteral(":/res/DirLight.mesh"),
                                            QVector3D(50, 50, 50), true);
            m_wireframe->appendChildNode(m_wireframeMaterial);
        }
    } else if (m_type == VisualAidType::PointLight) {
        const QString name = QStringLiteral("StudioPointLight") + QString::number(id);
        m_wireframeMaterial = createWidgetCustomMaterial(presentation, name, basicMaterial,
                                                         Qt::white, 0.25f);
        if (m_wireframeMaterial) {
            m_wireframe = createWidgetModel(presentation, layer, name,
                                            QStringLiteral(":/res/PointLight.mesh"),
                                            QVector3D(50, 50, 50), true);
            m_wireframe->appendChildNode(m_wireframeMaterial);
        }
    } else if (m_type == VisualAidType::AreaLight) {
        const QString name = QStringLiteral("StudioAreaLight") + QString::number(id);
        m_wireframeMaterial = createWidgetCustomMaterial(presentation, name, basicMaterial,
                                                         Qt::white, 0.25f);
        if (m_wireframeMaterial) {
            m_wireframe = createWidgetModel(presentation, layer, name,
                                            QStringLiteral(":/res/AreaLight.mesh"),
                                            QVector3D(50, 50, 50), true);
            m_wireframe->appendChildNode(m_wireframeMaterial);
        }
    }

    const QString iconName = QStringLiteral("StudioIcon") + QString::number(id);
    m_iconMaterial = createWidgetCustomMaterial(presentation, iconName, billboardMaterial,
                                                Qt::white, 0.25f);
    if (m_iconMaterial) {
        m_icon = createWidgetModel(presentation, layer, iconName,
                                   QStringLiteral(":/res/Icon.mesh"), QVector3D(1, 1, 1));
        m_icon->appendChildNode(m_iconMaterial);

        Q3DSPropertyChangeList imageChange = {
            Q3DSPropertyChange::fromVariant(QStringLiteral("image"),
            m_type == VisualAidType::Camera ? QStringLiteral(":/images/Asset-Camera-Pick.png")
                                            : QStringLiteral(":/images/Asset-Light-Pick.png"))
        };

        m_iconMaterial->applyPropertyChanges(imageChange);
        m_iconMaterial->notifyPropertyChanges(imageChange);
    }

    const QString colBoxName = QStringLiteral("StudioCollisionBox") + QString::number(id);
    m_collisionBox = createWidgetModel(presentation, pickingLayer, colBoxName,
                                       QStringLiteral("#Cube"), QVector3D(0.5f, 0.5f, 0.5f));
    m_collisionBox->appendChildNode(createWidgetDefaultMaterial(presentation, colBoxName));

    if (!isCreated()) {
        destroy();
        m_graphObject = nullptr;
        m_presentation = nullptr;
    }
}

bool Q3DSVisualAidWidget::isCreated() const
{
    return m_wireframe && m_icon && m_collisionBox;
}

Q3DSGraphObject *Q3DSVisualAidWidget::graphObject() const
{
    return m_graphObject;
}

VisualAidType Q3DSVisualAidWidget::type() const
{
    return m_type;
}

bool Q3DSVisualAidWidget::hasGraphObject(Q3DSGraphObject *graphObject) const
{
    return m_graphObject == graphObject;
}

bool Q3DSVisualAidWidget::hasCollisionBox(Q3DSGraphObject *graphObject) const
{
    return m_collisionBox == graphObject;
}

void Q3DSVisualAidWidget::destroy()
{
    if (m_wireframe) {
        m_presentation->masterSlide()->removeObject(m_wireframe);
        m_presentation->unlinkObject(m_wireframe);
        delete m_wireframe;
        m_wireframe = nullptr;
        m_wireframeMaterial = nullptr;
    }

    if (m_icon) {
        m_presentation->masterSlide()->removeObject(m_icon);
        m_presentation->unlinkObject(m_icon);
        delete m_icon;
        m_icon = nullptr;
        m_iconMaterial = nullptr;
    }

    if (m_collisionBox) {
        m_presentation->masterSlide()->removeObject(m_collisionBox);
        m_presentation->unlinkObject(m_collisionBox);
        delete m_collisionBox;
        m_collisionBox = nullptr;
    }

    m_graphObject = nullptr;
    m_presentation = nullptr;
}

void Q3DSVisualAidWidget::update(Q3DSCameraNode *layerCameraNode) const
{
    if (!isCreated())
        return;

    Q3DSNodeAttached *attached = m_graphObject->attached<Q3DSNodeAttached>();
    if (!attached)
        return;

    Qt3DCore::QTransform transform;
    QMatrix4x4 globalMatrix = attached->globalTransform;
    adjustRotationLeftToRight(&globalMatrix);
    transform.setMatrix(globalMatrix);

    QVector3D position = transform.translation();
    position.setZ(-position.z());

    QVector3D rotation = transform.rotation().toEulerAngles();

    m_collisionBox->notifyPropertyChanges({ m_collisionBox->setPosition(position),
                                            m_collisionBox->setRotation(rotation) });

    Q3DSPropertyChangeList list;
    list.append(m_wireframe->setPosition(position));
    list.append(m_wireframe->setRotation(rotation));

    if (m_type == VisualAidType::Camera) {
        Q3DSCameraNode *cameraNode = static_cast<Q3DSCameraNode *>(m_graphObject);
        Q3DSCameraAttached *cameraAttached = cameraNode->attached<Q3DSCameraAttached>();
        Qt3DRender::QCamera *camera = cameraAttached->camera;

        QMatrix4x4 invProjMat = camera->projectionMatrix().inverted();

        if (cameraNode->orthographic()) {
            list.append(m_wireframe->setScale(
                            QVector3D(m_presentation->presentationWidth() * 0.5f,
                                      m_presentation->presentationHeight() * 0.5f, 1)));
        } else {
            list.append(m_wireframe->setScale(QVector3D(1, 1, 1)));
        }

        Q3DSPropertyChangeList projectionChange = {
            Q3DSPropertyChange::fromVariant(
            QStringLiteral("invProjMat"), invProjMat)
        };

        m_wireframeMaterial->applyPropertyChanges(projectionChange);
        m_wireframeMaterial->notifyPropertyChanges(projectionChange);
    } else {
        Q3DSLightNode *lightNode = static_cast<Q3DSLightNode *>(m_graphObject);
        list.append(m_wireframe->setScale(lightNode->scale() * 50.0f));
    }

    m_wireframe->notifyPropertyChanges(list);

    Q3DSCameraAttached *layerCameraAttached = layerCameraNode->attached<Q3DSCameraAttached>();
    Qt3DRender::QCamera *layerCamera = layerCameraAttached->camera;

    QMatrix4x4 billboardMatrix;
    billboardMatrix.setColumn(3, (layerCamera->viewMatrix() * attached->globalTransform).column(3));
    billboardMatrix.scale(16, 16, 1);
    billboardMatrix = layerCamera->projectionMatrix() * billboardMatrix;

    Q3DSPropertyChangeList billboardChange = {
        Q3DSPropertyChange::fromVariant(
        QStringLiteral("billboardMatrix"), billboardMatrix)
    };

    m_iconMaterial->applyPropertyChanges(billboardChange);
    m_iconMaterial->notifyPropertyChanges(billboardChange);
}

}
