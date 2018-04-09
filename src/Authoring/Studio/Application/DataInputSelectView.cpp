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
#include <QtCore/qtimer.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>

#include "DataInputSelectView.h"
#include "StudioPreferences.h"
#include "StudioUtils.h"
#include "Literals.h"
#include "MainFrm.h"
#include "DataInputListDlg.h"
#include "StudioApp.h"

DataInputSelectView::DataInputSelectView(QWidget *parent)
    : QQuickWidget(parent)
    , m_model(new DataInputSelectModel(this))
{
    setWindowTitle(tr("Datainputs"));
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &DataInputSelectView::initialize);
}

void DataInputSelectView::setData(const QStringList &dataInputList,
                                  const QString &currentController,
                                  int handle, int instance)
{
    m_handle = handle;
    m_instance = instance;
    m_currController = currentController;
    updateData(dataInputList);
}

void DataInputSelectView::updateData(const QStringList &dataInputList)
{
    m_model->setFixedItemCount(0);
    QStringList dataInputs = dataInputList;
    m_model->stringList().clear();

    dataInputs.prepend(getAddNewDataInputString());
    m_model->setFixedItemCount(m_model->getFixedItemCount() + 1);

    dataInputs.prepend(getNoneString());
    m_model->setFixedItemCount(m_model->getFixedItemCount() + 1);
    m_selection = dataInputs.indexOf(m_currController);
    m_model->setStringList(dataInputs);
}

void DataInputSelectView::showEvent(QShowEvent *event)
{
    QQuickWidget::showEvent(event);
}

void DataInputSelectView::setSelection(int index)
{
    if (m_selection != index) {
        m_selection = index;
        QString sel = m_model->index(m_selection, 0).data().toString();
        if (sel != getAddNewDataInputString()) {
            Q_EMIT dataInputChanged(m_handle, m_instance, sel);
        } else {
            CDataInputListDlg dataInputDlg(&(g_StudioApp.m_dataInputDialogItems));
            dataInputDlg.exec();

            if (dataInputDlg.result() == QDialog::Accepted)
                g_StudioApp.SaveUIAFile(false);
        }
        QTimer::singleShot(0, this, &DataInputSelectView::close);
    }
}

void DataInputSelectView::focusOutEvent(QFocusEvent *event)
{
    QQuickWidget::focusOutEvent(event);
    QTimer::singleShot(0, this, &DataInputSelectView::close);
}

void DataInputSelectView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty("_resDir"_L1,
                                      resourceImageUrl());
    rootContext()->setContextProperty("_dataInputSelectView"_L1, this);
    rootContext()->setContextProperty("_dataInputSelectModel"_L1, m_model);
    engine()->addImportPath(qmlImportPath());
    setSource(QUrl("qrc:/Palettes/Inspector/DataInputChooser.qml"_L1));
}
