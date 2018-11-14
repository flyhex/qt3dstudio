/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include "EventsBrowserView.h"

#include <QtGui/qcolor.h>
#include "EventsModel.h"
#include "Literals.h"
#include "StudioUtils.h"
#include "StudioPreferences.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qtimer.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>

EventsBrowserView::EventsBrowserView(QWidget *parent) : QQuickWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &EventsBrowserView::initialize);
}

QAbstractItemModel *EventsBrowserView::model() const
{
    return m_model;
}

void EventsBrowserView::setModel(EventsModel *model)
{
    if (m_model != model) {
        m_model = model;
        Q_EMIT modelChanged();
    }
}

QSize EventsBrowserView::sizeHint() const
{
    return {500, 500};
}

qt3dsdm::CDataModelHandle EventsBrowserView::selectedHandle() const
{
    const auto handleId = m_model->handleForRow(m_selection);
    return handleId;
}

void EventsBrowserView::selectAndExpand(const QString &event)
{
    // All categories are expanded by default, so let's just select
    setSelection(m_model->rowForEventName(event));
}

void EventsBrowserView::setSelection(int index)
{
    auto handleId = m_model->handleForRow(index);
    if (!handleId.Valid()) {
        m_selection = -1;
        Q_EMIT selectionChanged();
    } else if (m_selection != index) {
        m_selection = index;
        Q_EMIT selectionChanged();
    }
}

void EventsBrowserView::focusOutEvent(QFocusEvent *event)
{
    QQuickWidget::focusOutEvent(event);
    QTimer::singleShot(0, this, &EventsBrowserView::close);
}

void EventsBrowserView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty("_eventsBrowserView"_L1, this);
    rootContext()->setContextProperty("_resDir"_L1,
                                      resourceImageUrl());
    engine()->addImportPath(qmlImportPath());
    setSource(QUrl("qrc:/Palettes/Action/EventsBrowser.qml"_L1));
}

