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

#ifndef DATAINPUTDLG_H
#define DATAINPUTDLG_H

#include <QtWidgets/qdialog.h>

#ifdef QT_NAMESPACE
using namespace QT_NAMESPACE;
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
    class DataInputDlg;
}
QT_END_NAMESPACE

QT_FORWARD_DECLARE_CLASS(QStandardItemModel)

class SDataInputDialogItem
{
public:
    float m_Value;
    QString m_ValueStr;
    QString m_ControlledElemProp;
    int m_TimeFrom;
    int m_TimeTo;
    QString name;
    int type;
};

class CDataInputDlg : public QDialog
{
    Q_OBJECT
public:
    CDataInputDlg(SDataInputDialogItem **datainput, QStandardItemModel *data,
                  QWidget* parent = nullptr);
    ~CDataInputDlg();

protected:
    void initDialog();
    QString getUniqueId(const QString &id);

private Q_SLOTS:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void onTypeChanged(int type);
    void onMinChanged(float min);
    void onMaxChanged(float max);
    void onNameChanged(const QString &name);

private:
    Ui::DataInputDlg *m_ui;
    SDataInputDialogItem *m_dataInput;
    QStandardItemModel *m_data;
    QString m_name;
    float m_max;
    float m_min;
    int m_type;
};

#endif