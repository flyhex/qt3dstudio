#ifndef PROPERTYMODEL_H
#define PROPERTYMODEL_H

#include <QAbstractListModel>

#include "UICDMHandles.h"
#include "UICDMDataTypes.h"
#include "UICDMMetaDataTypes.h"

struct PropertyInfo {
    Q_PROPERTY(QString name MEMBER m_name CONSTANT FINAL)
    Q_PROPERTY(float min MEMBER m_min CONSTANT FINAL)
    Q_PROPERTY(float max MEMBER m_max CONSTANT FINAL)
    Q_PROPERTY(UICDM::DataModelDataType::Value type MEMBER m_type CONSTANT FINAL)
    Q_PROPERTY(UICDM::AdditionalMetaDataType::Value additionalType MEMBER m_additionalType CONSTANT FINAL)
    Q_PROPERTY(QStringList possibleValues MEMBER m_possibleValues CONSTANT FINAL)

    UICDM::CUICDMPropertyHandle m_handle;
    QString m_name;
    QString m_nameId;
    UICDM::DataModelDataType::Value m_type;
    UICDM::AdditionalMetaDataType::Value m_additionalType;
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

    void setAction(const UICDM::CUICDMActionHandle &action);
    void setNameHandle(const UICDM::CUICDMHandlerArgHandle &valueHandle);
    void setValueHandle(const UICDM::CUICDMHandlerArgHandle &valueHandle);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    PropertyInfo property(int index) const;
    UICDM::CUICDMActionHandle action() const { return m_action; }
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
    UICDM::CUICDMActionHandle m_action;
    UICDM::CUICDMHandlerArgHandle m_nameHandle;
    UICDM::CUICDMHandlerArgHandle m_valueHandle;
    int m_defaultPropertyIndex = -1;
    QVariant m_value;
};

Q_DECLARE_METATYPE(PropertyInfo)

#endif // PROPERTYMODEL_H
