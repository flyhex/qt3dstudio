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

#ifndef Q3DSMANIPULATIONWIDGET_H
#define Q3DSMANIPULATIONWIDGET_H

#include "q3dsruntime2api_p.h"

namespace Q3DStudio {

enum class ManipulationWidgetType
{
    Translation,
    Rotation,
    Scale
};

class Q3DSManipulationWidget
{
public:
    bool hasManipulators() const;
    bool isXAxis(Q3DSGraphObject *obj) const;
    bool isYAxis(Q3DSGraphObject *obj) const;
    bool isZAxis(Q3DSGraphObject *obj) const;
    bool isXYPlane(Q3DSGraphObject *obj) const;
    bool isYZPlane(Q3DSGraphObject *obj) const;
    bool isZXPlane(Q3DSGraphObject *obj) const;
    bool isXYCircle(Q3DSGraphObject *obj) const;
    bool isYZCircle(Q3DSGraphObject *obj) const;
    bool isZXCircle(Q3DSGraphObject *obj) const;
    bool isCameraCircle(Q3DSGraphObject *obj) const;

    void setDefaultScale(const QVector3D &scale);
    void setScale(Q3DSGraphObject *obj, const QVector3D &scale);
    void resetScale(Q3DSGraphObject *obj = nullptr);
    void setColor(Q3DSGraphObject *obj, const QColor &color);
    void resetColor(Q3DSGraphObject *obj);
    void setEyeballEnabled(bool value);

    void createManipulators(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer,
                            ManipulationWidgetType type);
    void destroyManipulators();

    void applyProperties(Q3DSGraphObject *node, Q3DSCameraNode *camera,
                         Q3DSLayerNode *layer, bool globalSpace);

private:
    ManipulationWidgetType m_type = ManipulationWidgetType::Translation;
    Q3DSUipPresentation *m_presentation = nullptr;

    QVector<Q3DSModelNode *> m_manipulators;
    QVector<Q3DSCustomMaterialInstance *> m_manipulatorMaterials;
    QVector<QColor> m_manipulatorColors;
    QVector<QVector3D> m_manipulatorScales;
    QVector3D m_defaultScale;

    void createManipulator(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer,
                           const QString &name, const QString &mesh,
                           const QColor &color, const QVector3D &scale);
};

}

#endif // Q3DSMANIPULATIONWIDGET_H
