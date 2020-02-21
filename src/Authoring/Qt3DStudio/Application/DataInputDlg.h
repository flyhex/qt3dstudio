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
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QTableWidget>
#include <QtCore/qhash.h>
#include <QKeyEvent>

#include "Doc.h"
#include "Qt3DSDMDataTypes.h"

#ifdef QT_NAMESPACE
using namespace QT_NAMESPACE;
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
    class DataInputDlg;
}
QT_END_NAMESPACE

QT_FORWARD_DECLARE_CLASS(QStandardItemModel)

enum EDataType {
    DataTypeBoolean = 0,
    DataTypeFloat,
    DataTypeRangedNumber,
    DataTypeString,
    DataTypeVariant,
    DataTypeVector2,
    DataTypeVector3,
    DataTypeVector4
};

// The order also specifies the priority for default type in case of multiple accepted types
static const QVector<EDataType> allDataTypes {
    EDataType::DataTypeString,
    EDataType::DataTypeFloat,
    EDataType::DataTypeVector4,
    EDataType::DataTypeVector3,
    EDataType::DataTypeVector2,
    EDataType::DataTypeRangedNumber,
    EDataType::DataTypeBoolean,
    EDataType::DataTypeVariant
};

class CDataInputDlg : public QDialog
{
    Q_OBJECT

public:
    CDataInputDlg(CDataInputDialogItem **datainput, QStandardItemModel *data,
                  QWidget* parent = nullptr, const QVector<EDataType> acceptedTypes = allDataTypes);
    ~CDataInputDlg() override;

    // Maps between DataModel datatypes and datainput dialog types
    static bool isEquivalentDataType(int dlgType, qt3dsdm::DataModelDataType::Value dmType,
                                     bool strict = false);
    static QVector<EDataType> getAcceptedTypes(qt3dsdm::DataModelDataType::Value dmType,
                                               bool strict = false);

    bool eventFilter(QObject *obj, QEvent *ev) override;

protected:
    void initDialog();
    QString getUniqueId(const QString &id);
    void updateVisibility(int type);

private Q_SLOTS:
    void accept() override;
    void onTypeChanged(int type);
    void onMinChanged(float min);
    void onMaxChanged(float max);
    void onNameChanged(const QString &name);

private:
    void populateMetadata(int rows);
    void updateMetadataFromTable();
    void addMetadataRow(const QString &key, const QString &metadata, int row);
    bool checkDuplicateKeys() const;

    Ui::DataInputDlg *m_ui;
    CDataInputDialogItem *m_dataInput;
    QStandardItemModel *m_data;
    QString m_name;
    float m_max;
    float m_min;
    int m_type;
    QList<QPair<QString, QString>> m_orderedMetadata;
    QVector<EDataType> m_acceptedTypes;

};

#endif
