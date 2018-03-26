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
#ifndef DATAINPUTSELECTDLG_H
#define DATAINPUTSELECTDLG_H

#include <QtQuickWidgets/qquickwidget.h>
#include <QtCore/qstringlistmodel.h>
#include "DataInputSelectModel.h"

class DataInputSelectModel;

class DataInputSelectView : public QQuickWidget
{
    Q_OBJECT

public:
    explicit DataInputSelectView(QWidget *parent = nullptr);
    void setData(const QStringList &dataInputList,
                 const QString &currentController,
                 int handle = 0, int instance = 0);
    QString getAddNewDataInputString() { return tr("[Add New Datainput]"); }
    QString getNoneString() { return tr("[None]"); }

    Q_INVOKABLE void setSelection(int index);
    Q_INVOKABLE int selection() const { return m_selection; }
Q_SIGNALS:
    void dataInputChanged(int handle, int instance, const QString &selected);


protected:
    void focusOutEvent(QFocusEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void initialize();
    void updateData(const QStringList &dataInputList);
    int m_handle = 0;
    int m_instance = 0;
    DataInputSelectModel *m_model = nullptr;
    int m_selection = -1;
    QString m_currController;
};

#endif // DATAINPUTSELECTDLG_H