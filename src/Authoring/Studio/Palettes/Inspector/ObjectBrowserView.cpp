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
#include "ObjectBrowserView.h"

#include "CColor.h"
#include "Literals.h"
#include "ObjectListModel.h"
#include "StudioPreferences.h"
#include "StudioUtils.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qtimer.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>

ObjectBrowserView::ObjectBrowserView(QWidget *parent)
    : QQuickWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &ObjectBrowserView::initialize);
}

QAbstractItemModel *ObjectBrowserView::model() const
{
    return m_model;
}

void ObjectBrowserView::setModel(ObjectListModel *model)
{
    if (!m_model) {
        m_model = new FlatObjectListModel(model, this);
    }
    m_model->setSourceModel(model);
    m_ownerInstance = 0;
    m_selection = -1;

    Q_EMIT modelChanged();
}


QSize ObjectBrowserView::sizeHint() const
{
    return {500, 500};
}

QString ObjectBrowserView::absPath(int index) const
{
    return m_model->index(index, 0).data(ObjectListModel::AbsolutePathRole).toString();
}

QString ObjectBrowserView::relPath(int index) const
{
    return m_model->data(
        m_model->index(index),
        m_model->sourceModel()->indexForHandle(m_ownerInstance),
        ObjectListModel::PathReferenceRole).toString();
}

bool ObjectBrowserView::selectable(int index) const
{
    auto handleId = m_model->index(index, 0).data(ObjectListModel::HandleRole).toInt();
    auto handle = qt3dsdm::Qt3DSDMInstanceHandle(handleId);
    return m_model->sourceModel()->selectable(handle);
}

void ObjectBrowserView::selectAndExpand(const qt3dsdm::Qt3DSDMInstanceHandle &handle,
                                        const qt3dsdm::Qt3DSDMInstanceHandle &owner)
{
    m_ownerInstance = owner;
    QModelIndex index = m_model->sourceModel()->indexForHandle(handle);
    if (!index.isValid())
        return;
    m_model->expandTo(QModelIndex(), index);
    setSelection(m_model->rowForSourceIndex(index));
}

void ObjectBrowserView::setSelection(int index)
{
    if (m_selection != index) {
        m_selection = index;
        Q_EMIT selectionChanged();
    }
}

void ObjectBrowserView::setPathType(ObjectBrowserView::PathType type)
{
    if (type != m_pathType) {
        m_pathType = type;
        Q_EMIT pathTypeChanged();
    }
}

qt3dsdm::Qt3DSDMInstanceHandle ObjectBrowserView::selectedHandle() const
{
    auto handleId = m_model->index(m_selection, 0).data(ObjectListModel::HandleRole).toInt();
    return qt3dsdm::Qt3DSDMInstanceHandle(handleId);
}

void ObjectBrowserView::focusOutEvent(QFocusEvent *event)
{
    QQuickWidget::focusOutEvent(event);
    QTimer::singleShot(0, this, &ObjectBrowserView::close);
}

void ObjectBrowserView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty("_objectBrowserView"_L1, this);
    rootContext()->setContextProperty("_resDir"_L1, resourceImageUrl());
    qmlRegisterUncreatableType<ObjectBrowserView>("Qt3DStudio", 1, 0, "ObjectBrowserView"
                                                  , tr("Creation of ObjectBrowserView not allowed from QML"));
    engine()->addImportPath(qmlImportPath());
    setSource(QUrl("qrc:/Palettes/Inspector/ObjectBrowser.qml"_L1));
}
