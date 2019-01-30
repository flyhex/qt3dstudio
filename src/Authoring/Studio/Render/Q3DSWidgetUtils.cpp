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

namespace Q3DStudio {

void adjustRotationLeftToRight(QMatrix4x4 *m)
{
    float *p = m->data();
    p[2] *= -1;
    p[6] *= -1;
    p[8] *= -1;
    p[9] *= -1;
}

Q3DSModelNode *createWidgetModel(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer,
                                 const QString &name, const QString &mesh,
                                 const QVector3D &scale, bool wireframe = false)
{
    Q3DSModelNode *model = presentation->newObject<Q3DSModelNode>(
                (name + QLatin1Char('_')).toUtf8().constData());
    layer->appendChildNode(model);
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
    const QByteArray matName = (name + QLatin1String("Material_")).toUtf8();
    const QByteArray matId = '#' + matName;

    Q3DSDefaultMaterial *defMat = presentation->newObject<Q3DSDefaultMaterial>(matId);
    defMat->setDiffuse(color);
    defMat->setOpacity(opacity);
    return defMat;
}

}
