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

#include "SubPresentationListDlg.h"
#include "ui_SubPresentationListDlg.h"
#include "SubPresentationDlg.h"

#include <QtWidgets/qpushbutton.h>
#include <QtGui/qstandarditemmodel.h>

CSubPresentationListDlg::CSubPresentationListDlg(
        const QString &directory, const QVector<SubPresentationRecord> &subpresentations,
        QWidget *parent)
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
    , m_ui(new Ui::SubPresentationListDlg)
    , m_directory(directory)
    , m_records(subpresentations)
    , m_currentIndex(-1)
    , m_tableContents(new QStandardItemModel(0, 3, this))
{
    m_ui->setupUi(this);

    // Create icon buttons. Give them objectnames so their style can be modified via stylesheet.
    QPushButton *addButton = new QPushButton(this);
    addButton->setIcon(QIcon(":/images/add.png"));
    addButton->setObjectName(QStringLiteral("SubPresentationListButton"));
    QPushButton *editButton = new QPushButton(this);
    editButton->setIcon(QIcon(":/images/Objects-edit-disabled.png"));
    editButton->setObjectName(QStringLiteral("SubPresentationListButton"));
    QPushButton *removeButton = new QPushButton(this);
    removeButton->setIcon(QIcon(":/images/Action-Trash-Disabled.png"));
    removeButton->setObjectName(QStringLiteral("SubPresentationListButton"));

    m_ui->buttonBoxAddEditRemove->addButton(addButton, QDialogButtonBox::ActionRole);
    m_ui->buttonBoxAddEditRemove->addButton(editButton, QDialogButtonBox::ActionRole);
    m_ui->buttonBoxAddEditRemove->addButton(removeButton, QDialogButtonBox::ActionRole);
    QList<QAbstractButton *>buttons = m_ui->buttonBoxAddEditRemove->buttons();
    connect(buttons.at(0), &QAbstractButton::clicked,
            this, &CSubPresentationListDlg::onAddSubPresentation);
    connect(buttons.at(1), &QAbstractButton::clicked,
            this, &CSubPresentationListDlg::onEditSubPresentation);
    connect(buttons.at(2), &QAbstractButton::clicked,
            this, &CSubPresentationListDlg::onRemoveSubPresentation);

    initDialog();
}

CSubPresentationListDlg::~CSubPresentationListDlg()
{
    delete m_ui;
}

QVector<SubPresentationRecord> CSubPresentationListDlg::subpresentations() const
{
    return m_records;
}

void CSubPresentationListDlg::initDialog()
{
    // Check available list. If there are none, hide "Remove" and "Edit" buttons
    updateButtons();

    // Update table contents
    updateContents();

    // Disable selecting the whole table
    m_ui->tableView->setCornerButtonEnabled(false);

    // Align columns left and prevent selecting the whole column
    m_ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    m_ui->tableView->horizontalHeader()->setSectionsClickable(false);
    m_ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Hide the vertical header with line numbers
    m_ui->tableView->verticalHeader()->setHidden(true);

    // When clicking an item, select the whole row
    connect(m_ui->tableView, &QTableView::clicked, this, &CSubPresentationListDlg::onSelected);
}

void CSubPresentationListDlg::updateButtons()
{
    if (m_records.isEmpty() || m_currentIndex == -1) {
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

void CSubPresentationListDlg::updateContents()
{
    m_tableContents->clear();

    QStringList labels;
    labels << tr("Id") << tr("Type") << tr("Sub-Presentation");
    m_tableContents->setHorizontalHeaderLabels(labels);

    if (m_records.size() > 0) {
        QList<QStandardItem *> subPresentation;
        for (SubPresentationRecord rec : m_records) {
            QString type = rec.m_type == QStringLiteral("presentation")
                    ? tr("Presentation") : tr("QML Stream");
            subPresentation.clear();
            subPresentation.append(new QStandardItem(rec.m_id));
            subPresentation.append(new QStandardItem(type));
            subPresentation.append(new QStandardItem(rec.m_argsOrSrc));
            m_tableContents->appendRow(subPresentation);
        }
    }

    m_ui->tableView->setModel(m_tableContents);
}

void CSubPresentationListDlg::on_buttonBox_accepted()
{
    QDialog::accept();
}

void CSubPresentationListDlg::on_buttonBox_rejected()
{
    QDialog::reject();
}

void CSubPresentationListDlg::onAddSubPresentation()
{
    // Generate unique presentation-id
    QString id = getUniqueId(QStringLiteral("presentation-id"));
    SubPresentationRecord record = SubPresentationRecord(QStringLiteral("presentation-qml"),
                                                         id, QString());
    CSubPresentationDlg subpresdialog(m_directory, record, this);

    if (subpresdialog.exec() == QDialog::Accepted) {
        SubPresentationRecord record = subpresdialog.subpresentation();
        // Make sure that id is still unique
        record.m_id = getUniqueId(record.m_id);
        m_records.append(record);
    }

    updateButtons();
    updateContents();
}

void CSubPresentationListDlg::onRemoveSubPresentation()
{
    m_records.removeAt(m_ui->tableView->currentIndex().row());
    m_ui->tableView->clearSelection();
    m_currentIndex = -1;

    updateButtons();
    updateContents();
}

void CSubPresentationListDlg::onEditSubPresentation()
{
    CSubPresentationDlg subpresdialog(m_directory, m_records.at(m_currentIndex), this);

    if (subpresdialog.exec() == QDialog::Accepted) {
        m_records[m_currentIndex] = subpresdialog.subpresentation();
        // Make sure that id is still unique
        m_records[m_currentIndex].m_id = getUniqueId(m_records[m_currentIndex].m_id);
    }

    m_currentIndex = -1;

    updateButtons();
    updateContents();
}

void CSubPresentationListDlg::onSelected(const QModelIndex &index)
{
    m_currentIndex = index.row();
    m_ui->tableView->selectRow(m_currentIndex);

    updateButtons();
}

QString CSubPresentationListDlg::getUniqueId(const QString &id)
{
    QString retval = QStringLiteral("%1").arg(id);
    int idx = 1;
    while (m_tableContents->findItems(retval, Qt::MatchExactly, 0).size() && idx < 1000) {
        retval = QStringLiteral("%1_%2").arg(id).arg(idx, 3, 10, QLatin1Char('0'));
        ++idx;
    }
    return retval;
}
