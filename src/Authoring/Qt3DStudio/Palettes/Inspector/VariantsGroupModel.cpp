/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "VariantsGroupModel.h"
#include "VariantsTagModel.h"
#include "StudioApp.h"
#include "Core.h"
#include "MainFrm.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "IDocumentEditor.h"
#include "VariantTagDialog.h"
#include "StudioUtils.h"
#include "Dialogs.h"

#include <QtCore/qsavefile.h>

VariantsGroupModel::VariantsGroupModel(QObject *parent)
    : QAbstractListModel(parent)
{

}

void VariantsGroupModel::refresh()
{
    int instance = g_StudioApp.GetCore()->GetDoc()->GetSelectedInstance();
    auto bridge = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();

    if (instance == 0 || bridge->GetObjectType(instance) & ~OBJTYPE_IS_VARIANT) {
        m_instance = 0;
        m_property = 0;
        return;
    }

    auto propertySystem = g_StudioApp.GetCore()->GetDoc()->GetPropertySystem();
    m_instance = instance;
    m_property = bridge->getVariantsProperty(instance);

    qt3dsdm::SValue sValue;
    if (propertySystem->GetInstancePropertyValue(m_instance, m_property, sValue)) {
        beginResetModel();

        // delete tag models
        for (auto &g : qAsConst(m_data))
            delete g.m_tagsModel;

        m_data.clear();

        QString propVal = qt3dsdm::get<qt3dsdm::TDataStrPtr>(sValue)->toQString();
        QHash<QString, QStringList> propTags;
        if (!propVal.isEmpty()) {
            const QStringList propTagsList = propVal.split(QChar(','));
            for (auto &propTag : propTagsList) {
                const QStringList propTagPair = propTag.split(QChar(':'));
                propTags[propTagPair[0]].append(propTagPair[1]);
            }
        }

        // build the variants data model
        const auto variantsDef = g_StudioApp.GetCore()->getProjectFile().variantsDef();
        const auto keys = g_StudioApp.GetCore()->getProjectFile().variantsDefKeys();
        for (auto &group : keys) {
            TagGroupData g;
            g.m_title = group;
            g.m_color = variantsDef[group].m_color;

            QVector<std::pair<QString, bool> > tags;
            for (int i = 0; i < variantsDef[group].m_tags.length(); ++i) {
                tags.append({variantsDef[group].m_tags[i],
                             propTags[group].contains(variantsDef[group].m_tags[i])});
            }

            g.m_tagsModel = new VariantsTagModel(tags);
            m_data.push_back(g);
        }

        endResetModel();

        bool isVariantsEmpty = rowCount() == 0;
        if (m_variantsEmpty != isVariantsEmpty) {
            m_variantsEmpty = isVariantsEmpty;
            Q_EMIT varaintsEmptyChanged();
        }
    }
}

int VariantsGroupModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return m_data.size();
}

QVariant VariantsGroupModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == GroupTitleRole)
        return m_data.at(index.row()).m_title;
    else if (role == GroupColorRole)
        return m_data.at(index.row()).m_color;
    else if (role == TagRole)
        return QVariant::fromValue(m_data.at(index.row()).m_tagsModel);

    return QVariant();
}

void VariantsGroupModel::setTagState(const QString &group, const QString &tag, bool selected)
{
    QString val;
    QString tagsStr;
    bool skipFirst = false;
    for (auto &g : qAsConst(m_data)) {
        if (g.m_title == group)
            g.m_tagsModel->updateTagState(tag, selected);

        tagsStr = g.m_tagsModel->serialize(g.m_title);
        if (!tagsStr.isEmpty()) {
            if (skipFirst)
                val.append(QChar(','));
            val.append(tagsStr);
            skipFirst = true;
        }
    }

    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*g_StudioApp.GetCore()->GetDoc(), tr("Set Tag State"))
            ->SetInstancePropertyValue(m_instance, m_property, QVariant(val));
}

void VariantsGroupModel::addNewTag(const QString &group)
{
    VariantTagDialog dlg(VariantTagDialog::AddTag, group);

    if (dlg.exec() == QDialog::Accepted) {
        g_StudioApp.GetCore()->getProjectFile().addVariantTag(group, dlg.getNames().second);
        refresh();

        if (g_StudioApp.GetCore()->getProjectFile().variantsDef()[group].m_tags.size() == 1)
            g_StudioApp.m_pMainWnd->updateActionFilterEnableState();
    }
}

void VariantsGroupModel::addNewGroup()
{
    VariantTagDialog dlg(VariantTagDialog::AddGroup);

    if (dlg.exec() == QDialog::Accepted) {
        g_StudioApp.GetCore()->getProjectFile().addVariantGroup(dlg.getNames().second);
        refresh();
    }
}

void VariantsGroupModel::importVariants()
{
    QString importFilePath = g_StudioApp.GetDialogs()->getImportVariantsDlg();

    if (!importFilePath.isEmpty()) {
        g_StudioApp.GetCore()->getProjectFile().loadVariants(importFilePath);
        refresh();
    }
}

void VariantsGroupModel::exportVariants()
{
    QString exportFilePath = g_StudioApp.GetDialogs()->getExportVariantsDlg();

    if (exportFilePath.isEmpty())
        return;

    QDomDocument domDoc;
    domDoc.appendChild(domDoc.createProcessingInstruction(QStringLiteral("xml"),
                                                          QStringLiteral("version=\"1.0\""
                                                                         " encoding=\"utf-8\"")));

    const auto variantsDef = g_StudioApp.GetCore()->getProjectFile().variantsDef();
    const auto keys = g_StudioApp.GetCore()->getProjectFile().variantsDefKeys();
    QDomElement vElem = domDoc.createElement(QStringLiteral("variants"));
    domDoc.appendChild(vElem);
    for (auto &g : keys) {
        const auto group = variantsDef[g];
        QDomElement gElem = domDoc.createElement(QStringLiteral("variantgroup"));
        gElem.setAttribute(QStringLiteral("id"), g);
        gElem.setAttribute(QStringLiteral("color"), group.m_color);
        vElem.appendChild(gElem);

        for (auto &t : qAsConst(group.m_tags)) {
            QDomElement tElem = domDoc.createElement(QStringLiteral("variant"));;
            tElem.setAttribute(QStringLiteral("id"), t);
            gElem.appendChild(tElem);
        }
    }

    QSaveFile file(exportFilePath);
    if (StudioUtils::openTextSave(file))
        StudioUtils::commitDomDocumentSave(file, domDoc);
}

QHash<int, QByteArray> VariantsGroupModel::roleNames() const
{
    auto names = QAbstractListModel::roleNames();
    names.insert(GroupTitleRole, "group");
    names.insert(GroupColorRole, "color");
    names.insert(TagRole, "tags");
    return names;
}

Qt::ItemFlags VariantsGroupModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable;
}
