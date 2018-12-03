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
    , m_currentDataInputIndex(-1)
    , m_tableContents(new QStandardItemModel(0, columnCount, this))
    , m_infoContents(new QStandardItemModel(0, 3, this))
    , m_goToAdd(goToAdd)
    , m_sortColumn(-1)
    , m_defaultType(defaultType)
    , m_acceptedTypes(acceptedTypes)
{
    m_ui->setupUi(this);

    // Create icon buttons. Give them object and accessible names so their style can be modified
    // via stylesheet.
    QPushButton *addButton = new QPushButton(this);
    addButton->setIcon(QIcon(":/images/add.png"));
    addButton->setAccessibleName(QStringLiteral("DataInputListButton"));
    addButton->setObjectName(QStringLiteral("DataInputListButton"));
    QPushButton *removeButton = new QPushButton(this);
    removeButton->setIcon(QIcon(":/images/Action-Trash-Disabled.png"));
    removeButton->setAccessibleName(QStringLiteral("DataInputListButton"));
    removeButton->setObjectName(QStringLiteral("DataInputListButton"));

    m_ui->buttonBoxAddEditRemove->addButton(addButton, QDialogButtonBox::ActionRole);
    m_ui->buttonBoxAddEditRemove->addButton(removeButton, QDialogButtonBox::ActionRole);
    QList<QAbstractButton *>buttons = m_ui->buttonBoxAddEditRemove->buttons();
    connect(buttons.at(0), &QAbstractButton::clicked, this, &CDataInputListDlg::onAddDataInput);
    connect(buttons.at(1), &QAbstractButton::clicked, this, &CDataInputListDlg::onRemoveDataInput);

    buttons[0]->setToolTip(tr("Add New Data Input..."));
    buttons[0]->setText(tr("Add Data Input"));
    buttons[1]->setToolTip(tr("Remove Data Input"));
    buttons[1]->setText(tr("Remove existing Data Input"));

    m_ui->typeFilterCombo->addItems({tr("[All types]"), tr("Boolean"),
                                     tr("Float"), tr("Ranged Number"), tr("String"), tr("Variant"),
                                     tr("Vector2"), tr("Vector3")});
    m_ui->typeFilterCombo->setToolTip(tr("Filter the list by Data Input type"));

    m_ui->searchField->setToolTip(tr("Search for Data Input"));

    connect(m_ui->typeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CDataInputListDlg::onFilterTypeChanged);
    connect(m_ui->searchField, &QLineEdit::textChanged, this,
            &CDataInputListDlg::onSearchTextChanged);

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
    updateInfo();

    // Make the expression column wider than name and type
    m_ui->tableView->horizontalHeader()->setStretchLastSection(true);
    m_ui->tableView->horizontalHeader()->setMinimumSectionSize(125);
    m_ui->tableView->setFocusPolicy(Qt::NoFocus);

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
    m_ui->elementInfo->resizeColumnsToContents();
    m_ui->elementInfo->horizontalHeader()->setStretchLastSection(true);
    m_ui->elementInfo->horizontalHeader()->setMinimumSectionSize(125);
    m_ui->elementInfo->setModel(m_infoContents);
    m_ui->elementInfo->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

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
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setEnabled(true);
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setIcon(
                    QIcon(":/images/Action-Trash-Normal.png"));
    } else {
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setEnabled(false);
        m_ui->buttonBoxAddEditRemove->buttons()[1]->setIcon(
                    QIcon(":/images/Action-Trash-Disabled.png"));
    }
}

void CDataInputListDlg::updateContents()
{
    m_tableContents->clear();

    QStringList labels;
    labels << tr("Name") << tr("Input Type") << tr("Expression");
    m_tableContents->setHorizontalHeaderLabels(labels);

    QList<QStandardItem *> dataInput;

    for (auto &it : qAsConst(m_dataInputs)) {
        dataInput.clear();

        int dataInputType = it->type;

        if ((dataInputType == m_typeFilter || m_typeFilter == -1)
            && it->name.contains(m_searchString)){

            dataInput.append(new QStandardItem(it->name));

            if (dataInputType == DataTypeRangedNumber
                && (m_typeFilter == (int)DataTypeRangedNumber || m_typeFilter == -1)) {
                dataInput.append(new QStandardItem(tr("Ranged Number")));
                QString expression = QStringLiteral("[ ")
                        + QString::number(it->minValue)
                        + QStringLiteral(" ... ")
                        + QString::number(it->maxValue)
                        + QStringLiteral(" ]");
                dataInput.append(new QStandardItem(expression));
            } else if (dataInputType == DataTypeString
                       && (m_typeFilter == (int)DataTypeString || m_typeFilter == -1)) {
                dataInput.append(new QStandardItem(tr("String")));
            } else if (dataInputType == DataTypeFloat
                       && (m_typeFilter == (int)DataTypeFloat || m_typeFilter == -1)) {
                dataInput.append(new QStandardItem(tr("Float")));
#ifdef DATAINPUT_EVALUATOR_ENABLED
            } else if (dataInputType == DataTypeEvaluator
                       && (m_typeFilter == (int)DataTypeEvaluator || m_typeFilter == -1)) {
                dataInput.append(new QStandardItem(tr("Evaluator")));
                dataInput.append(new QStandardItem(m_dataInputs.at(i)->valueString));
#endif
            } else if (dataInputType == DataTypeBoolean
                       && (m_typeFilter == (int)DataTypeBoolean || m_typeFilter == -1)) {
                dataInput.append(new QStandardItem(tr("Boolean")));
            } else if (dataInputType == DataTypeVector3
                       && (m_typeFilter == (int)DataTypeVector3 || m_typeFilter == -1)) {
                dataInput.append(new QStandardItem(tr("Vector3")));
            } else if (dataInputType == DataTypeVector2
                       && (m_typeFilter == (int)DataTypeVector2 || m_typeFilter == -1)) {
                dataInput.append(new QStandardItem(tr("Vector2")));
            } else if (dataInputType == DataTypeVariant
                       && (m_typeFilter == (int)DataTypeVariant || m_typeFilter == -1)) {
                dataInput.append(new QStandardItem(tr("Variant")));
            }

            // highlight datainputs that are in use
            if (it->ctrldElems.size() || it->externalPresBoundTypes.size())
                dataInput.first()->setForeground(QBrush(CStudioPreferences::dataInputColor()));
            m_tableContents->appendRow(dataInput);
        }
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

    QStringList labels;
    labels << tr("Object Name") << tr("Location") << tr("Properties");
    m_infoContents->setHorizontalHeaderLabels(labels);

    // Only show controlled instances if we have a single datainput selected.
    if (m_ui->tableView->selectionModel()->selectedRows(0).size() == 1) {
        for (auto allCtrldElemsIt = m_dataInputs[m_currentDataInputName]->ctrldElems.begin();
             allCtrldElemsIt != m_dataInputs[m_currentDataInputName]->ctrldElems.end();) {
            QStandardItem *item = new QStandardItem(
                        g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()
                        ->GetClientDataModelBridge()->GetName(
                            allCtrldElemsIt->instHandle.GetHandleValue()));
            // Store actual handle value to Qt::Userdata+1
            item->setData(allCtrldElemsIt->instHandle.GetHandleValue());
            auto path = refHelper->GetObjectReferenceString(
                        doc->GetSceneInstance(), CRelativePathTools::EPATHTYPE_GUID,
                        allCtrldElemsIt->instHandle);
            // One element can have several properties controlled by this datainput,
            // do not show element several times. Show the number of properties after
            // the elementpath and the list of property names in a separate column.
            if (m_infoContents->findItems(path, Qt::MatchContains, 1).isEmpty()) {
                item->setToolTip(path);
                item->setEditable(false);

                QString propNames;
                int count = 0;
                CDataInputDialogItem *di = m_dataInputs[m_currentDataInputName];

                QVector<CDataInputDialogItem::ControlledItem> thisInstCtrldItems;
                di->getInstCtrldItems(
                            allCtrldElemsIt->instHandle.GetHandleValue(), thisInstCtrldItems);
                for (auto thisInstCtrldItemsIt : qAsConst(thisInstCtrldItems)) {
                    if (propNames.size() != 0)
                        propNames.append(QLatin1String(", "));

                    if (thisInstCtrldItemsIt.propHandle.Valid()) {
                        propNames.append(g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->
                                         GetPropertySystem()->GetFormalName(
                                             thisInstCtrldItemsIt.instHandle,
                                             thisInstCtrldItemsIt.propHandle));
                    } else {
                        propNames.append(QObject::tr("timeline/slide"));
                    }

                    count++;
                    // Advance main iterator so that after the inner loop we end up
                    // at the start of next instance's batch of controlleditems.
                    allCtrldElemsIt++;
                }

                QStandardItem *item2 = new QStandardItem(path + QStringLiteral("(")
                                                         + QString::number(count)
                                                         + QStringLiteral(")"));
                item2->setToolTip(path);
                item2->setEditable(false);
                QStandardItem *item3 = new QStandardItem(propNames);
                item3->setToolTip(propNames);
                item3->setEditable(false);
                m_infoContents->appendRow(QList<QStandardItem *>({item, item2, item3}));
            }
        }
        // Show this datainput uses in subpresentations but leave property name list
        // empty because we do not have that info.
        const auto uniqueKeys
                = m_dataInputs[m_currentDataInputName]->externalPresBoundTypes.uniqueKeys();
        for (auto &k : uniqueKeys) {
            m_infoContents->appendRow(QList<QStandardItem *>(
                {new QStandardItem(QObject::tr("<subpresentation>")),
                 new QStandardItem(k), new QStandardItem()}));
        }
    }

    m_ui->elementInfo->setModel(m_infoContents);
}
bool CDataInputListDlg::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (m_currentDataInputIndex >= 0 && (ke->key() == Qt::Key_Delete)) {
            onRemoveDataInput();
            event->accept();
            return true;
        } else {
            return QDialog::event(event);
        }
    } else {
        return QDialog::event(event);
    }
}

void CDataInputListDlg::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)) {
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

    // Otherwise find the new position of added DI and select it.
    auto idxList = m_ui->tableView->selectionModel()->model()->match(
                m_ui->tableView->selectionModel()->model()->index(
                    0,0), Qt::EditRole, m_mostRecentlyAdded);

    if (!idxList.empty())
        m_ui->tableView->selectRow(idxList.first().row());
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
        for (const auto &index : indexes) {
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
        // Datatype and strictness requirement
        QVector<QPair<qt3dsdm::DataModelDataType::Value, bool>> types;
        di->getBoundTypes(types);
        if (types.size()) {
            for (auto type : qAsConst(types)) {
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
        // Insert replaces the previous key - value pair if existing.
        m_dataInputs.insert(m_currentDataInputName, di);

        updateButtons();
        updateContents();

        // Find the new position of renamed DI and select it.
        auto idxList = m_ui->tableView->selectionModel()->model()->match(
                    m_ui->tableView->selectionModel()->model()->index(
                        0,0), Qt::EditRole, m_currentDataInputName);

        if (!idxList.empty())
            m_ui->tableView->selectRow(idxList.first().row());
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
        selBoundTypes.append(m_dataInputs[m_currentDataInputName]
                             ->ctrldElems[it.row()].dataType);

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
    QVector<QPair<qt3dsdm::DataModelDataType::Value, bool>> types;
    m_dataInputs[m_currentDataInputName]->getBoundTypes(types);
    setUniqueAcceptedDITypes(types);

    CDialogs::showWidgetBrowser(this, m_dataInputChooserView, geometry().center(),
                                CDialogs::WidgetBrowserAlign::Center);
}

void CDataInputListDlg::onElementSelectionChanged()
{
    bool disable = true;
    QModelIndexList selected = m_ui->elementInfo->selectionModel()->selectedRows(2);
    // We only can change bindings in the currently open project. Disable replace
    // actions if we have selected a row denoting control in a subpresentation
    // (fastest way to check is to see if property list is empty).
    if (selected.isEmpty())
        disable = true;
    else if (m_ui->elementInfo->model()->data(selected.at(0)).isNull())
        disable = true;
    else
        disable = false;

    m_replaceSelectedAction->setDisabled(disable);
    m_replaceAllAction->setDisabled(disable);
}

void CDataInputListDlg::onFilterTypeChanged(int index)
{
    m_typeFilter = index - 1;
    updateContents();
}

void CDataInputListDlg::onSearchTextChanged()
{
    m_searchString = m_ui->searchField->text();
    updateContents();
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
        for (auto &it : qAsConst(m_dataInputs))
            dataInputList.append({it->name, it->type});

        m_dataInputChooserView->setMatchingTypes(okDiTypes);
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
        CDataInputDialogItem *oldDI = *m_dataInputs.find(m_currentDataInputName);
        CDataInputDialogItem *newDI = *m_dataInputs.find(newDIName);

        QVector<CDataInputDialogItem::ControlledItem> ctrlItems;
        oldDI->getInstCtrldItems(it.GetHandleValue(), ctrlItems);

        for (auto &it2 : qAsConst(ctrlItems)) {
            oldDI->ctrldElems.removeAll(it2);
            newDI->ctrldElems.append(it2);
        }
    }
    // Make direct changes to object properties. Transaction that is opened will be
    // closed when the dialog ultimately exits.
    g_StudioApp.GetCore()->GetDoc()->ReplaceDatainput(m_currentDataInputName,
                                                      newDIName, elementHandles);
}
