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

#include "ImageChooserModel.h"
#include "StudioApp.h"

ImageChooserModel::ImageChooserModel(QObject *parent)
    : ChooserModelBase(parent)
{
}

ImageChooserModel::~ImageChooserModel()
{
}

bool ImageChooserModel::isVisible(const QString &path) const
{
    return getIconType(path) == OBJTYPE_IMAGE || !g_StudioApp.getRenderableId(path).isEmpty();
}

const QVector<ChooserModelBase::FixedItem> ImageChooserModel::getFixedItems() const
{
    static const QVector<FixedItem> items = { { OBJTYPE_IMAGE, "", tr("[None]") } };
    return items;
}

QString ImageChooserModel::specialDisplayName(const ChooserModelBase::TreeItem &item) const
{
    // Renderable items display the id instead of file name
    const QString path = item.index.data(QFileSystemModel::FilePathRole).toString();
    return g_StudioApp.getRenderableId(path);
}
