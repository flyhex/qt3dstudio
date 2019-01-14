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

#ifndef Q3DSSELECTIONWIDGET_H
#define Q3DSSELECTIONWIDGET_H

#include <QtCore/qmath.h>
#include "q3dsruntime2api_p.h"

namespace Q3DStudio {

enum class SelectionWidgetType
{
    Translation,
    Rotation,
    Scale
};

class Q3DSSelectionWidget
{
public:
    bool isCreated() const;
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

    void setScale(Q3DSGraphObject *obj, const QVector3D &scale);
    void resetScale(Q3DSGraphObject *obj);
    void setColor(Q3DSGraphObject *obj, const QColor &color);
    void resetColor(Q3DSGraphObject *obj);
    void setEyeballEnabled(bool value);

    void create(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer, SelectionWidgetType type);
    void destroy(Q3DSUipPresentation *presentation);
    void applyProperties(Q3DSGraphObject *node, Q3DSCameraNode *camera);

    static void adjustRotationLeftToRight(QMatrix4x4 *m)
    {
        float *p = m->data();
        p[2] *= -1;
        p[6] *= -1;
        p[8] *= -1;
        p[9] *= -1;
    }

private:
    SelectionWidgetType m_type = SelectionWidgetType::Translation;
    QVector<Q3DSModelNode *> m_models;
    QVector<Q3DSCustomMaterialInstance *> m_materials;
    QVector<QColor> m_colors;
    QVector<QVector3D> m_scales;

    void createModel(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer,
                     const QString &name, const QString &mesh,
                     const QColor &color, const QVector3D &scale);
};

}

#endif // Q3DSSELECTIONWIDGET_H
