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
    , m_matchingTypes(acceptedTypes)
    , m_dataInputList(QVector<QPair<QString, int>>())
{
    if (!m_matchingTypes.isEmpty())
        m_defaultType = m_matchingTypes[0];

    setWindowTitle(tr("Datainputs"));
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    QTimer::singleShot(0, this, &DataInputSelectView::initialize);
}

DataInputSelectView::~DataInputSelectView()
{
}

void DataInputSelectView::setData(const QVector<QPair<QString, int>> &dataInputList,
                                  const QString &currentController, int handle, int instance)
{
    m_handle = handle;
    m_instance = instance;
    m_currController = currentController;
    m_dataInputList = dataInputList;
    updateData();
}

void DataInputSelectView::setMatchingTypes(const QVector<EDataType> &matchingTypes)
{
    m_matchingTypes = matchingTypes;
    if (!m_matchingTypes.isEmpty())
        m_defaultType = m_matchingTypes[0];
    updateData();
}

void DataInputSelectView::updateData()
{
    m_selection = -1;

    m_model->setFixedItemCount(0);
    QVector<QPair<QString, QString>> dataInputs;
    m_model->clear();

    if (m_model->getShowFixedItems()) {
        dataInputs.append({getAddNewDataInputString(), {}});
        m_model->setFixedItemCount(m_model->getFixedItemCount() + 1);

        dataInputs.append({getNoneString(), {}});
        m_model->setFixedItemCount(m_model->getFixedItemCount() + 1);
    }

    for (auto &i : qAsConst(m_dataInputList)) {
        bool isCurrentCtrlr = i.first == m_currController;
        if (i.first.contains(m_searchString) || !m_searchString.size() || isCurrentCtrlr) {
            if (m_typeFilter == DataInputTypeFilter::AllTypes
                || (m_typeFilter == DataInputTypeFilter::MatchingTypes
                    && m_matchingTypes.contains((EDataType)i.second))
                || m_typeFilter == (EDataType)i.second || isCurrentCtrlr) {
                dataInputs.append({i.first, getDiTypeStr(i.second)});
                if (isCurrentCtrlr) {
                    m_selection = dataInputs.size() - 1;
                    dataInputs.last().first.append(tr(" (Current)"));
                }
            }
        }
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

void DataInputSelectView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        QTimer::singleShot(0, this, &DataInputSelectView::close);

    QQuickWidget::keyPressEvent(event);
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
                                           m_defaultType, m_matchingTypes);
            dataInputDlg.exec();

            if (dataInputDlg.result() == QDialog::Accepted) {
                m_mostRecentlyAdded = dataInputDlg.getAddedDataInput();
                if (m_mostRecentlyAdded.size()) {
                    CDataInputDialogItem *diItem = g_StudioApp.m_dataInputDialogItems.value(
                                m_mostRecentlyAdded);
                    if (m_matchingTypes.isEmpty()
                        || (diItem && m_matchingTypes.contains(
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

void DataInputSelectView::setSearchString(const QString &string)
{
    m_searchString = string;
    updateData();
}

void DataInputSelectView::setTypeFilter(const int index)
{
    m_typeFilter = index;
    updateData();
    Q_EMIT filterChanged();
}

void DataInputSelectView::focusOutEvent(QFocusEvent *event)
{
    QQuickWidget::focusOutEvent(event);
    QTimer::singleShot(0, this, &DataInputSelectView::close);
}

bool DataInputSelectView::toolTipsEnabled() const
{
    return CStudioPreferences::isTooltipsOn();
}

void DataInputSelectView::setCurrentController(const QString &currentController)
{
    m_currController = currentController;
    // Need to update the entire data as being current controller affects if the item is visible
    updateData();
}

void DataInputSelectView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty(QStringLiteral("_resDir"),
                                      StudioUtils::resourceImageUrl());
    rootContext()->setContextProperty(QStringLiteral("_parentView"), this);
    rootContext()->setContextProperty(QStringLiteral("_dataInputSelectModel"), m_model);
    engine()->addImportPath(StudioUtils::qmlImportPath());
    setSource(QUrl(QStringLiteral("qrc:/Palettes/Inspector/DataInputChooser.qml")));
}
