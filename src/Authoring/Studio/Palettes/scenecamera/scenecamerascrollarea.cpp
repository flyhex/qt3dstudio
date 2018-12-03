/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "Qt3DSCommonPrecompile.h"
#include "scenecamerascrollarea.h"
#include "scenecameraglwidget.h"
#include "Core.h"

#include <QtWidgets/qscrollbar.h>
#include <QtGui/qevent.h>

SceneCameraScrollArea::SceneCameraScrollArea(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_glWidget = new SceneCameraGlWidget(this);
}

SceneCameraScrollArea::~SceneCameraScrollArea()
{
}

void SceneCameraScrollArea::setZoom(qreal zoom, const QPoint &zoomPoint)
{
    // Calculate the actual presentation point
    qreal oldH = (horizontalScrollBar()->value() + zoomPoint.x()) / m_zoom;
    qreal oldV = (verticalScrollBar()->value() + zoomPoint.y()) / m_zoom;

    m_zoom = zoom;

    recalculateScrollRanges();

    // Move the scrollbars so that the actual presentation point stays in the same location
    horizontalScrollBar()->setValue(qRound(oldH * m_zoom - zoomPoint.x()));
    verticalScrollBar()->setValue(qRound(oldV * m_zoom - zoomPoint.y()));

    recalculateOffsets();

    Q_EMIT needUpdate();
}

void SceneCameraScrollArea::setPresentationSize(const QSize &size)
{
    if (m_presentationSize != size) {
        m_presentationSize = size;
        recalculateScrollRanges();
        recalculateOffsets();
    }
}

void SceneCameraScrollArea::recalculateScrollRanges()
{
    const QSizeF presSize = zoomedPresentationSize();

    const QSize viewSize = viewport()->size();
    horizontalScrollBar()->setRange(0, int(presSize.width() - viewSize.width()));
    verticalScrollBar()->setRange(0, int(presSize.height() - viewSize.height()));
    horizontalScrollBar()->setPageStep(viewSize.width());
    verticalScrollBar()->setPageStep(viewSize.height());
}

void SceneCameraScrollArea::recalculateOffsets()
{
    // Texture offset vector contains normalized rect of the viewable area of the texture
    const QSize viewSize = viewport()->size();
    const qreal fullWidth = qreal(horizontalScrollBar()->maximum() + viewSize.width());
    const qreal fullHeight = qreal(verticalScrollBar()->maximum() + viewSize.height());
    QVector4D textureOffset(
                float(horizontalScrollBar()->value() / fullWidth),
                float((verticalScrollBar()->maximum() - verticalScrollBar()->value()) / fullHeight),
                float(viewSize.width() / fullWidth), float(viewSize.height() / fullHeight));

    m_glWidget->setTextureOffset(textureOffset);

    // The geometry offset is adjusted to keep aspect ratio when view area is larger than
    // zoomed width/height. Since the geometry of the quad is in range [-1, 1], the width/height of
    // the offset is just a direct multiplier to the coordinate.
    // XY contain the subpixel offset to ensure we don't get artifacts depending on pixel alignment.
    const QSizeF presSize = zoomedPresentationSize();
    float subPixelX = 0.0f;
    float subPixelY = 0.0f;
    qreal normWidth = 1.0;
    qreal normHeight = 1.0;
    if (presSize.width() < fullWidth) {
        qreal diffX = (fullWidth - qRound(presSize.width())) / 2.0;
        subPixelX = float((diffX - qRound(diffX)) / fullWidth);
        normWidth = presSize.width() / fullWidth;
    }
    if (presSize.height() < fullHeight) {
        qreal diffY = (fullHeight - qRound(presSize.height())) / 2.0;
        subPixelY = float((diffY - qRound(diffY)) / fullHeight);
        normHeight = presSize.height() / fullHeight;
    }

    QVector4D geometryOffset(subPixelX, subPixelY, float(normWidth), float(normHeight));
    m_glWidget->setGeometryOffset(geometryOffset);
}

void SceneCameraScrollArea::scrollContentsBy(int, int)
{
    recalculateOffsets();
    Q_EMIT needUpdate();
}

void SceneCameraScrollArea::showEvent(QShowEvent *event)
{
    QAbstractScrollArea::showEvent(event);

    recalculateScrollRanges();
    recalculateOffsets();
    resizeGlWidget();
}

void SceneCameraScrollArea::resizeGlWidget()
{
    m_glWidget->resize(viewport()->size());
}

QSizeF SceneCameraScrollArea::zoomedPresentationSize()
{
    // Multiply QSize components separately to avoid rounding to integers
    QSizeF size = QSizeF(m_presentationSize.width() * m_zoom,
                         m_presentationSize.height() * m_zoom);
    return size;
}

void SceneCameraScrollArea::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);

    recalculateScrollRanges();
    recalculateOffsets();
    resizeGlWidget();
}
