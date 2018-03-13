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

const int columnCount = 3;

CDataInputListDlg::CDataInputListDlg(QVector<CDataInputDialogItem *> *datainputs, QWidget *parent)
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
    , m_ui(new Ui::DataInputListDlg)
    , m_actualDataInputs(datainputs)
    , m_currentDataInputIndex(-1)
    , m_tableContents(new QStandardItemModel(0, columnCount, this))
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

    // Disable selecting the whole table
    m_ui->tableView->setCornerButtonEnabled(false);

    // Make the expression column wider than name and type
    m_ui->tableView->horizontalHeader()->setStretchLastSection(true);
    m_ui->tableView->horizontalHeader()->setMinimumSectionSize(125);

    // Align columns left and prevent selecting the whole column
    m_ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    m_ui->tableView->horizontalHeader()->setSectionsClickable(false);

    // Hide the vertical header with line numbers
    m_ui->tableView->verticalHeader()->setHidden(true);

    connect(m_ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CDataInputListDlg::onSelectionChanged);
    connect(m_ui->tableView, &QTableView::activated, this, &CDataInputListDlg::onActivated);
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
#if 0
        } else if (dataInputType == DataTypeEvaluator) {
            dataInput.append(new QStandardItem(tr("Evaluator")));
            dataInput.append(new QStandardItem(m_dataInputs.at(i)->valueString));
#endif
        } else if (dataInputType == DataTypeBoolean) {
            dataInput.append(new QStandardItem(tr("Boolean")));
        } else if (dataInputType == DataTypeVector3) {
            dataInput.append(new QStandardItem(tr("Vector3")));
        } else if (dataInputType == DataTypeVariant) {
            dataInput.append(new QStandardItem(tr("Variant")));
        }

        m_tableContents->appendRow(dataInput);
    }

    m_ui->tableView->setModel(m_tableContents);
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
        CDataInputDlg datainputdialog(&m_dataInputs[m_currentDataInputIndex],
                                      m_tableContents, this);
        datainputdialog.exec();

        updateButtons();
        updateContents();

        m_ui->tableView->selectRow(m_currentDataInputIndex);
    }
}

void CDataInputListDlg::onActivated(const QModelIndex &index)
{
    const QModelIndexList indexes = m_ui->tableView->selectionModel()->selectedIndexes();
    m_currentDataInputIndex = indexes.size() == columnCount ? index.row() : -1;
    onEditDataInput();
}

void CDataInputListDlg::onSelectionChanged()
{
    const QModelIndexList indexes = m_ui->tableView->selectionModel()->selectedIndexes();
    m_currentDataInputIndex = indexes.size() == columnCount ? indexes.at(0).row() : -1;
    updateButtons();
}
