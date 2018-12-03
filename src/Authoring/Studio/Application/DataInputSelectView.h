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
#include "DataInputDlg.h"

class DataInputSelectModel;

class DataInputSelectView : public QQuickWidget
{
    Q_OBJECT
    Q_PROPERTY(int selected MEMBER m_selection NOTIFY selectedChanged)
public:
    explicit DataInputSelectView(const QVector<EDataType> &acceptedTypes,
                                 QWidget *parent = nullptr);
    void setData(const QVector<QPair<QString, int>> &dataInputList,
                 const QString &currentController,
                 int handle = 0, int instance = 0);
    void setMatchingTypes(const QVector<EDataType> &matchingTypes);
    QString getAddNewDataInputString() { return tr("[Add New Datainput]"); }
    QString getNoneString() { return tr("[None]"); }
    DataInputSelectModel *getModel() const { return m_model; }

    Q_INVOKABLE void setSelection(int index);
    Q_INVOKABLE int selection() const { return m_selection; }
    Q_INVOKABLE void setSearchString(const QString &string);
    Q_INVOKABLE void setTypeFilter(const int index);
    Q_INVOKABLE bool toolTipsEnabled() const;

Q_SIGNALS:
    void dataInputChanged(int handle, int instance, const QString &selected);
    void selectedChanged();

protected:
    void focusOutEvent(QFocusEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void initialize();
    void updateData();
    QString getDiTypeStr(int type);
    int m_handle = 0;
    int m_instance = 0;
    DataInputSelectModel *m_model = nullptr;
    int m_selection = -1;
    QString m_currController;
    QString m_mostRecentlyAdded;
    EDataType m_defaultType;
    QVector<EDataType> m_matchingTypes;
    // -2 denotes allowed types, -1 all types, 0... matches EDataType enum
    int m_typeFilter = -2;
    QVector<QPair<QString, int>> m_dataInputList;
    QString m_searchString;
};

#endif // DATAINPUTSELECTDLG_H
