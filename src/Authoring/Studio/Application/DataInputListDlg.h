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

#ifndef DATAINPUTLISTDLG_H
#define DATAINPUTLISTDLG_H

#include <QtWidgets/qdialog.h>
#include <QtCore/qmap.h>
#include "DataInputDlg.h"
#include "DataInputSelectView.h"
#include <QtCore/qpointer.h>

#ifdef QT_NAMESPACE
using namespace QT_NAMESPACE;
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
    class DataInputListDlg;
}
QT_END_NAMESPACE

class CDataInputDialogItem;

QT_FORWARD_DECLARE_CLASS(QStandardItemModel)
QT_FORWARD_DECLARE_CLASS(QKeyEvent)

class CDataInputListDlg : public QDialog
{
    Q_OBJECT
public:
    CDataInputListDlg(QMap<QString, CDataInputDialogItem *> *datainputs,
                      bool goToAdd = false, QWidget *parent = nullptr,
                      EDataType defaultType = EDataType::DataTypeFloat,
                      const QVector<EDataType> &acceptedTypes = allDataTypes);
    ~CDataInputListDlg();

    QString getAddedDataInput() { return m_mostRecentlyAdded; }

protected:
    void initDialog();
    void updateButtons();
    void updateContents();
    void updateInfo();
    QVector<CDataInputDialogItem *> dataInputs() const;
    void keyPressEvent(QKeyEvent *event);
    void refreshDIs();
    void replaceDatainputs(const QModelIndexList &selectedBindings, const QString &newDIName);
    // Given a list of data model datatypes, returns the type(s) of datainputs that can
    // work as controller for all of listed datatypes.
    void setUniqueAcceptedDITypes(
            const QVector<QPair<qt3dsdm::DataModelDataType::Value, bool>> &boundTypes);
private Q_SLOTS:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void onAddDataInput();
    void onRemoveDataInput();
    void onEditDataInput();
    void onActivated(const QModelIndex &index);
    void onSelectionChanged();
    void onSortOrderChanged(int column, Qt::SortOrder order);
    void onReplaceSelected();
    void onReplaceAll();
    void onElementSelectionChanged();

private:
    Ui::DataInputListDlg *m_ui;
    QMap<QString, CDataInputDialogItem *> m_dataInputs;
    QMap<QString, CDataInputDialogItem *> *m_actualDataInputs;
    int m_currentDataInputIndex;
    QString m_currentDataInputName;
    QStandardItemModel *m_tableContents;
    QStandardItemModel *m_infoContents;
    bool m_goToAdd;
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
    QString m_mostRecentlyAdded;
    EDataType m_defaultType;
    QVector<EDataType> m_acceptedTypes;
    QPointer<DataInputSelectView> m_dataInputChooserView;

    QAction *m_replaceSelectedAction;
    QAction *m_replaceAllAction;
};

#endif
