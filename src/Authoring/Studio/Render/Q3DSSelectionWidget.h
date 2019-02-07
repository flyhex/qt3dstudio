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

#include "q3dsruntime2api_p.h"

namespace Q3DStudio {

class Q3DSSelectionWidget
{
public:
    void select(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer, Q3DSNode *node);
    void update();

private:
    Q3DSUipPresentation *m_presentation = nullptr;

    struct BoundingBox
    {
        QVector3D min = QVector3D(std::numeric_limits<float>::max(),
                                  std::numeric_limits<float>::max(),
                                  std::numeric_limits<float>::max());
        QVector3D max = QVector3D(-std::numeric_limits<float>::max(),
                                  -std::numeric_limits<float>::max(),
                                  -std::numeric_limits<float>::max());
    };

    struct Selection
    {
        QVector<Q3DSNode *> nodes;
        QVector<Q3DSModelNode *> models;
        QVector<BoundingBox> boundingBoxes;
    };

    QHash<QString, BoundingBox> m_boundingBoxCache;
    QHash<Q3DSLayerNode *, Selection> m_selections;

    BoundingBox calculateLocalBoundingBox(Q3DSModelNode *model);
    BoundingBox calculateBoundingBox(Q3DSGraphObject *graphObject);
    void rotateBoundingBox(BoundingBox &box, const QQuaternion &rotation);
    void selectRecursive(Q3DSUipPresentation *presentation, Q3DSLayerNode *layer, Q3DSNode *node,
                         int &index, int depth);
};

}

#endif // Q3DSSELECTIONWIDGET_H
