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

#include "TimelineWidget.h"
#include "TimelineGraphicsScene.h"
#include "TimelineConstants.h"
#include "TimelineToolbar.h"
#include "PlayHead.h"
#include "Ruler.h"
#include "Separator.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"

#include <QtWidgets/qgraphicssceneevent.h>
#include <QtWidgets/qgraphicslinearlayout.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qscrollbar.h>
#include <QtWidgets/qslider.h>
#include <QtWidgets/qsplitter.h>
#include <QtWidgets/qlabel.h>

class Eventfilter : public QObject {
public:
    Eventfilter(QObject *parent) : QObject(parent) {}

    bool eventFilter(QObject *, QEvent *event) override {
        if (event->type() == QEvent::Wheel) {
            event->accept();
            return true;
        }

        return false;
    }
};

TimelineWidget::TimelineWidget(QWidget *parent)
    : QWidget()
    , m_toolbar(new TimelineToolbar())
    , m_viewTreeHeader(new QGraphicsView(this))
    , m_viewTreeContent(new QGraphicsView(this))
    , m_viewTimelineHeader(new QGraphicsView(this))
    , m_viewTimelineContent(new QGraphicsView(this))
    , m_graphicsScene(new TimelineGraphicsScene(m_viewTimelineContent, this)) {
    setWindowTitle(tr("Timeline", "Title of timeline view"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(m_viewTimelineContent->sizePolicy().hasHeightForWidth());

    m_viewTimelineHeader->setScene(m_graphicsScene);
    m_viewTimelineHeader->setFixedHeight(TimelineConstants::ROW_H);
    m_viewTimelineHeader->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_viewTimelineHeader->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewTimelineHeader->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewTimelineHeader->viewport()->installEventFilter(new Eventfilter(this));
    m_viewTimelineHeader->viewport()->setFocusPolicy(Qt::NoFocus);

    m_viewTimelineContent->setScene(m_graphicsScene);
    m_viewTimelineContent->setSizePolicy(sizePolicy1);
    m_viewTimelineContent->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_viewTimelineContent->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    m_viewTreeHeader->setScene(m_graphicsScene);
    m_viewTreeHeader->setFixedHeight(TimelineConstants::ROW_H);
    m_viewTreeHeader->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_viewTreeHeader->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewTreeHeader->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewTreeHeader->viewport()->installEventFilter(new Eventfilter(this));
    m_viewTreeHeader->viewport()->setFocusPolicy(Qt::NoFocus);

    m_viewTreeContent->setScene(m_graphicsScene);
    m_viewTreeContent->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_viewTreeContent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_viewTreeContent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setStyleSheet(QStringLiteral("background-color:%1;").arg(TimelineConstants::WIDGET_BG_COLOR));

    auto *layoutTree = new QVBoxLayout;
    layoutTree->setContentsMargins(QMargins(0, 0, 0, 0));
    layoutTree->addWidget(m_viewTreeHeader);
    layoutTree->addWidget(m_viewTreeContent);

    auto *layoutTimeline = new QVBoxLayout;
    layoutTimeline->setContentsMargins(QMargins(0, 0, 0, 0));
    layoutTimeline->addWidget(m_viewTimelineHeader);
    layoutTimeline->addWidget(m_viewTimelineContent);

    auto *layoutContent = new QHBoxLayout;
    layoutContent->setContentsMargins(QMargins(0, 0, 0, 10));
    layoutContent->addLayout(layoutTree);
    layoutContent->addLayout(layoutTimeline);

    auto *widgetLayout = new QVBoxLayout;
    widgetLayout->setContentsMargins(0, 0, 0, 0);
    widgetLayout->setSpacing(0);
    widgetLayout->addLayout(layoutContent);
    widgetLayout->addWidget(m_toolbar);
    setLayout(widgetLayout);

    // connections
    connect(m_graphicsScene->widgetRoot(), &QGraphicsWidget::geometryChanged, this, [this]() {
        // TODO: work in progress
        qDebug() << "geometryChanged";
        qDebug() << m_graphicsScene->widgetRoot()->rect();
        m_viewTimelineHeader->setSceneRect(m_graphicsScene->widgetRoot()->rect().adjusted(
                                           m_graphicsScene->ruler()->x(), 0, 0, 0));

        m_viewTimelineContent->setSceneRect(m_graphicsScene->widgetRoot()->rect().adjusted(
                                            m_graphicsScene->ruler()->x(),
                                            TimelineConstants::ROW_H, 0, 0));

        m_viewTreeContent->setSceneRect(QRectF(
                                        0, TimelineConstants::ROW_H,
                                        m_graphicsScene->ruler()->x(), 0));

        m_graphicsScene->playHead()->setHeight(m_graphicsScene->widgetRoot()->geometry().height());

        m_viewTreeHeader->setFixedWidth(m_graphicsScene->ruler()->x());
        m_viewTreeContent->setFixedWidth(m_graphicsScene->ruler()->x());
    });

    // connect timeline and ruler horizontalScrollBars
    connect(m_viewTimelineContent->horizontalScrollBar(), &QAbstractSlider::valueChanged, this,
            [this](int value) {
        m_viewTimelineHeader->horizontalScrollBar()->setValue(value);
    });

    // connect timeline and tree verticalScrollBars
    connect(m_viewTimelineContent->verticalScrollBar(), &QAbstractSlider::valueChanged, this,
            [this](int value) {
        m_viewTreeContent->verticalScrollBar()->setValue(value);
    });

    // connect tree and timeline verticalScrollBars
    connect(m_viewTreeContent->verticalScrollBar(), &QAbstractSlider::valueChanged, this,
            [this](int value) {
        m_viewTimelineContent->verticalScrollBar()->setValue(value);
    });

    // connect tree and tree header horizontalScrollBars
    connect(m_viewTreeContent->horizontalScrollBar(), &QAbstractSlider::valueChanged, this,
            [this](int value) {
        m_viewTreeHeader->horizontalScrollBar()->setValue(value);
    });

    connect(m_toolbar, &TimelineToolbar::newLayerTriggered, this, [this]() {
        m_graphicsScene->addNewLayer();
    });

    connect(m_toolbar, &TimelineToolbar::deleteLayerTriggered, this, [this]() {
        m_graphicsScene->deleteSelectedRow();
    });

    connect(m_toolbar, &TimelineToolbar::gotoTimeTriggered, this, [this]() {
        // TODO: implement
    });

    connect(m_toolbar, &TimelineToolbar::firstFrameTriggered, this, [this]() {
        m_graphicsScene->playHead()->setTime(0);
        m_toolbar->setTime(0);
        g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(0);
    });

    connect(m_toolbar, &TimelineToolbar::stopTriggered, this, [this]() {
        g_StudioApp.PlaybackStop();
    });

    connect(m_toolbar, &TimelineToolbar::playTriggered, this, [this]() {
        g_StudioApp.PlaybackPlay();
    });

    connect(m_toolbar, &TimelineToolbar::lastFrameTriggered, this, [this]() {
        double dur = m_graphicsScene->ruler()->duration();
        m_graphicsScene->playHead()->setTime(dur);
        m_toolbar->setTime(dur);
        g_StudioApp.GetCore()->GetDoc()->NotifyTimeChanged(dur);
    });

    connect(m_toolbar, &TimelineToolbar::timelineScaleChanged, this, [this](int scale) {
        m_graphicsScene->setTimelineScale(scale);
    });
}

TimelineToolbar *TimelineWidget::toolbar() const
{
    return m_toolbar;
}
