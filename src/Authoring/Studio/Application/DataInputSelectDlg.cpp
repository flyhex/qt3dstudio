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
#include "DataInputSelectDlg.h"

DataInputSelectDlg::DataInputSelectDlg(QWidget *parent)
    : QListWidget(parent)
{
    connect(this, &DataInputSelectDlg::itemSelectionChanged,
            this, &DataInputSelectDlg::onSelectionChanged);
    connect(this, &DataInputSelectDlg::itemClicked,
            this, &DataInputSelectDlg::onItemClicked);
}

void DataInputSelectDlg::setData(const QStringList &dataInputList,
                                 const QString &currentController)
{
    clear();
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    addItems(dataInputList);
    QList<QListWidgetItem *> itemList = findItems(currentController, Qt::MatchFlag::MatchExactly);

    if (!itemList.isEmpty())
        setItemSelected(itemList.first(), true);
}

void DataInputSelectDlg::showDialog()
{
    show();
}

void DataInputSelectDlg::onItemClicked(QListWidgetItem *item)
{
    if (item == currentItem())
        hide();
}

void DataInputSelectDlg::onSelectionChanged()
{
    if (currentItem()) {
        Q_EMIT dataInputChanged(currentItem()->text());
        hide();
    }
}
