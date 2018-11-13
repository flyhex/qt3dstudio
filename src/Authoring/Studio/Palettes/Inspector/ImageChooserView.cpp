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

#include "ImageChooserView.h"
#include "ImageChooserModel.h"
#include "StudioPreferences.h"
#include "Literals.h"
#include "StudioUtils.h"
#include "IDocumentEditor.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMValue.h"
#include "Core.h"
#include "Doc.h"
#include "StudioApp.h"

#include <QtCore/qtimer.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>

ImageChooserView::ImageChooserView(QWidget *parent)
    : QQuickWidget(parent)
    , m_model(new ImageChooserModel(true, this))
{
    setWindowTitle(tr("Images"));
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &ImageChooserView::initialize);
}

void ImageChooserView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty(QStringLiteral("_resDir"),
                                      StudioUtils::resourceImageUrl());
    rootContext()->setContextProperty(QStringLiteral("_imageChooserView"), this);
    rootContext()->setContextProperty(QStringLiteral("_imageChooserModel"), m_model);
    engine()->addImportPath(StudioUtils::qmlImportPath());
    setSource(QUrl(QStringLiteral("qrc:/Palettes/Inspector/ImageChooser.qml")));
}

QSize ImageChooserView::sizeHint() const
{
    return {500, 500};
}

void ImageChooserView::setHandle(int handle)
{
    m_handle = handle;
}

int ImageChooserView::handle() const
{
    return m_handle;
}

void ImageChooserView::setInstance(int instance)
{
    m_instance = instance;
}

int ImageChooserView::instance() const
{
    return m_instance;
}

QString ImageChooserView::currentDataModelPath() const
{
    QString cleanPath;
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();

    qt3dsdm::SValue value;
    propertySystem->GetInstancePropertyValue(m_instance, m_handle, value);

    const auto guid = qt3dsdm::get<qt3dsdm::SLong4>(value);

    const auto imageInstance = doc->GetDocumentReader().GetInstanceForGuid(guid);
    if (imageInstance.Valid()) {
        const QString path = doc->GetDocumentReader().GetSourcePath(imageInstance).toQString();

        // If path is renderable id, we need to resolve the actual path
        const QString renderablePath = g_StudioApp.getRenderableAbsolutePath(path);

        if (renderablePath.isEmpty())
            cleanPath = path;
        else
            cleanPath = renderablePath;

        cleanPath = QDir::cleanPath(
                    QDir(doc->GetDocumentDirectory().toQString()).filePath(cleanPath));
    } else {
        cleanPath = ChooserModelBase::noneString();
    }

    return cleanPath;
}

bool ImageChooserView::isFocused() const
{
    return hasFocus();
}

void ImageChooserView::focusInEvent(QFocusEvent *event)
{
    QQuickWidget::focusInEvent(event);
    emit focusChanged();
}

void ImageChooserView::focusOutEvent(QFocusEvent *event)
{
    QQuickWidget::focusOutEvent(event);
    emit focusChanged();
    QTimer::singleShot(0, this, &QQuickWidget::close);
}

void ImageChooserView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        QTimer::singleShot(0, this, &ImageChooserView::close);

    QQuickWidget::keyPressEvent(event);
}

void ImageChooserView::showEvent(QShowEvent *event)
{
    m_model->setCurrentFile(currentDataModelPath());
    QQuickWidget::showEvent(event);
}
