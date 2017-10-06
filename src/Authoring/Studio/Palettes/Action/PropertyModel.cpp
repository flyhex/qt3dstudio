#include "PropertyModel.h"

#include "ClientDataModelBridge.h"
#include "Core.h"
#include "Doc.h"
#include "StudioApp.h"

#include "UICDMActionCore.h"
#include "UICDMActionInfo.h"
#include "UICDMDataCore.h"
#include "UICDMMetaData.h"
#include "UICDMStudioSystem.h"


PropertyModel::PropertyModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void PropertyModel::setAction(const UICDM::CUICDMActionHandle &action)
{
    beginResetModel();
    m_action = action;
    m_valueHandle = 0;
    m_nameHandle = 0;
    m_properties.clear();

    if (action.Valid()) {
        auto doc = g_StudioApp.GetCore()->GetDoc();
        auto studioSystem = doc->GetStudioSystem();
        auto propertySystem = studioSystem->GetPropertySystem();
        auto bridge = studioSystem->GetClientDataModelBridge();

        auto actionInfo = studioSystem->GetActionCore()->GetActionInfo(action);

        UICDM::IMetaData &metaData(*studioSystem->GetActionMetaData());
        UICDM::TMetaDataPropertyHandleList metaProperties;
        const auto instance = bridge->GetInstance(actionInfo.m_Owner, actionInfo.m_TargetObject);
        metaData.GetMetaDataProperties(instance, metaProperties);

        for (const auto &metaProperty: metaProperties) {
            auto propertyMetaInfo = metaData.GetMetaDataPropertyInfo(metaProperty);
            if (propertyMetaInfo->m_IsHidden == false) {
                PropertyInfo property;
                property.m_handle = propertyMetaInfo->m_Property;
                property.m_name = QString::fromWCharArray(propertySystem->GetFormalName(instance, property.m_handle).wide_str());
                property.m_nameId = QString::fromWCharArray(propertySystem->GetName(property.m_handle).wide_str());
                property.m_type = propertyMetaInfo->GetDataType();
                property.m_additionalType = propertyMetaInfo->GetAdditionalType();

                const auto additionalMetaDataType = propertySystem->GetAdditionalMetaDataType(instance, property.m_handle);
                switch (additionalMetaDataType) {
                case UICDM::AdditionalMetaDataType::Range: {
                    const UICDM::TMetaDataData &metaDataData =
                            propertySystem->GetAdditionalMetaDataData(instance, property.m_handle);
                    UICDM::SMetaDataRange minMax = UICDM::get<UICDM::SMetaDataRange>(metaDataData);
                    property.m_min = minMax.m_Min;
                    property.m_max = minMax.m_Max;
                    break;
                }
                case UICDM::AdditionalMetaDataType::StringList: {
                    const UICDM::TMetaDataData &metaDataData =
                            propertySystem->GetAdditionalMetaDataData(instance, property.m_handle);
                    auto values = UICDM::get<UICDM::TMetaDataStringList>(metaDataData);
                    QStringList possibleValues;
                    for (const auto &value: values) {
                        possibleValues.append(QString::fromWCharArray(value.wide_str()));
                    }
                    property.m_possibleValues = possibleValues;
                    break;
                }
                case UICDM::AdditionalMetaDataType::Font: {
                    std::vector<Q3DStudio::CString> fontNames;
                    doc->GetProjectFonts(fontNames);
                    QStringList possibleValues;
                    for (const auto &fontName: fontNames) {
                        possibleValues.append(fontName.toQString());
                    }
                    property.m_possibleValues = possibleValues;
                    break;
                }
                default:;
                }
                m_properties.append(property);
            }
        }
    }
    endResetModel();

    Q_EMIT valueHandleChanged();
}

void PropertyModel::setNameHandle(const UICDM::CUICDMHandlerArgHandle &handle)
{
    m_nameHandle = handle;
}

void PropertyModel::setValueHandle(const UICDM::CUICDMHandlerArgHandle &handle)
{
    m_valueHandle = handle;

    updateDefaultPropertyIndex();
    updateValue();
    if (m_valueHandle != handle)
        Q_EMIT valueHandleChanged();
}

int PropertyModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_properties.size();
}


QVariant PropertyModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent()))
        return {};

    const auto property = m_properties.at(index.row());

    switch (role)
    {
    case NameRole:
        return property.m_name;
    case HandleRole:
        return property.m_handle.GetHandleValue();

    default:
        return {};
    }

    return QVariant();
}

QHash<int, QByteArray> PropertyModel::roleNames() const
{
    auto names = QAbstractItemModel::roleNames();
    names.insert(NameRole, "name");
    names.insert(HandleRole, "handle");

    return names;
}

PropertyInfo PropertyModel::property(int index) const
{
    if (index < 0 || index >= m_properties.size() )
        return {};
    return m_properties[index];
}

int PropertyModel::valueHandle() const
{
    return m_valueHandle;
}

QVariant PropertyModel::value() const
{
    return m_value;
}

void PropertyModel::updateDefaultPropertyIndex()
{
    if (!m_nameHandle.Valid()) {
        m_defaultPropertyIndex = -1;
        Q_EMIT defaultPropertyIndexChanged();
        return;
    }

    UICDM::SValue sValue;
    auto doc = g_StudioApp.GetCore()->GetDoc();
    auto studioSystem = doc->GetStudioSystem();
    studioSystem->GetActionCore()->GetHandlerArgumentValue(m_nameHandle, sValue);

    if (sValue.getType() != UICDM::DataModelDataType::String) {
        m_defaultPropertyIndex = -1;
        Q_EMIT defaultPropertyIndexChanged();
        return;
    }

    auto propertyName = UICDM::get<QString>(sValue);
    auto iter = std::find_if(m_properties.constBegin(), m_properties.constEnd(),
                             [&propertyName](const PropertyInfo &info)
    {
        return (info.m_nameId == propertyName);
    });

    auto index = std::distance(m_properties.constBegin(), iter);

    if (m_defaultPropertyIndex != index) {
        m_defaultPropertyIndex = index;
        Q_EMIT defaultPropertyIndexChanged();
    }
}

int PropertyModel::defaultPropertyIndex() const
{
    return m_defaultPropertyIndex;
}

void PropertyModel::updateValue()
{
    const auto oldValue = m_value;
    if (!m_valueHandle.Valid()) {
        m_value.clear();
    } else {
        UICDM::SValue sValue;
        auto doc = g_StudioApp.GetCore()->GetDoc();
        auto studioSystem = doc->GetStudioSystem();
        studioSystem->GetActionCore()->GetHandlerArgumentValue(m_valueHandle, sValue);
        m_value = sValue.toQVariant();
    }
    if (oldValue != m_value) {

        Q_EMIT valueChanged();
    }
}
