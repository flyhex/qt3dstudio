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
#include "StudioPreferences.h"

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
    rootContext()->setContextProperty("_resDir"_L1,
                                      resourceImageUrl());
    rootContext()->setContextProperty("_imageChooserView"_L1, this);
    rootContext()->setContextProperty("_imageChooserModel"_L1, m_model);
    engine()->addImportPath(qmlImportPath());
    setSource(QUrl("qrc:/Palettes/Inspector/ImageChooser.qml"_L1));
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

bool ImageChooserView::isFocused() const
{
    return hasFocus();
}

void ImageChooserView::focusInEvent(QFocusEvent *event)
{
    QQuickWidget::focusOutEvent(event);
    emit focusChanged();
}

void ImageChooserView::focusOutEvent(QFocusEvent *event)
{
    QQuickWidget::focusOutEvent(event);
    emit focusChanged();
    QTimer::singleShot(0, this, &QQuickWidget::close);
}

void ImageChooserView::showEvent(QShowEvent *event)
{
    const auto doc = g_StudioApp.GetCore()->GetDoc();
    const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();

    qt3dsdm::SValue value;
    propertySystem->GetInstancePropertyValue(m_instance, m_handle, value);

    const auto guid = qt3dsdm::get<qt3dsdm::SLong4>(value);

    const auto imageInstance = doc->GetDocumentReader().GetInstanceForGuid(guid);
    if (imageInstance.Valid()) {
        const QString path = doc->GetDocumentReader().GetSourcePath(imageInstance);

        // If path is renderable id, we need to resolve the actual path
        const QString renderablePath = g_StudioApp.getRenderableAbsolutePath(path);

        if (renderablePath.isEmpty())
            m_model->setCurrentFile(path);
        else
            m_model->setCurrentFile(renderablePath);
    } else {
        m_model->setCurrentFile(tr("[None]"));
    }

    QQuickWidget::showEvent(event);
}
