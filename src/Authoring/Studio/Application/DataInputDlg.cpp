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

CDataInputDlg::CDataInputDlg(SDataInputDialogItem **datainput, QStandardItemModel *data,
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

    m_ui->comboBoxTypeList->addItem(tr("Number"));
#if 0 // TODO: To be added in future version
    m_ui->comboBoxTypeList->addItem(tr("Text"));
    m_ui->comboBoxTypeList->addItem(tr("Color"));
    m_ui->comboBoxTypeList->addItem(tr("Boolean"));
#endif

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
}

CDataInputDlg::~CDataInputDlg()
{
    delete m_ui;
}

void CDataInputDlg::initDialog()
{
    if (!m_dataInput->name.isEmpty()) {
        m_ui->comboBoxTypeList->setCurrentIndex(m_dataInput->type);
        m_ui->lineEditInputName->setText(m_dataInput->name);
        m_ui->doubleSpinBoxMin->setValue(m_dataInput->m_TimeFrom / 1000.);
        m_ui->doubleSpinBoxMax->setValue(m_dataInput->m_TimeTo / 1000.);
    } else {
        m_name = getUniqueId(tr("newDataInput"));
        m_ui->lineEditInputName->setText(m_name);
    }
}

void CDataInputDlg::on_buttonBox_accepted()
{
    m_dataInput->name = m_name;
    m_dataInput->type = m_type;
    if (m_type == 0) {
        m_dataInput->m_TimeFrom = m_min * 1000.;
        m_dataInput->m_TimeTo = m_max * 1000.;
    }
    QDialog::accept();
}

void CDataInputDlg::on_buttonBox_rejected()
{
    QDialog::reject();
}

void CDataInputDlg::onTypeChanged(int type)
{
    if (type == 0) {
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
    m_name = getUniqueId(name);
    m_ui->lineEditInputName->setText(m_name);
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
