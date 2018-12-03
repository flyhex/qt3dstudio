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

#include "scenecameraview.h"
#include "ui_scenecameraview.h"
#include "scenecameraglwidget.h"
#include "StudioApp.h"
#include "Core.h"
#include "StudioProjectSettings.h"
#include "MainFrm.h"
#include "Q3DSPlayerWnd.h"
#include "MouseCursor.h"
#include "ResourceCache.h"

#include <QtGui/qevent.h>
#include <QtWidgets/qscrollbar.h>

const QPoint invalidMousePoint = QPoint(-999999, -999999);

SceneCameraView::SceneCameraView(CMainFrame *mainFrame, QWidget *parent) :
    QWidget(parent)
  , m_ui(new Ui::SceneCameraView)
  , m_mousePressPointLeft(invalidMousePoint)
  , m_mousePressPointRight(invalidMousePoint)
{
    m_ui->setupUi(this);

    m_cursorPan = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_EDIT_CAMERA_PAN);
    m_cursorZoom = CResourceCache::GetInstance()->GetCursor(CMouseCursor::CURSOR_EDIT_CAMERA_ZOOM);

    // Limit the preview framerate a bit to limit amount of updates when dragging the slider
    m_updateTimer.setInterval(0);
    m_updateTimer.setSingleShot(true);

    connect(m_ui->zoomSlider, &QSlider::valueChanged,
            this, &SceneCameraView::handleSliderValueChange);
    connect(mainFrame->GetPlayerWnd(), &Q3DStudio::Q3DSPlayerWnd::newFrame,
            this, &SceneCameraView::requestUpdate);
    connect(m_ui->scrollArea, &SceneCameraScrollArea::needUpdate,
            this, &SceneCameraView::requestUpdate);
    connect(&m_updateTimer, &QTimer::timeout, this, &SceneCameraView::doUpdate);
}

SceneCameraView::~SceneCameraView()
{
    delete m_ui;
}

void SceneCameraView::wheelEvent(QWheelEvent *e)
{
    m_zoomPoint = m_ui->scrollArea->viewport()->mapFrom(this, e->pos());
    int currentZoomValue = m_ui->zoomSlider->value();
    // Adjust amount of change based on zoom level
    int divider = qMin(120, 1000 / currentZoomValue);
    m_ui->zoomSlider->setValue(currentZoomValue + (e->angleDelta().y() / divider));
}

void SceneCameraView::resizeEvent(QResizeEvent *e)
{
    m_zoomPoint = m_ui->scrollArea->viewport()->geometry().center();

    QWidget::resizeEvent(e);
}

void SceneCameraView::mousePressEvent(QMouseEvent *e)
{
    // Ignore panning starting outside scrollarea
    if (!m_ui->scrollArea->rect().contains(e->pos()))
        return;

    // Panning can be done with left or middle button. Left is more natural and we don't need it
    // for selection. Alt+middle pans in edit camera mode, so middle button is also supported for
    // panning.
    if (m_mousePressPointRight == invalidMousePoint
            && (e->button() == Qt::LeftButton || e->button() == Qt::MidButton)) {
        m_mousePressPointLeft = e->pos();
        m_mousePressScrollValues = QPoint(m_ui->scrollArea->horizontalScrollBar()->value(),
                                          m_ui->scrollArea->verticalScrollBar()->value());
        setCursor(m_cursorPan);
    } else if (m_mousePressPointLeft == invalidMousePoint && e->button() == Qt::RightButton) {
        m_mousePressPointRight = e->pos();
        m_mousePressZoomValue = m_ui->zoomSlider->value();
        setCursor(m_cursorZoom);
    }
}

void SceneCameraView::mouseMoveEvent(QMouseEvent *e)
{
    if (m_mousePressPointLeft != invalidMousePoint) {
        const QPoint delta = e->pos() - m_mousePressPointLeft;
        m_ui->scrollArea->horizontalScrollBar()->setValue(m_mousePressScrollValues.x() - delta.x());
        m_ui->scrollArea->verticalScrollBar()->setValue(m_mousePressScrollValues.y() - delta.y());
    }
    if (m_mousePressPointRight != invalidMousePoint) {
        const qreal delta = qreal(e->pos().y() - m_mousePressPointRight.y());
        m_zoomPoint = m_mousePressPointRight;
        m_ui->zoomSlider->setValue(m_mousePressZoomValue - delta / 2.0);
    }
}

void SceneCameraView::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_mousePressPointLeft != invalidMousePoint
            && e->button() == Qt::LeftButton || e->button() == Qt::MidButton) {
        m_mousePressPointLeft = invalidMousePoint;
        setCursor(Qt::ArrowCursor);
    } else if (m_mousePressPointRight != invalidMousePoint) {
        m_zoomPoint = m_ui->scrollArea->viewport()->geometry().center();
        m_mousePressPointRight = invalidMousePoint;
        m_mousePressZoomValue = 0;
        setCursor(Qt::ArrowCursor);
    }
}

void SceneCameraView::handleSliderValueChange()
{
    const qreal zoom = qreal(m_ui->zoomSlider->value()) / 10.0;
    QString valueString = QString::number(zoom, 'f', 1);
    m_ui->slideValueLabel->setText(tr("%1x").arg(valueString));
    m_ui->scrollArea->setZoom(zoom, m_zoomPoint);
    m_zoomPoint = m_ui->scrollArea->viewport()->geometry().center();
}

void SceneCameraView::doUpdate()
{
    // There is no event for presentation size change, so update every frame to catch the change
    m_ui->scrollArea->setPresentationSize(
                g_StudioApp.GetCore()->GetStudioProjectSettings()->getPresentationSize());
    m_ui->scrollArea->glWidget()->update();
}

void SceneCameraView::requestUpdate()
{
    if (!m_updateTimer.isActive())
        m_updateTimer.start();
}
