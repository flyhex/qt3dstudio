/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "ChooseImagePropertyDlg.h"
#include "StudioApp.h"
#include "Core.h"
#include "Qt3DSDMDataCore.h"
#include "ui_ChooseImagePropertyDlg.h"

// This dialog displays all texture properties of an object and allows the user to choose one
ChooseImagePropertyDlg::ChooseImagePropertyDlg(qt3dsdm::Qt3DSDMInstanceHandle inTarget,
                                               bool isRefMaterial,
                                               QWidget *parent)
    : QDialog(parent)
    , m_instance(inTarget)
    , m_ui(new Ui::ChooseImagePropertyDlg)
{
    m_ui->setupUi(this);

    connect(m_ui->listProps, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        m_selectedPropHandle = item->isSelected() ? item->data(Qt::UserRole).toInt() : -1;
    });

    connect(m_ui->listProps, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        Q_UNUSED(item)
        QDialog::accept();
    });

    if (!isRefMaterial)
        m_ui->cbDetach->setVisible(false);

    fillList();

    window()->setFixedSize(size());
}

ChooseImagePropertyDlg::~ChooseImagePropertyDlg()
{
    delete m_ui;
}

void ChooseImagePropertyDlg::setTextureTitle()
{
    setWindowTitle(tr("Set texture"));
    m_ui->label->setText(tr("Set texture to:"));
}

bool ChooseImagePropertyDlg::detachMaterial() const
{
    return m_ui->cbDetach->checkState() == Qt::Checked;
}

// fill the list with all material properties of type image
void ChooseImagePropertyDlg::fillList()
{
    qt3dsdm::IPropertySystem *propertySystem = g_StudioApp.GetCore()->GetDoc()->GetPropertySystem();
    qt3dsdm::TPropertyHandleList props;
    propertySystem->GetAggregateInstanceProperties(m_instance, props);
    for (auto &p : props) {
        auto metaDataType = propertySystem->GetAdditionalMetaDataType(m_instance, p);
        if (metaDataType == qt3dsdm::AdditionalMetaDataType::Value::Image) {
            QString propName = propertySystem->GetFormalName(m_instance, p);
            QListWidgetItem *newItem = new QListWidgetItem(propName);
            newItem->setData(Qt::UserRole, QVariant(p));
            m_ui->listProps->addItem(newItem);
        }
    }

    if (m_ui->listProps->count() > 0) {
        // select the first item by default
        m_ui->listProps->setCurrentRow(0);
        m_selectedPropHandle = m_ui->listProps->currentItem()->data(Qt::UserRole).toInt();
    }
}

int ChooseImagePropertyDlg::getSelectedPropertyHandle() const
{
    return m_selectedPropHandle;
}
