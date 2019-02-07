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

#ifndef Q3DSWIDGETUTILS_H
#define Q3DSWIDGETUTILS_H

#include "q3dsruntime2api_p.h"

namespace Q3DStudio {

void adjustRotationLeftToRight(QMatrix4x4 *m);
Q3DSModelNode *createWidgetModel(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer,
                                 const QString &name, const QString &mesh,
                                 const QVector3D &scale, bool wireframe = false);
Q3DSCustomMaterialInstance *createWidgetCustomMaterial(Q3DSUipPresentation *presentation,
                                                       const QString &name,
                                                       const QString &materialData,
                                                       const QColor &color = Qt::white,
                                                       float opacity = 1.0f);
Q3DSDefaultMaterial *createWidgetDefaultMaterial(Q3DSUipPresentation *presentation,
                                                 const QString &name,
                                                 const QColor &color = Qt::white,
                                                 float opacity = 1.0f);
}

#endif // Q3DSWIDGETUTILS_H
