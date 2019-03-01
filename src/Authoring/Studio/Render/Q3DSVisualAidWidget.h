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

#ifndef Q3DSVISUALAIDWIDGET_H
#define Q3DSVISUALAIDWIDGET_H

#include <Qt3DRender/qcamera.h>
#include "q3dsruntime2api_p.h"

namespace Q3DStudio {

enum class VisualAidType
{
    Camera,
    DirectionalLight,
    PointLight,
    AreaLight
};

class Q3DSVisualAidWidget
{
public:
    Q3DSVisualAidWidget();
    Q3DSVisualAidWidget(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer,
                        Q3DSLayerNode *pickingLayer, VisualAidType type,
                        Q3DSGraphObject *graphObject, quint64 id);
    ~Q3DSVisualAidWidget();

    bool isCreated() const;
    Q3DSGraphObject *graphObject() const;
    VisualAidType type() const;
    bool hasGraphObject(Q3DSGraphObject *graphObject) const;
    bool hasCollisionBox(Q3DSGraphObject *graphObject) const;

    void destroy();
    void update(Q3DSCameraNode *layerCameraNode, Q3DSGraphObject *selectedObject);

private:
    VisualAidType m_type = VisualAidType::Camera;
    Q3DSModelNode *m_wireframe = nullptr;
    Q3DSCustomMaterialInstance *m_wireframeMaterial = nullptr;
    Q3DSModelNode *m_icon = nullptr;
    Q3DSCustomMaterialInstance *m_iconMaterial = nullptr;
    Q3DSModelNode *m_collisionBox = nullptr;
    Q3DSGraphObject *m_graphObject = nullptr;
    Q3DSUipPresentation *m_presentation = nullptr;
    bool m_isSelected = false;
};

}

#endif // Q3DSVISUALAIDWIDGET_H
