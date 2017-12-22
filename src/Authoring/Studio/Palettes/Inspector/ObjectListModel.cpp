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

#include "ObjectListModel.h"

#include "ClientDataModelBridge.h"
#include "Core.h"
#include "Doc.h"
#include "GraphUtils.h"
#include "IObjectReferenceHelper.h"
#include "StudioUtils.h"
#include "SlideSystem.h"
#include "StudioObjectTypes.h"
#include "StudioPreferences.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"

#include <QCoreApplication>
#include <QColor>

ObjectListModel::ObjectListModel(CCore *core,
                                 const qt3dsdm::Qt3DSDMInstanceHandle &baseHandle, QObject *parent)
    : QAbstractItemModel(parent)
    , m_core(core)
    , m_baseHandle(baseHandle)
{
    auto doc = m_core->GetDoc();
    m_objRefHelper = doc->GetDataModelObjectReferenceHelper();
    m_slideHandle = m_objRefHelper->GetSlideList(m_baseHandle)[0];
}

QHash<int, QByteArray> ObjectListModel::roleNames() const
{
    auto names = QAbstractItemModel::roleNames();
    names.insert(NameRole, "name");
    names.insert(HandleRole, "handle");
    names.insert(IconRole, "icon");
    names.insert(TextColorRole, "textColor");
    names.insert(AbsolutePathRole, "absolutePath");
    names.insert(PathReferenceRole, "referencePath");

    return names;
}

int ObjectListModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 1;

    const auto handle = handleForIndex(parent);
    const auto children = childrenList(m_slideHandle, handle);
    return children.size();
}

int ObjectListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant ObjectListModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent()))
        return {};

    auto handle = handleForIndex(index);

    auto studioSystem = m_core->GetDoc()->GetStudioSystem();
    auto propertySystem = studioSystem->GetPropertySystem();
    qt3dsdm::SValue typeValue;
    propertySystem->GetInstancePropertyValue(handle,
                                             studioSystem->GetClientDataModelBridge()
                                             ->GetTypeProperty(), typeValue);
    qt3dsdm::DataModelDataType::Value valueType(qt3dsdm::GetValueType(typeValue));
    if (valueType == qt3dsdm::DataModelDataType::None)
        return {};

    switch (role) {
    case NameRole: {
        return nameForHandle(handle);
    }
    case PathReferenceRole: {
        Q3DStudio::CString data(m_objRefHelper->GetObjectReferenceString(
            m_baseHandle, CRelativePathTools::EPATHTYPE_RELATIVE, handle));
        return data.toQString();
    }
    case AbsolutePathRole: {
        Q3DStudio::CString data(m_objRefHelper->GetObjectReferenceString(
            m_baseHandle, CRelativePathTools::EPATHTYPE_GUID, handle));
        return data.toQString();
    }
    case HandleRole: {
        return (int)handleForIndex(index);
    }
    case IconRole: {
        auto info = m_objRefHelper->GetInfo(handle);
        return resourceImageUrl() + CStudioObjectTypes::GetNormalIconName(info.m_Type);
    }
    case TextColorRole: {
        auto bridge = m_core->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
        auto objType = bridge->GetObjectType(handle);
        auto info = m_objRefHelper->GetInfo(handle);
        if (m_excludeTypes.contains(objType))
            return QVariant::fromValue(CStudioPreferences::disabledColor());
        else if (info.m_Master)
            return QVariant::fromValue(CStudioPreferences::masterColor());
        else
            return QVariant::fromValue(CStudioPreferences::textColor());
    }
    default:
        return {};
    }

    return {};
}

QModelIndex ObjectListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, (quintptr)(m_baseHandle));

    const auto handle = handleForIndex(parent);
    const auto children = childrenList(m_slideHandle, handle);
    if (row >= children.size())
        return {};

    auto childHandle = children[row];
    return createIndex(row, column, (quintptr)(childHandle));
}

QModelIndex ObjectListModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    const auto handle = handleForIndex(index);
    qt3dsdm::Qt3DSDMInstanceHandle parentHandle = m_core->GetDoc()->GetAssetGraph()->GetParent(handle);
    if (!parentHandle.Valid())
        return {};

    int row = 0;
    qt3dsdm::Qt3DSDMInstanceHandle grandParentHandle = m_core->GetDoc()->GetAssetGraph()
            ->GetParent(parentHandle);
    if (grandParentHandle.Valid()) {
        const auto children = childrenList(m_slideHandle, grandParentHandle);
        const auto it = std::find(children.begin(), children.end(), parentHandle);
        Q_ASSERT(it != children.end());
        row = it - children.begin();
    }

    return createIndex(row, 0, (quintptr)(parentHandle));
}

qt3dsdm::Qt3DSDMInstanceHandle ObjectListModel::handleForIndex(const QModelIndex &index) const
{
    return static_cast<qt3dsdm::Qt3DSDMInstanceHandle>(index.internalId());
}

void ObjectListModel::excludeObjectTypes(const QVector<EStudioObjectType> &types)
{
    m_excludeTypes = types;
}

bool ObjectListModel::selectable(const qt3dsdm::Qt3DSDMInstanceHandle &handle) const
{
    auto bridge = m_core->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
    auto objType = bridge->GetObjectType(handle);
    return !m_excludeTypes.contains(objType);
}


qt3dsdm::TInstanceHandleList ObjectListModel::childrenList(const qt3dsdm::Qt3DSDMSlideHandle &slideHandle,
                                                           const qt3dsdm::Qt3DSDMInstanceHandle &handle) const
{
    auto studioSystem = m_core->GetDoc()->GetStudioSystem();
    auto slideSystem = studioSystem->GetSlideSystem();
    auto currentMaster = slideSystem->GetMasterSlide(slideHandle);

    qt3dsdm::TInstanceHandleList children;
    m_objRefHelper->GetChildInstanceList(handle, children, slideHandle, m_baseHandle);
    children.erase(
    std::remove_if(children.begin(), children.end(),
                   [&slideHandle, slideSystem, &currentMaster](const qt3dsdm::Qt3DSDMInstanceHandle &h) {
                        const auto childSlide = slideSystem->GetAssociatedSlide(h);
                        if (!childSlide.Valid())
                            return true;
                        const auto childMaster = slideSystem->GetMasterSlide(childSlide);
                        if (childMaster == currentMaster) {
                            return childSlide != childMaster && childSlide != slideHandle;
                        } else {
                            return childSlide != childMaster;
                        }
                    }), children.end());
    return children;
}

QString ObjectListModel::nameForHandle(const qt3dsdm::Qt3DSDMInstanceHandle &handle) const
{
    const auto data = m_objRefHelper->GetInfo(handle);
    return data.m_Name.toQString();
}

QModelIndex ObjectListModel::indexForHandle(const qt3dsdm::Qt3DSDMInstanceHandle &handle,
                                            const QModelIndex &startIndex) const
{
    if (handle == m_baseHandle)
        return index(0, 0, {});

    for (int i = 0; i < rowCount(startIndex); i++) {
        auto idx = index(i, 0, startIndex);
        if (static_cast<qt3dsdm::Qt3DSDMInstanceHandle>(idx.internalId()) == handle)
            return idx;
        if (rowCount(idx) > 0) {
            QModelIndex foundIndex = indexForHandle(handle, idx);
            if (foundIndex.isValid())
                return foundIndex;
        }
    }
    return {};
}


FlatObjectListModel::FlatObjectListModel(ObjectListModel *sourceModel, QObject *parent)
    : QAbstractListModel(parent)

{
    Q_ASSERT(sourceModel);
    setSourceModel(sourceModel);
}

QVector<FlatObjectListModel::SourceInfo> FlatObjectListModel::collectSourceIndexes(
        const QModelIndex &sourceIndex, int depth) const
{
    QVector<SourceInfo> sourceInfo;

    for (int i = 0; i < m_sourceModel->rowCount(sourceIndex); i++) {
        auto idx = m_sourceModel->index(i, 0, sourceIndex);
        SourceInfo info;
        info.depth = depth;
        info.index = idx;
        sourceInfo.append(info);
        if (m_sourceModel->rowCount(idx) > 0) {
            sourceInfo += collectSourceIndexes(idx, depth + 1);
        }
    }

    return sourceInfo;
}

QHash<int, QByteArray> FlatObjectListModel::roleNames() const
{
    auto roles = m_sourceModel->roleNames();
    roles.insert(DepthRole, "depth");
    roles.insert(ExpandedRole, "expanded");
    roles.insert(ParentExpandedRole, "parentExpanded");
    roles.insert(HasChildrenRole, "hasChildren");
    return roles;
}

QModelIndex FlatObjectListModel::mapToSource(const QModelIndex &proxyIndex) const
{
    int row = proxyIndex.row();
    if (row < 0 || row >= m_sourceInfo.count())
        return {};
    return m_sourceInfo[row].index;
}

QModelIndex FlatObjectListModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    return index(rowForSourceIndex(sourceIndex));
}

QVariant FlatObjectListModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();
    if (row < 0 || row >= m_sourceInfo.count())
        return {};

    switch (role) {
    case DepthRole: {
        auto info = m_sourceInfo[row];
        return info.depth;
    }
    case ExpandedRole: {
        auto info = m_sourceInfo[row];
        return info.expanded;
    }
    case ParentExpandedRole: {
        auto info = m_sourceInfo[row];
        if (info.depth == 0)
            return true;

        int depth = info.depth;
        for (int i = row - 1; i >= 0; i--) {
            const auto prevInfo = m_sourceInfo[i];
            if (prevInfo.depth < depth) {
                if (!prevInfo.expanded) {
                    return false;
                } else {
                    depth = prevInfo.depth;
                }
            }
        }
        return true;
    }
    case HasChildrenRole: {
        if (row == m_sourceInfo.count() - 1)
            return false;
        auto info = m_sourceInfo[row];
        auto nextInfo = m_sourceInfo[row + 1];
        return (nextInfo.depth > info.depth);
    }
    }

    QModelIndex sourceIndex = mapToSource(index);
    return m_sourceModel->data(sourceIndex, role);
}

bool FlatObjectListModel::setData(const QModelIndex &index, const QVariant &data, int role)
{
    const auto row = index.row();
    if (row < 0 || row >= m_sourceInfo.count())
        return {};

    switch (role) {
    case ExpandedRole: {
        auto info = &m_sourceInfo[index.row()];
        info->expanded = data.toBool();
        Q_EMIT dataChanged(this->index(0, 0), this->index(rowCount() - 1, 0), {});
        return true;
    }
    }

    QModelIndex sourceIndex = mapToSource(index);
    return m_sourceModel->setData(sourceIndex, data, role);
}

int FlatObjectListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_sourceInfo.count();
}

void FlatObjectListModel::setSourceModel(ObjectListModel *sourceModel)
{
    beginResetModel();
    sourceModel->disconnect(this);
    connect(sourceModel, &QAbstractListModel::dataChanged, this,
            [this](const QModelIndex &start, const QModelIndex &end, const QVector<int> &roles) {
        emit dataChanged(mapFromSource(start), mapFromSource(end), roles);

    });
    connect(sourceModel, &QAbstractListModel::rowsInserted, this,
            [this](const QModelIndex &parent, int start, int end) {
        const int parentRow = rowForSourceIndex(parent);
        const int depth = m_sourceInfo[parentRow].depth + 1;
        const int startRow = rowForSourceIndex(parent, start);
        Q_ASSERT(startRow != -1);
        beginInsertRows({}, startRow, startRow + end - start);
        for (int row = end; row >= start; --row) {
            SourceInfo info;
            info.depth = depth;
            info.index = m_sourceModel->index(row, 0, parent);
            m_sourceInfo.insert(startRow, info);
        }
        endInsertRows();
    });
    connect(sourceModel, &QAbstractListModel::rowsRemoved, this,
            [this](const QModelIndex &parent, int start, int end) {
        const int startRow = rowForSourceIndex(parent, start);
        const int endRow = rowForSourceIndex(parent, end);
        Q_ASSERT(startRow != -1 && endRow != -1);
        beginRemoveRows({}, startRow, endRow);
        m_sourceInfo.remove(startRow, endRow - startRow + 1);
        endRemoveRows();
    });
    connect(sourceModel, &QAbstractListModel::modelReset, this,
            [this]() {
        beginResetModel();
        m_sourceInfo = collectSourceIndexes({}, 0);
        endResetModel();
    });

    connect(sourceModel, &ObjectListModel::roleUpdated, this, [this](int role) {
        emit dataChanged(index(0,0), index(rowCount() - 1, 0), {role});
    });

    connect(sourceModel, &ObjectListModel::rolesUpdated, this,
            [this](const QVector<int> &roles = QVector<int> ()) {
                emit dataChanged(index(0,0), index(rowCount() - 1, 0), roles);
            });


    m_sourceModel = sourceModel;
    m_sourceInfo = collectSourceIndexes({}, 0);
    endResetModel();
}

// startIndex and searchIndex are source indexes
bool FlatObjectListModel::expandTo(const QModelIndex &startIndex, const QModelIndex &searchIndex)
{
    // Found the index we are looking for. We don't want to expand it, so just return true.
    if (startIndex == searchIndex)
        return true;

    // Look for the search index in children
    const int rowCount = m_sourceModel->rowCount(startIndex);
    for (int i = 0; i < rowCount; i++) {
        auto idx = m_sourceModel->index(i, 0, startIndex);
        if (idx == searchIndex) {
            // Expand startIndex as that is the parent
            setData(index(rowForSourceIndex(startIndex)), QVariant(true), ExpandedRole);
            return true;
        }
        if (m_sourceModel->rowCount(idx) > 0) {
            bool found = expandTo(idx, searchIndex);
            if (found) {
                // Found by some descendant. Keep expanding parents
                setData(index(rowForSourceIndex(startIndex)), QVariant(true), ExpandedRole);
                return true;
            }
        }
    }
    return false;
}

int FlatObjectListModel::rowForSourceIndex(const QModelIndex &sourceIndex) const
{
    for (int i = 0; i < m_sourceInfo.size(); i++) {
        if (m_sourceInfo[i].index == sourceIndex)
            return i;
    }
    return -1;
}

int FlatObjectListModel::rowForSourceIndex(const QModelIndex& parentIndex, int row) const
{
    const int parentRow = rowForSourceIndex(parentIndex);
    if (parentRow == -1)
        return -1;
    const int childDepth = m_sourceInfo[parentRow].depth + 1;
    int i = parentRow + 1;
    while (i < m_sourceInfo.size()) {
        const auto& info = m_sourceInfo[i];
        if (info.depth < childDepth)
            break;
        if (info.depth == childDepth) {
            if (row == 0)
                break;
            --row;
        }
        ++i;
    }
    return i;
}

QModelIndex FlatObjectListModel::sourceIndexForHandle(const qt3dsdm::Qt3DSDMInstanceHandle &handle)
{
    return m_sourceModel->indexForHandle(handle);
}
