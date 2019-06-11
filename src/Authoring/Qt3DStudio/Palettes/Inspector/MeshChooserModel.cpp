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

#include "MeshChooserModel.h"
#include "Core.h"
#include "Doc.h"
#include "IDocumentBufferCache.h"
#include "StudioApp.h"
#include "BasicObjectsModel.h"

MeshChooserModel::MeshChooserModel(QObject *parent)
    : ChooserModelBase(parent)
{
}

MeshChooserModel::~MeshChooserModel()
{

}

bool MeshChooserModel::isVisible(const QString &path) const
{
    return getIconType(path) == OBJTYPE_MODEL;
}

const QVector<ChooserModelBase::FixedItem> MeshChooserModel::getFixedItems() const
{
    static QVector<FixedItem> items;

    if (items.isEmpty()) {
        auto primitives = BasicObjectsModel::BasicMeshesModel();
        CDoc *doc = g_StudioApp.GetCore()->GetDoc();
        for (int i = 0; i < primitives.size(); i++) {
            auto item = primitives.at(i);
            const wchar_t *itemName = doc->GetBufferCache().GetPrimitiveName(item.primitiveType());
            if (itemName[0] == L'#')
                items.append({ OBJTYPE_MODEL, item.icon(), QString::fromWCharArray(itemName + 1) });
        }
    }

    return items;
}
