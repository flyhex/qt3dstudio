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

#include "DataInputDlg.h"
#include "ui_DataInputDlg.h"
#include "Qt3DSMessageBox.h"

#include <QtWidgets/qabstractbutton.h>
#include <QtGui/qstandarditemmodel.h>
#include <QtWidgets/qstyleditemdelegate.h>

CDataInputDlg::CDataInputDlg(CDataInputDialogItem **datainput, QStandardItemModel *data,
                             QWidget *parent, const QVector<EDataType> acceptedTypes)
    : QDialog(parent)
    , m_ui(new Ui::DataInputDlg)
    , m_data(data)
    , m_dataInput(*datainput)
    , m_name(m_dataInput->name)
    , m_type(0)
    , m_min(0.0)
    , m_max(10.0)
    , m_metadata(m_dataInput->metadata)
    , m_acceptedTypes(acceptedTypes)
{
    m_ui->setupUi(this);

    // For enabling stylesheet for drop-down items
    QStyledItemDelegate *itemDelegate = new QStyledItemDelegate();
    m_ui->comboBoxTypeList->setItemDelegate(itemDelegate);

    m_ui->comboBoxTypeList->addItem(tr("Boolean"), QVariant(DataTypeBoolean));
#ifdef DATAINPUT_EVALUATOR_ENABLED
    m_ui->comboBoxTypeList->addItem(tr("Evaluator"), QVariant(DataTypeEvaluator));
#endif
    m_ui->comboBoxTypeList->addItem(tr("Float"), QVariant(DataTypeFloat));
    m_ui->comboBoxTypeList->addItem(tr("Ranged Number"), QVariant(DataTypeRangedNumber));
    m_ui->comboBoxTypeList->addItem(tr("String"), QVariant(DataTypeString));
    m_ui->comboBoxTypeList->addItem(tr("Variant"), QVariant(DataTypeVariant));
    m_ui->comboBoxTypeList->addItem(tr("Vector2"), QVariant(DataTypeVector2));
    m_ui->comboBoxTypeList->addItem(tr("Vector3"), QVariant(DataTypeVector3));

    QStandardItemModel *model
            = qobject_cast<QStandardItemModel *>(m_ui->comboBoxTypeList->model());
    const QBrush transparent(Qt::transparent);
    for (int i = 0; i < m_ui->comboBoxTypeList->model()->rowCount(); ++i)
    {
        QStandardItem *item = model->item(i, 0);
        if (!acceptedTypes.contains((EDataType)i))
            item->setEnabled(false);
    }

    initDialog();

    window()->setFixedSize(size());

    connect(m_ui->comboBoxTypeList,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &CDataInputDlg::onTypeChanged);
    connect(m_ui->doubleSpinBoxMin,
            static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CDataInputDlg::onMinChanged);
    connect(m_ui->doubleSpinBoxMax,
            static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CDataInputDlg::onMaxChanged);
    connect(m_ui->lineEditInputName, &QLineEdit::textChanged, this, &CDataInputDlg::onNameChanged);
    connect(m_ui->pushButtonAddMeta, &QPushButton::clicked, this, &CDataInputDlg::onMetadataAdded);
    connect(m_ui->tableEditMetadata, &QTableWidget::itemDoubleClicked, this,
            [this](QTableWidgetItem *item) {
        m_ui->lineEditMetadataKey->setText(m_ui->tableEditMetadata->item(item->row(), 0)->text());
        m_ui->lineEditMetadata->setText(m_ui->tableEditMetadata->item(item->row(), 1)->text());
        m_metadataEdit = true;
        m_editMetadataKey = m_ui->tableEditMetadata->item(item->row(), 0)->text();
        m_ui->lineEditMetadataKey->setFocus();
    });
}

CDataInputDlg::~CDataInputDlg()
{
    delete m_ui;
}

void CDataInputDlg::initDialog()
{
    // Disallow special characters and whitespaces
    QRegExpValidator *rxp = new QRegExpValidator(QRegExp("[A-Za-z0-9_]+"), this);
    QRegExpValidator *rxpWsOk = new QRegExpValidator(QRegExp("[A-Za-z0-9_ ]+"), this);
    m_ui->lineEditInputName->setValidator(rxp);
    m_ui->lineEditMetadataKey->setValidator(rxp);
    m_ui->lineEditMetadata->setValidator(rxpWsOk);

    if (!m_dataInput->name.isEmpty()) {
        m_name = m_dataInput->name;
        if (m_dataInput->type == DataTypeRangedNumber) {
            m_min = m_dataInput->minValue;
            m_max = m_dataInput->maxValue;
        }
#ifdef DATAINPUT_EVALUATOR_ENABLED
        else if (m_type == DataTypeEvaluator) {
            m_ui->lineEditEvaluation->setText(m_dataInput->valueString);
        }
#endif
    } else {
        m_name = getUniqueId(tr("newDataInput"));
        if (m_dataInput->type == DataTypeRangedNumber) {
            m_dataInput->minValue = m_min;
            m_dataInput->maxValue = m_max;
        }
    }

    m_type = m_dataInput->type;
    m_ui->comboBoxTypeList->setCurrentIndex(m_type);
    m_ui->lineEditInputName->setText(m_name);
    if (m_type == DataTypeRangedNumber) {
        m_ui->doubleSpinBoxMin->setValue(m_dataInput->minValue);
        m_ui->doubleSpinBoxMax->setValue(m_dataInput->maxValue);
    }

    m_metadata = m_dataInput->metadata;
    // Removes focus dotted border, does not disable double-click
    // to start editing an entry.
    m_ui->tableEditMetadata->setFocusPolicy(Qt::NoFocus);
    m_ui->tableEditMetadata->setRowCount(m_metadata.size());
    m_ui->tableEditMetadata->setColumnCount(3);
    m_ui->tableEditMetadata->setColumnWidth(0, 150);
    m_ui->tableEditMetadata->setColumnWidth(1, 280);
    m_ui->tableEditMetadata->setColumnWidth(2, 30);
    m_ui->tableEditMetadata->setEditTriggers(QAbstractItemView::NoEditTriggers);

    populateMetadata();
    updateVisibility(m_dataInput->type);
}

void CDataInputDlg::accept()
{
    if (m_dataInput->name != m_name)
        m_dataInput->name = getUniqueId(m_name);

    m_dataInput->type = m_type;
    if (m_type == DataTypeRangedNumber) {
        m_dataInput->minValue = m_min;
        m_dataInput->maxValue = m_max;
    }
#ifdef DATAINPUT_EVALUATOR_ENABLED
    else if (m_type == DataTypeEvaluator) {
        m_dataInput->valueString = m_text;
    }
#endif
    m_dataInput->metadata = m_metadata;

    QDialog::accept();
}

void CDataInputDlg::onTypeChanged(int type)
{
    updateVisibility(type);
    m_type = type;
}

void CDataInputDlg::onMinChanged(float min)
{
    if (m_ui->doubleSpinBoxMax->value() < min)
        m_ui->doubleSpinBoxMax->setValue(min);
    m_min = min;
}

void CDataInputDlg::onMaxChanged(float max)
{
    if (m_ui->doubleSpinBoxMin->value() > max)
        m_ui->doubleSpinBoxMin->setValue(max);
    m_max = max;
}

void CDataInputDlg::onNameChanged(const QString &name)
{
    int cursorPos = m_ui->lineEditInputName->cursorPosition();
    m_name = name;
    m_ui->lineEditInputName->setText(m_name);
    m_ui->lineEditInputName->setCursorPosition(cursorPos);
}

void CDataInputDlg::onMetadataAdded()
{
    auto key = m_ui->lineEditMetadataKey->text();
    auto metadata = m_ui->lineEditMetadata->text();
    // Let's allow empty metadata because it might have significance to
    // metadata user
    if (key.isEmpty()) {
        Qt3DSMessageBox::Show(tr("Metadata Error"), tr("Metadata key cannot be empty."),
                              Qt3DSMessageBox::ICON_WARNING, false, this);
        return;
    }

    // We are editing an existing key - value, remove old entry
    if (m_metadataEdit) {
        m_metadata.remove(m_editMetadataKey);
        m_metadataEdit = false;
        m_editMetadataKey.clear();
    }

    m_metadata.insert(key, metadata);
    m_ui->tableEditMetadata->setRowCount(m_metadata.size());
    m_ui->lineEditMetadata->clear();
    m_ui->lineEditMetadataKey->clear();

    // Let's update all entries since we do not know if QHash::insert() replaced an existing
    // entry of just appended a new one.
    populateMetadata();
}

QString CDataInputDlg::getUniqueId(const QString &id)
{
    QString retval = QStringLiteral("%1").arg(id);
    int idx = 1;
    while (m_data->findItems(retval, Qt::MatchExactly, 0).size() && idx < 1000) {
        retval = QStringLiteral("%1_%2").arg(id).arg(idx, 3, 10, QLatin1Char('0'));
        ++idx;
    }
    return retval;
}

void CDataInputDlg::updateVisibility(int type)
{
    if (type == DataTypeRangedNumber) {
        m_ui->labelMin->setVisible(true);
        m_ui->labelMax->setVisible(true);
        m_ui->doubleSpinBoxMin->setVisible(true);
        m_ui->doubleSpinBoxMax->setVisible(true);
    } else {
        m_ui->labelMin->setVisible(false);
        m_ui->labelMax->setVisible(false);
        m_ui->doubleSpinBoxMin->setVisible(false);
        m_ui->doubleSpinBoxMax->setVisible(false);
    }
#ifdef DATAINPUT_EVALUATOR_ENABLED
    if (type == DataTypeEvaluator) {
        m_ui->lineEditEvaluation->setVisible(true);
        m_ui->labelEvaluation->setVisible(true);
    } else {
        m_ui->lineEditEvaluation->setVisible(false);
        m_ui->labelEvaluation->setVisible(false);
    }
#endif
    // Adjust text label positioning according to the
    // visibility of info text warning about allowed datatypes.
    if (m_dataInput->ctrldElems.size()) {
        m_ui->labelInfoText->setVisible(true);
        m_ui->infoTxtSpacer->changeSize(20, 18);
    } else {
        m_ui->labelInfoText->setVisible(false);
        m_ui->infoTxtSpacer->changeSize(20, 0);
    }
}

void CDataInputDlg::keyPressEvent(QKeyEvent *e)
{
    // If we are editing metadata entry, grab ESC event and clear text fields.
    // Also get out of editing existing metadata entry -mode without saving changes.
    if (e->key() == Qt::Key_Escape) {
        if (m_ui->lineEditMetadata->hasFocus() || m_ui->lineEditMetadataKey->hasFocus()) {
            m_ui->lineEditMetadata->clear();
            m_ui->lineEditMetadataKey->clear();
            m_metadataEdit = false;
            // Return focus to main dialog to allow it to receive ESC events.
            this->setFocus();
            return;
        }
    }
    QDialog::keyPressEvent(e);
}

const bool CDataInputDlg::isEquivalentDataType(int dlgType,
                                               qt3dsdm::DataModelDataType::Value dmType,
                                               bool strict)
{
    if ((dlgType == EDataType::DataTypeString
         && dmType == qt3dsdm::DataModelDataType::String)
        || (dlgType == EDataType::DataTypeRangedNumber
            && dmType == qt3dsdm::DataModelDataType::RangedNumber)
        || (dlgType == EDataType::DataTypeFloat
            && (dmType == qt3dsdm::DataModelDataType::Float
                || (dmType == qt3dsdm::DataModelDataType::String && !strict)))
        || (dlgType == EDataType::DataTypeBoolean
            && dmType == qt3dsdm::DataModelDataType::Bool)
        || (dlgType == EDataType::DataTypeVector3
            && dmType == qt3dsdm::DataModelDataType::Float3)
        || (dlgType == EDataType::DataTypeVector2
            && dmType == qt3dsdm::DataModelDataType::Float2)
        // Variant can be bound to any property type except
        // as timeline controller because only datainput of type Ranged Number
        // has additional min/max information. For slide control,
        // we can allow variant type in addition to String type.
        || (dlgType == EDataType::DataTypeVariant
            && dmType != qt3dsdm::DataModelDataType::RangedNumber)
#ifdef DATAINPUT_EVALUATOR_ENABLED
        || dlgType == EDataType::DataTypeEvaluator
#endif
        ) {
        return true;
    }

    return false;
}

QVector<EDataType> CDataInputDlg::getAcceptedTypes(qt3dsdm::DataModelDataType::Value dmType,
                                                   bool strict)
{
    QVector<EDataType> acceptedTypes;
    for (auto candidate : allDataTypes) {
        if (isEquivalentDataType(candidate, dmType, strict))
            acceptedTypes.append(candidate);
    }
    return acceptedTypes;
}

void CDataInputDlg::populateMetadata()
{
    static const QIcon trashIcon = QIcon(":/images/Action-Trash-Normal.png");
    m_ui->tableEditMetadata->clearContents();
    QHashIterator<QString, QString> i(m_metadata);
    int currRow = 0;
    while (i.hasNext()) {
        i.next();
        QTableWidgetItem *key = new QTableWidgetItem(i.key());
        QTableWidgetItem *metadata = new QTableWidgetItem(i.value());
        m_ui->tableEditMetadata->setItem(currRow, 0, key);
        m_ui->tableEditMetadata->setItem(currRow, 1, metadata);
        QPushButton *delButton = new QPushButton(trashIcon, {}, nullptr);

        m_ui->tableEditMetadata->setCellWidget(currRow, 2, delButton);
        connect(delButton, &QPushButton::clicked, this, [this, i]{
            m_metadata.remove(i.key());
            populateMetadata();
        });
        ++currRow;
    }
}
