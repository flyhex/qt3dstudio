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

#include "BasicObjectsView.h"
#include "BasicObjectsModel.h"
#include "CColor.h"
#include "Literals.h"
#include "StudioPreferences.h"
#include "StudioUtils.h"
#include "StudioApp.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qtimer.h>
#include <QtGui/qdrag.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlfile.h>
#include <QtQuick/qquickitem.h>

BasicObjectsView::BasicObjectsView(QWidget *parent) : QQuickWidget(parent)
  , m_ObjectsModel(new BasicObjectsModel(this))

{
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &BasicObjectsView::initialize);
}

QSize BasicObjectsView::sizeHint() const
{
    return {150, 500};
}

void BasicObjectsView::startDrag(QQuickItem *item, int row)
{
    const auto index = m_ObjectsModel->index(row);

    QDrag drag(this);
    drag.setMimeData(m_ObjectsModel->mimeData({index}));
    drag.setPixmap(QPixmap(QQmlFile::urlToLocalFileOrQrc(
                               index.data(BasicObjectsModel::IconRole).toUrl())));
    drag.exec(Qt::CopyAction);
    QTimer::singleShot(0, item, &QQuickItem::ungrabMouse);
}

void BasicObjectsView::mousePressEvent(QMouseEvent *event)
{
    g_StudioApp.setLastActiveView(this);
    QQuickWidget::mousePressEvent(event);
}

void BasicObjectsView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty("_basicObjectsModel"_L1, m_ObjectsModel);
    rootContext()->setContextProperty("_basicObjectsView"_L1, this);
    rootContext()->setContextProperty("_resDir"_L1, resourceImageUrl());

    engine()->addImportPath(qmlImportPath());
    setSource(QUrl("qrc:/Palettes/BasicObjects/BasicObjectsView.qml"_L1));
}
