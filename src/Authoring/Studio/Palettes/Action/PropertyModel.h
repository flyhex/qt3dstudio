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

#ifndef PROPERTYMODEL_H
#define PROPERTYMODEL_H

#include <QAbstractListModel>

#include "Qt3DSDMHandles.h"
#include "Qt3DSDMDataTypes.h"
#include "Qt3DSDMMetaDataTypes.h"

struct PropertyInfo {
    Q_PROPERTY(QString name MEMBER m_name CONSTANT FINAL)
    Q_PROPERTY(float min MEMBER m_min CONSTANT FINAL)
    Q_PROPERTY(float max MEMBER m_max CONSTANT FINAL)
    Q_PROPERTY(qt3dsdm::DataModelDataType::Value type MEMBER m_type CONSTANT FINAL)
    Q_PROPERTY(qt3dsdm::AdditionalMetaDataType::Value additionalType MEMBER m_additionalType CONSTANT FINAL)
    Q_PROPERTY(QStringList possibleValues MEMBER m_possibleValues CONSTANT FINAL)

    qt3dsdm::Qt3DSDMPropertyHandle m_handle;
    QString m_name;
    QString m_nameId;
    qt3dsdm::DataModelDataType::Value m_type;
    qt3dsdm::AdditionalMetaDataType::Value m_additionalType;
    QStringList m_possibleValues;
    float m_min = 0.0f;
    float m_max = 0.0f;

    Q_GADGET
};

class PropertyModel : public QAbstractListModel
{
    Q_PROPERTY(int valueHandle READ valueHandle NOTIFY valueHandleChanged FINAL)
    Q_PROPERTY(QVariant value READ value NOTIFY valueChanged FINAL)
    Q_PROPERTY(int defaultPropertyIndex READ defaultPropertyIndex NOTIFY defaultPropertyIndexChanged FINAL)
    Q_OBJECT

public:
    explicit PropertyModel(QObject *parent = nullptr);

    enum Roles {
        NameRole = Qt::DisplayRole,
        HandleRole = Qt::UserRole + 1
    };

    void setAction(const qt3dsdm::Qt3DSDMActionHandle &action);
    void setNameHandle(const qt3dsdm::Qt3DSDMHandlerArgHandle &valueHandle);
    void setValueHandle(const qt3dsdm::Qt3DSDMHandlerArgHandle &valueHandle);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    PropertyInfo property(int index) const;
    qt3dsdm::Qt3DSDMActionHandle action() const { return m_action; }
    int valueHandle() const;

    QVariant value() const;
    int defaultPropertyIndex() const;

Q_SIGNALS:
    void valueHandleChanged();
    void valueChanged();
    void defaultPropertyIndexChanged();

private:
    void updateValue();
    void updateDefaultPropertyIndex();

    QVector<PropertyInfo> m_properties;
    qt3dsdm::Qt3DSDMActionHandle m_action;
    qt3dsdm::Qt3DSDMHandlerArgHandle m_nameHandle;
    qt3dsdm::Qt3DSDMHandlerArgHandle m_valueHandle;
    int m_defaultPropertyIndex = -1;
    QVariant m_value;
};

Q_DECLARE_METATYPE(PropertyInfo)

#endif // PROPERTYMODEL_H
