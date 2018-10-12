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
#include "DataInputDlg.h"
#include "StudioApp.h"

// Empty acceptedTypes vector means all types are accepted
DataInputSelectView::DataInputSelectView(const QVector<EDataType> &acceptedTypes, QWidget *parent)
    : QQuickWidget(parent)
    , m_model(new DataInputSelectModel(this))
    , m_defaultType(EDataType::DataTypeFloat)
    , m_acceptedTypes(acceptedTypes)

{
    if (!m_acceptedTypes.isEmpty())
        m_defaultType = m_acceptedTypes[0];

    setWindowTitle(tr("Datainputs"));
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &DataInputSelectView::initialize);
}

void DataInputSelectView::setData(const QVector<QPair<QString, int>> &dataInputList,
                                  const QString &currentController, int handle, int instance)
{
    m_handle = handle;
    m_instance = instance;
    m_currController = currentController;
    updateData(dataInputList);
}

void DataInputSelectView::setAcceptedTypes(const QVector<EDataType> &acceptedTypes)
{
    m_acceptedTypes = acceptedTypes;
    if (!m_acceptedTypes.isEmpty())
        m_defaultType = m_acceptedTypes[0];
}

void DataInputSelectView::updateData(const QVector<QPair<QString, int>> &dataInputList)
{
    m_selection = -1;

    m_model->setFixedItemCount(0);
    QVector<QPair<QString, QString>> dataInputs;
    m_model->clear();

    dataInputs.append({getAddNewDataInputString(), {}});
    m_model->setFixedItemCount(m_model->getFixedItemCount() + 1);

    dataInputs.append({getNoneString(), {}});
    m_model->setFixedItemCount(m_model->getFixedItemCount() + 1);

    for (auto i : dataInputList) {
        dataInputs.append({i.first, getDiTypeStr(i.second)});
        if (i.first == m_currController)
            m_selection = dataInputs.size() - 1;
    }

    m_model->setData(dataInputs);
}

QString DataInputSelectView::getDiTypeStr(int type)
{
    switch (static_cast<EDataType>(type)) {
    case EDataType::DataTypeBoolean:
        return tr("Boolean");
        break;
#ifdef DATAINPUT_EVALUATOR_ENABLED
    case EDataType::DataTypeEvaluator:
        return tr("Evaluator");
        break;
#endif
    case EDataType::DataTypeFloat:
        return tr("Float");
        break;
    case EDataType::DataTypeRangedNumber:
        return tr("Ranged Number");
        break;
    case EDataType::DataTypeString:
        return tr("String");
        break;
    case EDataType::DataTypeVariant:
        return tr("Variant");
        break;
    case EDataType::DataTypeVector2:
        return tr("Vector2");
        break;
    case EDataType::DataTypeVector3:
        return tr("Vector3");
        break;
    default:
        return {};
        Q_ASSERT(false);
        break;
    }
}

void DataInputSelectView::showEvent(QShowEvent *event)
{
    QQuickWidget::showEvent(event);
}

void DataInputSelectView::setSelection(int index)
{
    if (m_selection != index) {
        m_selection = index;
        QString sel = m_model->data(m_model->index(index), Qt::DisplayRole).toString();
        if (sel != getAddNewDataInputString()) {
            // do not set the value if it has not changed
            if (sel != m_currController && !(sel == getNoneString() && !m_currController.size())) {
                Q_EMIT dataInputChanged(m_handle, m_instance, sel);
                Q_EMIT selectedChanged();
            }
        } else {
            CDataInputListDlg dataInputDlg(&g_StudioApp.m_dataInputDialogItems, true, nullptr,
                                           m_defaultType, m_acceptedTypes);
            dataInputDlg.exec();

            if (dataInputDlg.result() == QDialog::Accepted) {
                m_mostRecentlyAdded = dataInputDlg.getAddedDataInput();
                if (m_mostRecentlyAdded.size()) {
                    CDataInputDialogItem *diItem = g_StudioApp.m_dataInputDialogItems.value(
                                m_mostRecentlyAdded);
                    if (m_acceptedTypes.isEmpty()
                            || (diItem && m_acceptedTypes.contains(
                                    static_cast<EDataType>(diItem->type)))) {
                        Q_EMIT dataInputChanged(m_handle, m_instance, m_mostRecentlyAdded);
                    }
                }
                g_StudioApp.saveDataInputsToProjectFile();
            }
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
