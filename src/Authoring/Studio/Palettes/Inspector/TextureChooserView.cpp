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

#include "TextureChooserView.h"
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
#include "StudioPreferences.h"

#include <QtCore/qtimer.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>

TextureChooserView::TextureChooserView(QWidget *parent)
    : QQuickWidget(parent)
    , m_model(new ImageChooserModel(false, this))
{
    setWindowTitle(tr("Texture"));
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &TextureChooserView::initialize);
}

void TextureChooserView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty(QStringLiteral("_resDir"), StudioUtils::resourceImageUrl());
    rootContext()->setContextProperty(QStringLiteral("_textureChooserView"), this);
    rootContext()->setContextProperty(QStringLiteral("_textureChooserModel"), m_model);
    engine()->addImportPath(StudioUtils::qmlImportPath());
    setSource(QUrl(QStringLiteral("qrc:/Palettes/Inspector/TextureChooser.qml")));
}

void TextureChooserView::setHandle(int handle)
{
    m_handle = handle;
}

int TextureChooserView::handle() const
{
    return m_handle;
}

void TextureChooserView::setInstance(int instance)
{
    m_instance = instance;
}

int TextureChooserView::instance() const
{
    return m_instance;
}

QString TextureChooserView::currentDataModelPath() const
{
    QString cleanPath;
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();

    qt3dsdm::SValue value;
    propertySystem->GetInstancePropertyValue(m_instance, m_handle, value);

    const QString currentValue = qt3dsdm::get<QString>(value);
    // An empty value can sometimes be represented by a relative path either to project root or the
    // presentation file, such as"./" or "../", so let's just consider all directory paths as empty
    if (currentValue.isEmpty() || QFileInfo(currentValue).isDir()) {
        cleanPath = ChooserModelBase::noneString();
    } else {
        cleanPath = QDir::cleanPath(QDir(doc->GetDocumentDirectory().toQString())
                                    .filePath(currentValue));
    }
    return cleanPath;
}

void TextureChooserView::focusOutEvent(QFocusEvent *event)
{
    QQuickWidget::focusOutEvent(event);
    QTimer::singleShot(0, this, &TextureChooserView::close);
}

void TextureChooserView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        QTimer::singleShot(0, this, &TextureChooserView::close);

    QQuickWidget::keyPressEvent(event);
}

void TextureChooserView::showEvent(QShowEvent *event)
{
    m_model->setCurrentFile(currentDataModelPath());

    QQuickWidget::showEvent(event);
}
