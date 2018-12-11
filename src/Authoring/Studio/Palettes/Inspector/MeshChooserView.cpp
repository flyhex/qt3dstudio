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

#include "MeshChooserView.h"
#include "MeshChooserModel.h"
#include "Literals.h"
#include "StudioUtils.h"
#include "StudioPreferences.h"
#include "IDocumentEditor.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMValue.h"
#include "Core.h"
#include "Doc.h"
#include "StudioApp.h"
#include "IDocumentBufferCache.h"

#include <QtCore/qtimer.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>

MeshChooserView::MeshChooserView(QWidget *parent)
    : QQuickWidget(parent)
    , m_model(new MeshChooserModel(this))
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &MeshChooserView::initialize);
}

void MeshChooserView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty(QStringLiteral("_resDir"),
                                      StudioUtils::resourceImageUrl());
    rootContext()->setContextProperty(QStringLiteral("_meshChooserView"), this);
    rootContext()->setContextProperty(QStringLiteral("_meshChooserModel"), m_model);

    engine()->addImportPath(StudioUtils::qmlImportPath());
    setSource(QUrl(QStringLiteral("qrc:/Palettes/Inspector/MeshChooser.qml")));
}

int MeshChooserView::numMeshes() const
{
    return m_model->rowCount();
}

void MeshChooserView::setSelectedMeshName(const QString &name)
{
    bool resourceName = false;
    QString meshName = QLatin1Char('#') + name;
    const wchar_t **files = g_StudioApp.GetCore()->GetDoc()->GetBufferCache().GetPrimitiveNames();
    for (const wchar_t **item = files; item && *item; ++item) {
        QString primitive = QString::fromWCharArray(*item);
        if (primitive == meshName) {
            resourceName = true;
            break;
        }
    }
    if (!resourceName)
        meshName = name;

    Q_EMIT meshSelected(m_handle, m_instance, meshName);
}

void MeshChooserView::setHandle(int handle)
{
    m_handle = handle;
}

void MeshChooserView::setInstance(int instance)
{
    m_instance = instance;
}

void MeshChooserView::updateSelection()
{
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();

    qt3dsdm::SValue value;
    propertySystem->GetInstancePropertyValue(m_instance, m_handle, value);

    QString currentFile;
    const QString meshValue = qt3dsdm::get<QString>(value);
    if (meshValue.startsWith(QLatin1Char('#'))) {
        currentFile = meshValue.mid(1);
    } else {
        currentFile = QDir::cleanPath(QDir(doc->GetDocumentDirectory().toQString())
                                      .filePath(meshValue));
    }

    m_model->setCurrentFile(currentFile);
}

bool MeshChooserView::isFocused() const
{
    return hasFocus();
}

void MeshChooserView::focusInEvent(QFocusEvent *event)
{
    QQuickWidget::focusInEvent(event);
    emit focusChanged();
}

void MeshChooserView::focusOutEvent(QFocusEvent *event)
{
    QQuickWidget::focusOutEvent(event);
    emit focusChanged();
    QTimer::singleShot(0, this, &QQuickWidget::close);
}

void MeshChooserView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        QTimer::singleShot(0, this, &MeshChooserView::close);

    QQuickWidget::keyPressEvent(event);
}

void MeshChooserView::showEvent(QShowEvent *event)
{
    updateSelection();
    QQuickWidget::showEvent(event);
}
