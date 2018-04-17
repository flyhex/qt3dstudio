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

#include <QtWidgets/qabstractbutton.h>
#include <QtGui/qstandarditemmodel.h>
#include <QtWidgets/qstyleditemdelegate.h>

CDataInputDlg::CDataInputDlg(CDataInputDialogItem **datainput, QStandardItemModel *data,
                             QWidget *parent)
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
    , m_ui(new Ui::DataInputDlg)
    , m_data(data)
    , m_dataInput(*datainput)
    , m_name(m_dataInput->name)
    , m_type(0)
    , m_min(0.0)
    , m_max(10.0)
{
    m_ui->setupUi(this);

    // For enabling stylesheet for drop-down items
    QStyledItemDelegate *itemDelegate = new QStyledItemDelegate();
    m_ui->comboBoxTypeList->setItemDelegate(itemDelegate);

    m_ui->comboBoxTypeList->addItem(tr("Boolean"));
    m_ui->comboBoxTypeList->addItem(tr("Evaluator"));
    m_ui->comboBoxTypeList->addItem(tr("Float"));
    m_ui->comboBoxTypeList->addItem(tr("Ranged Number"));
    m_ui->comboBoxTypeList->addItem(tr("String"));
    m_ui->comboBoxTypeList->addItem(tr("Variant"));
    m_ui->comboBoxTypeList->addItem(tr("Vector2"));
    m_ui->comboBoxTypeList->addItem(tr("Vector3"));

    initDialog();

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
    connect(m_ui->lineEditEvaluation, &QLineEdit::textChanged, this, &CDataInputDlg::onTextChanged);
}

CDataInputDlg::~CDataInputDlg()
{
    delete m_ui;
}

void CDataInputDlg::initDialog()
{
    m_ui->lineEditEvaluation->setVisible(false);
    m_ui->labelEvaluation->setVisible(false);

    // Disallow special characters and whitespaces
    QRegExpValidator *rxp = new QRegExpValidator(QRegExp("[A-Za-z0-9_]+"), this);
    m_ui->lineEditInputName->setValidator(rxp);

    if (!m_dataInput->name.isEmpty()) {
        m_name = m_dataInput->name;
        m_type = m_dataInput->type;
        m_ui->comboBoxTypeList->setCurrentIndex(m_dataInput->type);
        updateVisibility(m_dataInput->type);
        m_ui->lineEditInputName->setText(m_dataInput->name);
        if (m_type == DataTypeRangedNumber) {
            m_ui->doubleSpinBoxMin->setValue(m_dataInput->minValue);
            m_ui->doubleSpinBoxMax->setValue(m_dataInput->maxValue);
            m_min = m_dataInput->minValue;
            m_max = m_dataInput->maxValue;
        }
        else if (m_type == DataTypeEvaluator) {
            m_ui->lineEditEvaluation->setText(m_dataInput->valueString);
        }
    } else {
        m_name = getUniqueId(tr("newDataInput"));
        m_ui->lineEditInputName->setText(m_name);
    }
}

void CDataInputDlg::on_buttonBox_accepted()
{
    m_dataInput->name = m_name;
    m_dataInput->type = m_type;
    if (m_type == DataTypeRangedNumber) {
        m_dataInput->minValue = m_min;
        m_dataInput->maxValue = m_max;
    }
    else if (m_type == DataTypeEvaluator) {
        m_dataInput->valueString = m_text;
    }
    QDialog::accept();
}

void CDataInputDlg::on_buttonBox_rejected()
{
    QDialog::reject();
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
    m_name = getUniqueId(name);
    m_ui->lineEditInputName->setText(m_name);
    m_ui->lineEditInputName->setCursorPosition(cursorPos);
}

void CDataInputDlg::onTextChanged(const QString &text)
{
    m_text = text;
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

    if (type == DataTypeEvaluator) {
        m_ui->lineEditEvaluation->setVisible(true);
        m_ui->labelEvaluation->setVisible(true);

    } else {
        m_ui->lineEditEvaluation->setVisible(false);
        m_ui->labelEvaluation->setVisible(false);
    }
}

const bool CDataInputDlg::isEquivalentDataType(int dlgType,
                                               qt3dsdm::DataModelDataType::Value dmType)
{
    if ((dlgType == EDataType::DataTypeString
         && dmType == qt3dsdm::DataModelDataType::String)
        || (dlgType == EDataType::DataTypeRangedNumber
            && (dmType == qt3dsdm::DataModelDataType::Float
                || dmType == qt3dsdm::DataModelDataType::String))
        || (dlgType == EDataType::DataTypeFloat
            && (dmType == qt3dsdm::DataModelDataType::Float
                || dmType == qt3dsdm::DataModelDataType::String))
        || (dlgType == EDataType::DataTypeBoolean
            && dmType == qt3dsdm::DataModelDataType::Bool)
        || (dlgType == EDataType::DataTypeVector3
            && dmType == qt3dsdm::DataModelDataType::Float3)
        || (dlgType == EDataType::DataTypeVector2
            && dmType == qt3dsdm::DataModelDataType::Float2)
        // Variant can be bound to any property type.
        // Allow also Evaluator binding to any property as we only know the evaluation
        // result type at runtime.
        || dlgType == EDataType::DataTypeVariant || dlgType == EDataType::DataTypeEvaluator) {
        return true;
    }

    return false;
}
