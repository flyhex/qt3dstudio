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

CDataInputListDlg::CDataInputListDlg(QVector<SDataInputDialogItem *> *datainputs, QWidget *parent)
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
    , m_ui(new Ui::DataInputListDlg)
    , m_actualDataInputs(datainputs)
    , m_currentDataInputIndex(-1)
    , m_tableContents(new QStandardItemModel(0, 3, this))
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

    // Check available list. If there are none, hide "Remove" and "Edit" buttons
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

    // When clicking an item, select the whole row
    connect(m_ui->tableView, &QTableView::clicked, this, &CDataInputListDlg::onSelected);

    // Double-click opens the item in edit mode
    connect(m_ui->tableView, &QTableView::doubleClicked, this, &CDataInputListDlg::onEditDataInput);
}

void CDataInputListDlg::updateButtons()
{
    if (m_dataInputs.isEmpty() || m_currentDataInputIndex == -1) {
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setEnabled(false);
        m_ui->buttonBoxAddEditRemove->buttons()[2]->setEnabled(false);
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setIcon(
                    QIcon(":/images/Objects-edit-disabled.png"));
        m_ui->buttonBoxAddEditRemove->buttons()[2]->setIcon(
                    QIcon(":/images/Action-Trash-Disabled.png"));
    } else {
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setEnabled(true);
        m_ui->buttonBoxAddEditRemove->buttons()[2]->setEnabled(true);
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setIcon(
                    QIcon(":/images/Objects-edit-normal.png"));
        m_ui->buttonBoxAddEditRemove->buttons()[2]->setIcon(
                    QIcon(":/images/Action-Trash-Normal.png"));
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
        if (dataInputType == DataTypeNumber) {
            dataInput.append(new QStandardItem(tr("Ranged Number")));
            QString expression = QStringLiteral("[ ")
                    + QString::number(m_dataInputs.at(i)->minValue / 1000.)
                    + QStringLiteral(" ... ")
                    + QString::number(m_dataInputs.at(i)->maxValue / 1000.)
                    + QStringLiteral(" ]");
            dataInput.append(new QStandardItem(expression));
        } else if (dataInputType == DataTypeString) {
            dataInput.append(new QStandardItem(tr("String")));
        }
#if 0 // TODO: To be added in future version
        else if (dataInputType == DataTypeEvaluator) {
            dataInput.append(new QStandardItem(tr("Evaluator")));
            dataInput.append(new QStandardItem(m_dataInputs.at(i)->valueString));
        }  else if (dataInputType == DataTypeBoolean) {
            dataInput.append(new QStandardItem(tr("Boolean")));
        } else if (dataInputType == DataTypeVector3) {
            dataInput.append(new QStandardItem(tr("Vector3")));
        } else if (dataInputType == DataTypeVector2) {
            dataInput.append(new QStandardItem(tr("Vector2")));
        } else if (dataInputType == DataTypeVariant) {
            dataInput.append(new QStandardItem(tr("Variant")));
        }
#endif
        m_tableContents->appendRow(dataInput);
    }

    m_ui->tableView->setModel(m_tableContents);
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
    SDataInputDialogItem *dataInput = new SDataInputDialogItem();
    CDataInputDlg datainputdialog(&dataInput, m_tableContents, this);
    if (datainputdialog.exec() == QDialog::Accepted)
        m_dataInputs.append(dataInput);

    updateButtons();
    updateContents();
}

void CDataInputListDlg::onRemoveDataInput()
{
    m_dataInputs.removeAt(m_ui->tableView->currentIndex().row());
    m_ui->tableView->clearSelection();
    m_currentDataInputIndex = -1;

    updateButtons();
    updateContents();
}

void CDataInputListDlg::onEditDataInput()
{
    CDataInputDlg datainputdialog(&m_dataInputs[m_currentDataInputIndex], m_tableContents, this);
    datainputdialog.exec();
    m_currentDataInputIndex = -1;

    updateButtons();
    updateContents();
}

void CDataInputListDlg::onSelected(const QModelIndex &index)
{
    m_currentDataInputIndex = index.row();
    m_ui->tableView->selectRow(m_currentDataInputIndex);

    updateButtons();
}
