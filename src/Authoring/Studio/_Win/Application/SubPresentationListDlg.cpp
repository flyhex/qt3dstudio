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
#include <QtGui/qevent.h>
#include <algorithm>

const int columnCount = 3;

CSubPresentationListDlg::CSubPresentationListDlg(
        const QString &directory, const QVector<SubPresentationRecord> &subpresentations,
        QWidget *parent)
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
    , m_ui(new Ui::SubPresentationListDlg)
    , m_directory(directory)
    , m_records(subpresentations)
    , m_currentIndex(-1)
    , m_tableContents(new QStandardItemModel(0, columnCount, this))
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

    buttons[0]->setToolTip(tr("Add New Sub-presentation..."));
    buttons[1]->setToolTip(tr("Edit Sub-presentation..."));
    buttons[2]->setToolTip(tr("Remove Sub-presentation"));

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
    // Check available list. If there are none, disable "Remove" and "Edit" buttons
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

    connect(m_ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CSubPresentationListDlg::onSelectionChanged);
    connect(m_ui->tableView, &QTableView::activated, this, &CSubPresentationListDlg::onActivated);
}

void CSubPresentationListDlg::updateButtons()
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
    if (m_records.isEmpty() || m_currentIndex == -1) {
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setEnabled(false);
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setIcon(
                    QIcon(":/images/Objects-edit-disabled.png"));
    } else {
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setEnabled(true);
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setIcon(
                    QIcon(":/images/Objects-edit-normal.png"));
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

void CSubPresentationListDlg::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Delete)) {
        onRemoveSubPresentation();
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
                m_records.removeAt(removedRows[i]);

            m_ui->tableView->clearSelection();
            m_currentIndex = -1;

            updateButtons();
            updateContents();
        }
    }
}

void CSubPresentationListDlg::onEditSubPresentation()
{
    if (m_currentIndex >= 0) {
        CSubPresentationDlg subpresdialog(m_directory, m_records.at(m_currentIndex), this);
        if (subpresdialog.exec() == QDialog::Accepted) {
            m_records[m_currentIndex] = subpresdialog.subpresentation();
            // We need to update the table to be able to accurately find out if the id is unique
            updateContents();
            // Make sure that id is still unique
            m_records[m_currentIndex].m_id = getUniqueId(m_records[m_currentIndex].m_id, true);
        }
        // Update again, as the id might have been updated
        updateContents();
        updateButtons();

        m_ui->tableView->selectRow(m_currentIndex);
    }
}

void CSubPresentationListDlg::onActivated(const QModelIndex &index)
{
    const QModelIndexList indexes = m_ui->tableView->selectionModel()->selectedIndexes();
    m_currentIndex = indexes.size() == columnCount ? index.row() : -1;
    onEditSubPresentation();
}

void CSubPresentationListDlg::onSelectionChanged()
{
    const QModelIndexList indexes = m_ui->tableView->selectionModel()->selectedIndexes();
    m_currentIndex = indexes.size() == columnCount ? indexes.at(0).row() : -1;
    updateButtons();
}

QString CSubPresentationListDlg::getUniqueId(const QString &id, bool editing)
{
    QString retval = QStringLiteral("%1").arg(id);
    int idx = 1;
    int limit = (editing == true) ? 1 : 0;
    // When editing, ignore our own id (i.e. there is supposed to be one entry already)
    while (m_tableContents->findItems(retval, Qt::MatchExactly, 0).size() > limit
           && idx < 1000) {
        retval = QStringLiteral("%1_%2").arg(id).arg(idx, 3, 10, QLatin1Char('0'));
        ++idx;
    }
    return retval;
}
