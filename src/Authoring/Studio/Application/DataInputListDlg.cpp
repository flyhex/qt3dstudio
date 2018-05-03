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

#include "DataInputListDlg.h"
#include "ui_DataInputListDlg.h"
#include "DataInputDlg.h"

#include <QtWidgets/qpushbutton.h>
#include <QtGui/qstandarditemmodel.h>
#include <QtGui/qevent.h>
#include <algorithm>
#include <QtCore/qtimer.h>

const int columnCount = 3;

CDataInputListDlg::CDataInputListDlg(QVector<CDataInputDialogItem *> *datainputs,
                                     bool goToAdd, QWidget *parent)
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
    , m_ui(new Ui::DataInputListDlg)
    , m_actualDataInputs(datainputs)
    , m_goToAdd(goToAdd)
    , m_currentDataInputIndex(-1)
    , m_tableContents(new QStandardItemModel(0, columnCount, this))
    , m_sortColumn(-1)
{
    m_ui->setupUi(this);

    // Create icon buttons. Give them object and accessible names so their style can be modified
    // via stylesheet.
    QPushButton *addButton = new QPushButton(this);
    addButton->setIcon(QIcon(":/images/add.png"));
    addButton->setAccessibleName(QStringLiteral("DataInputListButton"));
    addButton->setObjectName(QStringLiteral("DataInputListButton"));
    QPushButton *editButton = new QPushButton(this);
    editButton->setIcon(QIcon(":/images/Objects-edit-disabled.png"));
    editButton->setAccessibleName(QStringLiteral("DataInputListButton"));
    editButton->setObjectName(QStringLiteral("DataInputListButton"));
    QPushButton *removeButton = new QPushButton(this);
    removeButton->setIcon(QIcon(":/images/Action-Trash-Disabled.png"));
    removeButton->setAccessibleName(QStringLiteral("DataInputListButton"));
    removeButton->setObjectName(QStringLiteral("DataInputListButton"));

    m_ui->buttonBoxAddEditRemove->addButton(addButton, QDialogButtonBox::ActionRole);
    m_ui->buttonBoxAddEditRemove->addButton(editButton, QDialogButtonBox::ActionRole);
    m_ui->buttonBoxAddEditRemove->addButton(removeButton, QDialogButtonBox::ActionRole);
    QList<QAbstractButton *>buttons = m_ui->buttonBoxAddEditRemove->buttons();
    connect(buttons.at(0), &QAbstractButton::clicked, this, &CDataInputListDlg::onAddDataInput);
    connect(buttons.at(1), &QAbstractButton::clicked, this, &CDataInputListDlg::onEditDataInput);
    connect(buttons.at(2), &QAbstractButton::clicked, this, &CDataInputListDlg::onRemoveDataInput);

    buttons[0]->setToolTip(tr("Add New Data Input..."));
    buttons[1]->setToolTip(tr("Edit Data Input..."));
    buttons[2]->setToolTip(tr("Remove Data Input"));

    initDialog();
}

CDataInputListDlg::~CDataInputListDlg()
{
    delete m_ui;
}

void CDataInputListDlg::initDialog()
{
    // Copy given list to our internal one. We want to commit to the changes only after "Ok"
    // has been pressed.
    for (int i = 0; i < m_actualDataInputs->count(); ++i)
        m_dataInputs.append(m_actualDataInputs->at(i));

    // Check available list. If there are none, disable "Remove" and "Edit" buttons
    updateButtons();

    // Update table contents
    updateContents();

    // Make the expression column wider than name and type
    m_ui->tableView->horizontalHeader()->setStretchLastSection(true);
    m_ui->tableView->horizontalHeader()->setMinimumSectionSize(125);

    // Align columns left and prevent selecting the whole column
    m_ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    connect(m_ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CDataInputListDlg::onSelectionChanged);
    connect(m_ui->tableView, &QTableView::activated, this, &CDataInputListDlg::onActivated);
    connect(m_ui->tableView->horizontalHeader(), &QHeaderView::sortIndicatorChanged,
            this, &CDataInputListDlg::onSortOrderChanged);

    // Directly show data input modification dialog
    if (m_goToAdd)
         QTimer::singleShot(0, this, &CDataInputListDlg::onAddDataInput);
}

void CDataInputListDlg::updateButtons()
{
    if (m_ui->tableView->selectionModel()
            && m_ui->tableView->selectionModel()->selectedIndexes().size() > 0) {
        m_ui->buttonBoxAddEditRemove->buttons()[2]->setEnabled(true);
        m_ui->buttonBoxAddEditRemove->buttons()[2]->setIcon(
                    QIcon(":/images/Action-Trash-Normal.png"));
    } else {
        m_ui->buttonBoxAddEditRemove->buttons()[2]->setEnabled(false);
        m_ui->buttonBoxAddEditRemove->buttons()[2]->setIcon(
                    QIcon(":/images/Action-Trash-Disabled.png"));
    }
    if (m_dataInputs.isEmpty() || m_currentDataInputIndex == -1) {
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setEnabled(false);
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setIcon(
                    QIcon(":/images/Objects-edit-disabled.png"));
    } else {
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setEnabled(true);
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setIcon(
                    QIcon(":/images/Objects-edit-normal.png"));
    }
}

void CDataInputListDlg::updateContents()
{
    m_tableContents->clear();

    QStringList labels;
    labels << tr("Name") << tr("Input Type") << tr("Expression");
    m_tableContents->setHorizontalHeaderLabels(labels);

    QList<QStandardItem *> dataInput;
    for (int i = 0; i < m_dataInputs.count(); ++i) {
        dataInput.clear();
        dataInput.append(new QStandardItem(m_dataInputs.at(i)->name));
        int dataInputType = m_dataInputs.at(i)->type;
        if (dataInputType == DataTypeRangedNumber) {
            dataInput.append(new QStandardItem(tr("Ranged Number")));
            QString expression = QStringLiteral("[ ")
                    + QString::number(m_dataInputs.at(i)->minValue)
                    + QStringLiteral(" ... ")
                    + QString::number(m_dataInputs.at(i)->maxValue)
                    + QStringLiteral(" ]");
            dataInput.append(new QStandardItem(expression));
        } else if (dataInputType == DataTypeString) {
            dataInput.append(new QStandardItem(tr("String")));
        } else if (dataInputType == DataTypeFloat) {
            dataInput.append(new QStandardItem(tr("Float")));
        } else if (dataInputType == DataTypeEvaluator) {
            dataInput.append(new QStandardItem(tr("Evaluator")));
            dataInput.append(new QStandardItem(m_dataInputs.at(i)->valueString));
        } else if (dataInputType == DataTypeBoolean) {
            dataInput.append(new QStandardItem(tr("Boolean")));
        } else if (dataInputType == DataTypeVector3) {
            dataInput.append(new QStandardItem(tr("Vector3")));
        } else if (dataInputType == DataTypeVector2) {
            dataInput.append(new QStandardItem(tr("Vector2")));
        } else if (dataInputType == DataTypeVariant) {
            dataInput.append(new QStandardItem(tr("Variant")));
        }

        m_tableContents->appendRow(dataInput);
    }

    m_ui->tableView->setModel(m_tableContents);

    if (m_sortColumn >= 0)
        m_ui->tableView->sortByColumn(m_sortColumn, m_sortOrder);
}

void CDataInputListDlg::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Delete)) {
        onRemoveDataInput();
    } else if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)) {
        // Eat enter if we have selections
        const QModelIndexList indexes = m_ui->tableView->selectionModel()->selectedIndexes();
        if (indexes.size() > 0)
            event->accept();
        else
            QDialog::keyPressEvent(event);
    } else {
        QDialog::keyPressEvent(event);
    }
}

void CDataInputListDlg::on_buttonBox_accepted()
{
    m_actualDataInputs->clear();

    for (int i = 0; i < m_dataInputs.count(); ++i)
        m_actualDataInputs->append(m_dataInputs.at(i));

    QDialog::accept();
}

void CDataInputListDlg::on_buttonBox_rejected()
{
    QDialog::reject();
}

void CDataInputListDlg::onAddDataInput()
{
    // Create a new data input dialog item and give it to dialog
    CDataInputDialogItem *dataInput = new CDataInputDialogItem();
    CDataInputDlg datainputdialog(&dataInput, m_tableContents, this);
    if (datainputdialog.exec() == QDialog::Accepted)
        m_dataInputs.append(dataInput);

    updateButtons();
    updateContents();
}

void CDataInputListDlg::onRemoveDataInput()
{
    QVector<int> removedRows;
    if (m_ui->tableView->selectionModel()) {
        const QModelIndexList indexes = m_ui->tableView->selectionModel()->selectedIndexes();
        for (const auto index : indexes) {
            if (!removedRows.contains(index.row()))
                removedRows.append(index.row());
        }

        if (removedRows.size() > 0) {
            std::sort(removedRows.begin(), removedRows.end());
            for (int i = removedRows.size() - 1; i >= 0; --i)
                m_dataInputs.removeAt(removedRows[i]);

            m_ui->tableView->clearSelection();
            m_currentDataInputIndex = -1;

            updateButtons();
            updateContents();
        }
    }
}

void CDataInputListDlg::onEditDataInput()
{
    if (m_currentDataInputIndex >= 0) {
        int index = 0;
        for (int i = 0; i < m_dataInputs.size(); ++i) {
            auto dataInput = m_dataInputs.at(i);
            index = i;
            if (dataInput->name == m_currentDataInputName)
                break;
        }
        CDataInputDlg datainputdialog(&m_dataInputs[index], m_tableContents, this);
        datainputdialog.exec();

        updateButtons();
        updateContents();

        m_ui->tableView->selectRow(m_currentDataInputIndex);
    }
}

void CDataInputListDlg::onActivated(const QModelIndex &index)
{
    const QModelIndexList indexes = m_ui->tableView->selectionModel()->selectedRows(0);
    m_currentDataInputIndex = indexes.size() ? index.row() : -1;
    if (m_currentDataInputIndex >= 0) {
        m_currentDataInputName
                = m_tableContents->itemFromIndex(indexes.at(0))->data(Qt::EditRole).toString();
    }
    onEditDataInput();
}

void CDataInputListDlg::onSelectionChanged()
{
    const QModelIndexList indexes = m_ui->tableView->selectionModel()->selectedRows(0);
    m_currentDataInputIndex = indexes.size() ? indexes.at(0).row() : -1;
    if (m_currentDataInputIndex >= 0) {
        m_currentDataInputName
                = m_tableContents->itemFromIndex(indexes.at(0))->data(Qt::EditRole).toString();
    }
    updateButtons();
}

void CDataInputListDlg::onSortOrderChanged(int column, Qt::SortOrder order)
{
    m_sortColumn = column;
    m_sortOrder = order;
}
