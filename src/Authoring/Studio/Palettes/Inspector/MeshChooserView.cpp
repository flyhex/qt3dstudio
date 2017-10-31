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
    setWindowTitle(tr("Meshes"));
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &MeshChooserView::initialize);
}

void MeshChooserView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty("_resDir"_L1,
                                      resourceImageUrl());
    rootContext()->setContextProperty("_meshChooserView"_L1, this);
    rootContext()->setContextProperty("_meshChooserModel"_L1, m_model);

    engine()->addImportPath(qmlImportPath());
    setSource(QUrl("qrc:/Palettes/Inspector/MeshChooser.qml"_L1));
}

QSize MeshChooserView::sizeHint() const
{
    return {500, 500};
}

void MeshChooserView::setSelectedMeshName(const QString &name)
{
    hide();
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

void MeshChooserView::focusOutEvent(QFocusEvent *event)
{
    QQuickWidget::focusOutEvent(event);
    QTimer::singleShot(0, this, &MeshChooserView::close);
}

void MeshChooserView::showEvent(QShowEvent *event)
{
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();

    qt3dsdm::SValue value;
    propertySystem->GetInstancePropertyValue(m_instance, m_handle, value);

    const QString meshValue = qt3dsdm::get<QString>(value);
    const Q3DStudio::CFilePath selectionItem = Q3DStudio::CFilePath::fromQString(meshValue);
    const Q3DStudio::CFilePath selectionWithoutId = selectionItem.GetPathWithoutIdentifier();

    QString currentFile;

    if (selectionWithoutId.size()) {
        currentFile = selectionWithoutId.toQString();
    } else {
        currentFile = selectionItem.GetIdentifier().toQString();
    }

    m_model->setCurrentFile(currentFile);

    QQuickWidget::showEvent(event);
}
