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

#include "ObjectReferenceView.h"
#include "Literals.h"
#include "StudioUtils.h"
#include "StudioPreferences.h"
#include "IDocumentEditor.h"
#include "Qt3DSDMValue.h"
#include "Core.h"
#include "Doc.h"
#include "StudioApp.h"

#include <QtCore/qtimer.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>

ObjectReferenceView::ObjectReferenceView(QWidget *parent) : QQuickWidget(parent)
{
    setWindowTitle(tr("Object Reference"));
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &ObjectReferenceView::initialize);
}

void ObjectReferenceView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty("_resDir"_L1,
                                      resourceImageUrl());
    rootContext()->setContextProperty("_objectReferenceView"_L1, this);
    engine()->addImportPath(qmlImportPath());
    setSource(QUrl("qrc:/Studio/Palettes/Inspector/ObjectReference.qml"_L1));
}

QSize ObjectReferenceView::sizeHint() const
{
    return {500, 500};
}

QAbstractItemModel *ObjectReferenceView::model() const
{
    return m_model;
}

void ObjectReferenceView::setModel(QAbstractItemModel *model)
{
    if (m_model != model) {
        m_model = model;
        Q_EMIT modelChanged();
    }
}

void ObjectReferenceView::setHandle(int handle)
{
    m_handle = handle;
}

void ObjectReferenceView::setInstance(int instance)
{
    m_instance = instance;
}

void ObjectReferenceView::setSelectedReference(const QString &name)
{
    qt3dsdm::SValue v = QVariant(name);
    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(), QObject::tr("Set Property"))
            ->SetInstancePropertyValue(m_instance, m_handle, v);
}

void ObjectReferenceView::focusOutEvent(QFocusEvent *event)
{
    QQuickWidget::focusOutEvent(event);
    close();
}

