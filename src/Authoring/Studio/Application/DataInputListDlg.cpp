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
#include "StudioPreferences.h"
#include "StudioApp.h"
#include "Dialogs.h"
#include "Core.h"
#include "Doc.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "DataModelObjectReferenceHelper.h"

#include <QtWidgets/qpushbutton.h>
#include <QtGui/qstandarditemmodel.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qaction.h>
#include <algorithm>
#include <QtCore/qtimer.h>

const int columnCount = 3;

CDataInputListDlg::CDataInputListDlg(QMap<QString, CDataInputDialogItem *> *datainputs,
                                     bool goToAdd, QWidget *parent, EDataType defaultType,
                                     const QVector<EDataType> &acceptedTypes)
    : QDialog(parent)
    , m_ui(new Ui::DataInputListDlg)
    , m_actualDataInputs(datainputs)
    , m_goToAdd(goToAdd)
    , m_defaultType(defaultType)
    , m_currentDataInputIndex(-1)
    , m_tableContents(new QStandardItemModel(0, columnCount, this))
    , m_infoContents(new QStandardItemModel(0, 2, this))
    , m_sortColumn(-1)
    , m_acceptedTypes(acceptedTypes)
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

    window()->setFixedSize(size());
}

CDataInputListDlg::~CDataInputListDlg()
{
    delete m_ui;
}

void CDataInputListDlg::initDialog()
{
    // Copy given list to our internal one. We want to commit to the changes only after "Ok"
    // has been pressed.
    const auto keys = m_actualDataInputs->keys();
    for (auto name : keys)
        m_dataInputs.insert(name, m_actualDataInputs->value(name));

    // Check available list. If there are none, disable "Remove" and "Edit" buttons
    updateButtons();

    // Update table contents
    updateContents();

    // Make the expression column wider than name and type
    m_ui->tableView->horizontalHeader()->setStretchLastSection(true);
    m_ui->tableView->horizontalHeader()->setMinimumSectionSize(125);

    // Align columns left and prevent selecting the whole column
    m_ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    m_replaceSelectedAction = new QAction(QObject::tr("Replace selected"), m_ui->elementInfo);
    m_replaceAllAction = new QAction(QObject::tr("Replace all"), m_ui->elementInfo);
    m_replaceSelectedAction->setDisabled(true);
    m_replaceAllAction->setDisabled(true);

    m_ui->elementInfo->addAction(m_replaceSelectedAction);
    m_ui->elementInfo->addAction(m_replaceAllAction);
    m_ui->elementInfo->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_ui->elementInfo->setFocusPolicy(Qt::NoFocus);
    m_ui->elementInfo->horizontalHeader()->setStretchLastSection(true);
    m_ui->elementInfo->horizontalHeader()->setMinimumSectionSize(125);
    m_ui->elementInfo->setModel(m_infoContents);

    connect(m_replaceSelectedAction, &QAction::triggered, this,
            &CDataInputListDlg::onReplaceSelected);
    connect(m_replaceAllAction, &QAction::triggered, this, &CDataInputListDlg::onReplaceAll);

    connect(m_ui->elementInfo->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CDataInputListDlg::onElementSelectionChanged);

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

    for (auto it : qAsConst(m_dataInputs)) {
        dataInput.clear();
        dataInput.append(new QStandardItem(it->name));
        int dataInputType = it->type;
        if (dataInputType == DataTypeRangedNumber) {
            dataInput.append(new QStandardItem(tr("Ranged Number")));
            QString expression = QStringLiteral("[ ")
                    + QString::number(it->minValue)
                    + QStringLiteral(" ... ")
                    + QString::number(it->maxValue)
                    + QStringLiteral(" ]");
            dataInput.append(new QStandardItem(expression));
        } else if (dataInputType == DataTypeString) {
            dataInput.append(new QStandardItem(tr("String")));
        } else if (dataInputType == DataTypeFloat) {
            dataInput.append(new QStandardItem(tr("Float")));
#ifdef DATAINPUT_EVALUATOR_ENABLED
        } else if (dataInputType == DataTypeEvaluator) {
            dataInput.append(new QStandardItem(tr("Evaluator")));
            dataInput.append(new QStandardItem(m_dataInputs.at(i)->valueString));
#endif
        } else if (dataInputType == DataTypeBoolean) {
            dataInput.append(new QStandardItem(tr("Boolean")));
        } else if (dataInputType == DataTypeVector3) {
            dataInput.append(new QStandardItem(tr("Vector3")));
        } else if (dataInputType == DataTypeVector2) {
            dataInput.append(new QStandardItem(tr("Vector2")));
        } else if (dataInputType == DataTypeVariant) {
            dataInput.append(new QStandardItem(tr("Variant")));
        }
        // highlight datainputs that are in use
        if (it->controlledElems.size() || it->externalPresBoundTypes.size())
            dataInput.first()->setForeground(QBrush(CStudioPreferences::dataInputColor()));
        m_tableContents->appendRow(dataInput);
    }

    m_ui->tableView->setModel(m_tableContents);

    if (m_sortColumn >= 0)
        m_ui->tableView->sortByColumn(m_sortColumn, m_sortOrder);
}

void CDataInputListDlg::updateInfo()
{
    auto doc = g_StudioApp.GetCore()->GetDoc();
    auto refHelper = doc->GetDataModelObjectReferenceHelper();

    m_infoContents->clear();
    if (m_ui->tableView->selectionModel()->selectedRows(0).size() == 1) {
        for (auto it : qAsConst(m_dataInputs[m_currentDataInputName]->controlledElems)) {
            QStandardItem *item = new QStandardItem(
                        g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->
                        GetClientDataModelBridge()->GetName(it.GetHandleValue()).toQString());
            // Store actual handle value to Qt::Userdata+1
            item->setData(it.GetHandleValue());
            auto path = refHelper->GetObjectReferenceString(
                        doc->GetSceneInstance(), CRelativePathTools::EPATHTYPE_GUID,
                        it.GetHandleValue()).toQString();
            // One element can have several properties controlled by this datainput,
            // do not show element several times. Show the number of properties after
            // the elementpath.
            if (m_infoContents->findItems(path, Qt::MatchContains, 1).isEmpty()) {
                item->setToolTip(path);
                item->setEditable(false);
                QStandardItem *item2
                        = new QStandardItem(path + QStringLiteral(" (") + QString::number(
                                                m_dataInputs[m_currentDataInputName]->
                                                controlledElems.count(it.GetHandleValue()))
                                                + QStringLiteral(")"));
                item2->setEditable(false);
                m_infoContents->appendRow(QList<QStandardItem *>({item, item2}));
            }
        }
    }

    m_ui->elementInfo->setModel(m_infoContents);
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

void CDataInputListDlg::accept()
{
    if (g_StudioApp.GetCore()->GetDoc()->IsTransactionOpened())
        g_StudioApp.GetCore()->GetDoc()->CloseTransaction();

    m_actualDataInputs->clear();

    const auto keys = m_dataInputs.keys();
    for (auto name : keys)
        m_actualDataInputs->insert(name, m_dataInputs.value(name));

    QDialog::accept();
}

void CDataInputListDlg::reject()
{
    // If the user cancels, also roll back any possible changes to data input bindings.
    if (g_StudioApp.GetCore()->GetDoc()->IsTransactionOpened())
        g_StudioApp.GetCore()->GetDoc()->RollbackTransaction();

    QDialog::reject();
}

void CDataInputListDlg::onAddDataInput()
{
    // Create a new data input dialog item and give it to dialog
    CDataInputDialogItem *dataInput = new CDataInputDialogItem();
    dataInput->type = m_defaultType;
    CDataInputDlg datainputdialog(&dataInput, m_tableContents, this, m_acceptedTypes);
    datainputdialog.setWindowTitle("Add Data Input");
    if (datainputdialog.exec() == QDialog::Accepted) {
        m_dataInputs.insert(dataInput->name, dataInput);
        m_mostRecentlyAdded = dataInput->name;
    } else {
        m_mostRecentlyAdded.clear();
    }

    updateButtons();
    updateContents();

    // If we went straight to adding a new datainput, close
    // dialog automatically
    if (m_goToAdd)
        accept();
}

void CDataInputListDlg::onRemoveDataInput()
{
    QString title(QObject::tr("Warning"));
    QString text(QObject::tr("This operation cannot be undone. Are you sure?"));
    auto ret = g_StudioApp.GetDialogs()->DisplayMessageBox(title, text,
                                                           Qt3DSMessageBox::ICON_WARNING, true,
                                                           this);
    if (ret != Qt3DSMessageBox::MSGBX_OK)
        return;

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
                m_dataInputs.remove(
                            m_tableContents->item(removedRows[i])->data(Qt::EditRole).toString());

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
        CDataInputDialogItem *di = m_dataInputs.value(m_currentDataInputName);

        // Only show types that are ok for _all_ currently controlled properties.
        // If datainput is not controlling any elements, all types are ok.

        // Datainput binding types
        QVector<EDataType> allowedTypes;
        bool strictFound = false;
        if (di->controlledElems.size()) {
            for (auto type : qAsConst(di->boundTypes)) {
                // If we hit strict type requirement for a certain bound datatype, set allowed types
                // to only this data type (and Variant) and exit after appending it to
                // allowedTypes vector.
                // We should never have strict requirement for two different datatypes, obviously.
                if (type.second)
                    allowedTypes.clear();

                auto acceptedTypes = CDataInputDlg::getAcceptedTypes(type.first, type.second);

                for (auto t : qAsConst(acceptedTypes)) {
                    if (!allowedTypes.contains(t))
                        allowedTypes.append(t);
                }
                // if we just hit a strict type requirement we are finished
                if (type.second) {
                    strictFound = true;
                    break;
                }
            }
        }

        // Datainput bindings for all other presentations, unless we already have a
        // strict type requirement
        if (di->externalPresBoundTypes.size() && !strictFound) {
            for (auto type : qAsConst(di->externalPresBoundTypes)) {
                if (type.second)
                    allowedTypes.clear();

                const auto acceptedTypes = CDataInputDlg::getAcceptedTypes(type.first, type.second);

                for (auto t : acceptedTypes) {
                    if (!allowedTypes.contains(t))
                        allowedTypes.append(t);
                }
                // if we just hit a strict type requirement we are finished
                if (type.second)
                    break;
            }
        }

        // no bindings in this or other presentations, all datatypes are ok
        if (!allowedTypes.size())
            allowedTypes = allDataTypes;

        CDataInputDlg datainputdialog(&di, m_tableContents, this, allowedTypes);
        datainputdialog.exec();

        // if we are renaming a datainput, remove the old key - value and
        // add it again as new entry with new name
        if (m_currentDataInputName != di->name) {
            m_ui->elementInfo->selectAll();
            const QModelIndexList indexes = m_ui->elementInfo->selectionModel()->selectedRows();
            QList<qt3dsdm::Qt3DSDMInstanceHandle> elementHandles;
            for (auto it : indexes) {
               elementHandles.append(
                           m_ui->elementInfo->model()->data(it, Qt::UserRole + 1).toInt());
            }
            // Opens up a transaction if one is not existing; we will close it at
            // dialog exit to batch ok/cancel all changes done in this dialog.
            g_StudioApp.GetCore()->GetDoc()->ReplaceDatainput(m_currentDataInputName,
                                                              di->name, elementHandles);
            m_dataInputs.remove(m_currentDataInputName);
            m_currentDataInputName = di->name;
        }
        // insert replaces the previous key - value pair if existing
        m_dataInputs.insert(m_currentDataInputName, di);

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
    updateInfo();
    onElementSelectionChanged();
}

void CDataInputListDlg::onSortOrderChanged(int column, Qt::SortOrder order)
{
    m_sortColumn = column;
    m_sortOrder = order;
}

void CDataInputListDlg::onReplaceSelected()
{
    if (!m_dataInputChooserView) {
        m_dataInputChooserView = new DataInputSelectView(m_acceptedTypes, this);
        // Do not show "Add new" and "None" choices.
        m_dataInputChooserView->getModel()->showFixedItems(false);
    } else {
        disconnect(m_dataInputChooserView, nullptr, nullptr, nullptr);
    }

    connect(m_dataInputChooserView, &DataInputSelectView::dataInputChanged, this,
            [this](int handle, int instance, const QString &controllerName) {
        Q_UNUSED(handle)
        Q_UNUSED(instance)

        const QModelIndexList indexes = m_ui->elementInfo->selectionModel()->selectedRows();

        replaceDatainputs(indexes, controllerName);
        refreshDIs();
    });

    QVector<QPair<qt3dsdm::DataModelDataType::Value, bool>> selBoundTypes;

    const auto selRows = m_ui->elementInfo->selectionModel()->selectedRows(0);
    for (auto it : selRows)
        selBoundTypes.append(m_dataInputs[m_currentDataInputName]->boundTypes[it.row()]);

    setUniqueAcceptedDITypes(selBoundTypes);

    CDialogs::showWidgetBrowser(this, m_dataInputChooserView, geometry().center(),
                                CDialogs::WidgetBrowserAlign::Center);
}

void CDataInputListDlg::onReplaceAll()
{
    if (!m_dataInputChooserView) {
        m_dataInputChooserView = new DataInputSelectView(m_acceptedTypes, this);
        // Do not show "Add new" and "None" choices.
        m_dataInputChooserView->getModel()->showFixedItems(false);
    } else {
        disconnect(m_dataInputChooserView, nullptr, nullptr, nullptr);
    }

    connect(m_dataInputChooserView, &DataInputSelectView::dataInputChanged, this,
            [this](int handle, int instance, const QString &controllerName) {
        Q_UNUSED(handle)
        Q_UNUSED(instance)

        m_ui->elementInfo->selectAll();
        const QModelIndexList indexes = m_ui->elementInfo->selectionModel()->selectedRows();

        replaceDatainputs(indexes, controllerName);
        refreshDIs();
    });

    setUniqueAcceptedDITypes(
            m_dataInputs[m_currentDataInputName]->boundTypes);

    CDialogs::showWidgetBrowser(this, m_dataInputChooserView, geometry().center(),
                                CDialogs::WidgetBrowserAlign::Center);
}

void CDataInputListDlg::onElementSelectionChanged()
{
    bool disable = m_ui->elementInfo->selectionModel()->selectedRows().size() == 0;
    m_replaceSelectedAction->setDisabled(disable);
    m_replaceAllAction->setDisabled(disable);
}

void CDataInputListDlg::refreshDIs()
{
    updateContents();
    updateInfo();
}

void CDataInputListDlg::setUniqueAcceptedDITypes(
        const QVector<QPair<qt3dsdm::DataModelDataType::Value, bool>> &boundTypes)
{
    QVector<EDataType> okDiTypes(allDataTypes);

    for (auto it : qAsConst(allDataTypes)) {
        for (auto it2 : boundTypes) {
            if (!CDataInputDlg::isEquivalentDataType(it, it2.first, it2.second)) {
                auto idx = okDiTypes.indexOf(it);
                if (idx != -1)
                    okDiTypes.remove(idx);
            }
        }

        QVector<QPair<QString, int>> dataInputList;

        for (auto it : qAsConst(m_dataInputs)) {
            if (okDiTypes.contains((EDataType)it->type))
                dataInputList.append(QPair<QString, int>(it->name, it->type));
        }

        m_dataInputChooserView->setData(dataInputList, m_currentDataInputName);
    }
}

void CDataInputListDlg::replaceDatainputs(const QModelIndexList &selectedBindings,
                                          const QString &newDIName)
{
    QList<qt3dsdm::Qt3DSDMInstanceHandle> elementHandles;
    for (auto it : selectedBindings)
       elementHandles.append(m_ui->elementInfo->model()->data(it, Qt::UserRole + 1).toInt());

    // Update bindings for the internal list held by this dialog.
    for (auto it : qAsConst(elementHandles)) {
        // Find the old datainput controller entry/entries.
        // Same controlled element can appear several times if
        // this datainput is controlling several properties.
        // Replace them all.
        CDataInputDialogItem *oldDI = *m_dataInputs.find(m_currentDataInputName);

        int index = oldDI->controlledElems.indexOf(it.GetHandleValue());
        // Replicate controlledelement and bound type entries to new controller DI entry.
        // Make sure that new DI name is valid i.e. included in datainput map.
        while (index != -1) {
            CDataInputDialogItem *newDI = *m_dataInputs.find(newDIName);
            if (newDI) {
                newDI->controlledElems.append(it.GetHandleValue());
                newDI->boundTypes.append(oldDI->boundTypes[index]);
            } else {
                Q_ASSERT(false); // Trying to change controller to something
                                 // that is not listed in the global table.
            }
            index = oldDI->controlledElems.indexOf(it.GetHandleValue(), index + 1);
        }

        // Remove entries from old datainput.
        index = oldDI->controlledElems.indexOf(it.GetHandleValue());
        while (index != -1) {
            oldDI->controlledElems.remove(index);
            // Boundtypes indexes correspond to controlledElems so we can simply
            // use same index. Boundtypes can also have same datatype listed
            // several times, so removing one count here does not eliminate other
            // entries for the same datatype that may still be remaining.
            oldDI->boundTypes.remove(index);
            index = oldDI->controlledElems.indexOf(it.GetHandleValue());
        }
    }
    // Make direct changes to object properties. Transaction that is opened will be
    // closed when the dialog ultimately exits.
    g_StudioApp.GetCore()->GetDoc()->ReplaceDatainput(m_currentDataInputName,
                                                      newDIName, elementHandles);
}
