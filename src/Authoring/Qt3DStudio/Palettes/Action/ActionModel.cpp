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

#include "ActionModel.h"

#include "Qt3DSCommonPrecompile.h"
#include "ClientDataModelBridge.h"
#include "CmdDataModelActionSetValue.h"
#include "Core.h"
#include "Doc.h"
#include "StudioApp.h"
#include "Qt3DSDMActionSystem.h"
#include "Qt3DSDMStudioSystem.h"

ActionModel::ActionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void ActionModel::setInstanceHandle(const qt3dsdm::Qt3DSDMInstanceHandle &handle)
{
    beginResetModel();
    m_handle = handle;
    auto doc = g_StudioApp.GetCore()->GetDoc();
    m_actions.clear();
    if (handle.Valid()) {
        doc->GetStudioSystem()->GetActionSystem()->GetActions(doc->GetActiveSlide(),
                                                              handle, m_actions);
    }

    endResetModel();
}

QHash<int, QByteArray> ActionModel::roleNames() const
{
    auto names = QAbstractItemModel::roleNames();
    names.insert(DescriptionRole, "description");
    names.insert(VisibleRole, "visible");

    return names;
}


int ActionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return int(m_actions.size());
}

QVariant ActionModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent()))
        return {};

    const auto action = m_actions.at(index.row());
    auto system = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem();
    if (action.Valid()) {
        auto actionCore = system->GetActionCore();

        // Ensure the handle is still valid on the back-end, as some undo scenarios may cause this
        // function to be called for already deleted actions
        if (actionCore->HandleValid(action)) {
            switch (role)
            {
            case DescriptionRole:
                return actionString(action);
            case VisibleRole:
                return system->GetActionSystem()->GetActionEyeballValue(activeSlide(), action);
            default:
                return {};
            }
        }
    }
    return {};
}

bool ActionModel::setData(const QModelIndex &index, const QVariant &data, int role)
{
    if (!hasIndex(index.row(), index.column(), index.parent()))
        return false;

    const auto action = m_actions.at(index.row());

    if (role == VisibleRole) {
        auto doc = g_StudioApp.GetCore()->GetDoc();
        CCmd *theCmd = new CCmdDataModelActionSetEyeball(doc, activeSlide(), action, data.toBool());
        g_StudioApp.GetCore()->ExecuteCommand(theCmd);
        Q_EMIT dataChanged(index, index, {VisibleRole});
    }


    return false;
}

void ActionModel::addAction(const Qt3DSDMActionHandle &action)
{
    if (std::find(m_actions.begin(), m_actions.end(), action) == m_actions.end()) {
        const auto count = rowCount();
        beginInsertRows({}, count, count);
        m_actions.push_back(action);
        endInsertRows();
    }
}

void ActionModel::removeAction(const Qt3DSDMActionHandle &action)
{
    // KDAB_FIXME use beginRemoveRows
    beginResetModel();
    m_actions.erase(std::remove(m_actions.begin(), m_actions.end(), action), m_actions.end());
    endResetModel();
}

void ActionModel::updateAction(const Qt3DSDMActionHandle &action)
{
    for (unsigned i = 0; i < m_actions.size(); i++) {
        if (m_actions[i] == action) {
            auto idx = index(i, 0);
            Q_EMIT dataChanged(idx, idx, {});
        }
    }
}

const Qt3DSDMActionHandle ActionModel::actionAt(int row)
{
    if (row >= 0 && static_cast<unsigned>(row) < m_actions.size())
        return m_actions[row];

    return {};
}

const SActionInfo ActionModel::actionInfoAt(int row)
{
    const auto action = actionAt(row);
    if (!action.Valid())
        return {};
    auto actionCore = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetActionCore();
    return actionCore->GetActionInfo(action);
}

qt3dsdm::IActionSystem *ActionModel::actionSystem() const
{
    return g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetActionSystem();
}

qt3dsdm::Qt3DSDMSlideHandle ActionModel::activeSlide() const
{
    return g_StudioApp.GetCore()->GetDoc()->GetActiveSlide();
}

QString ActionModel::actionString(const Qt3DSDMActionHandle &action) const
{
    QString result;
    if (action.Valid()) {
        auto core = g_StudioApp.GetCore();
        auto doc = core->GetDoc();
        auto actionCore = doc->GetStudioSystem()->GetActionCore();
        auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();

        const SActionInfo &actionInfo = actionCore->GetActionInfo(action);

        // Query the event name
        QString eventFormalName(tr("[Unknown Event]"));
        Qt3DSDMEventHandle eventHandle = bridge->ResolveEvent(actionInfo);
        if (eventHandle.Valid())
            eventFormalName =
                QString::fromWCharArray(bridge->GetEventInfo(eventHandle).m_FormalName.wide_str());

        // Query the asset name
        QString assetName = tr("[Unknown]");
        assetName = bridge->GetName(actionInfo.m_Owner).toQString();

        const auto sourceInstance =
            bridge->GetInstance(actionInfo.m_Owner, actionInfo.m_TriggerObject);
        const auto targetInstance =
            bridge->GetInstance(actionInfo.m_Owner, actionInfo.m_TargetObject);
        QString sourceName = sourceInstance.Valid()
            ? bridge->GetName(sourceInstance).toQString()
            : tr("[Unknown Source]");
        QString targetName = targetInstance.Valid()
            ? bridge->GetName(targetInstance).toQString()
            : tr("[Unknown Target]");

        // Query the action name
        QString handlerFormalName(tr("[Unknown Handler]"));
        const auto handlerHandle = bridge->ResolveHandler(actionInfo);
        if (handlerHandle.Valid())
            handlerFormalName = QString::fromWCharArray(bridge->GetHandlerInfo(handlerHandle).m_FormalName.wide_str());

        // Format the strings
        if (actionInfo.m_Owner == sourceInstance) {
            if (sourceInstance == targetInstance) {
                result = tr("Listen for '%1', '%2'").arg(eventFormalName, handlerFormalName);
            } else {
                result = tr("Listen for '%1', tell %2 to '%3'").arg(eventFormalName, targetName,
                                                                   handlerFormalName);
            }
        } else if (actionInfo.m_Owner == targetInstance) {
            result = tr("Listen to '%1' for '%2', '%3'").arg(sourceName, eventFormalName,
                                                            handlerFormalName);
        } else {
            result = tr("Listen to '%1' for '%2', tell %3 to '%4'").arg(sourceName,
                                                                       eventFormalName,
                                                                       targetName,
                                                                       handlerFormalName);
        }
    }
    return result;
}
