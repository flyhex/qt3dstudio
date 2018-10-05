/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#include "RecentItems.h"
#include "StudioPreferences.h"

#include <QtWidgets/qmenu.h>
#include <QtCore/qfileinfo.h>

const int CRecentItems::MAX_ITEMS = 10; // maximum allowed number of recent items

CRecentItems::CRecentItems(QMenu *inMenuID)
{
    m_Menu = inMenuID;

    ReconstructList();
}

CRecentItems::~CRecentItems()
{
}

void CRecentItems::AddRecentItem(const QString &inItem)
{
    RemoveRecentItem(inItem, false);

    m_RecentItems.insert(m_RecentItems.begin(), inItem);

    while (m_RecentItems.size() > MAX_ITEMS)
        m_RecentItems.pop_back();

    RebuildList();
}

void CRecentItems::RemoveRecentItem(const QString &inItem, bool rebuild)
{
    auto thePos = m_RecentItems.begin();
    for (; thePos != m_RecentItems.end(); ++thePos) {
        if (*thePos == inItem) {
            m_RecentItems.erase(thePos);
            break;
        }
    }

    if (rebuild)
        RebuildList();
}

// load the recent items from the preferences file to m_RecentItems
void CRecentItems::ReconstructList()
{
    m_Menu->clear();
    m_RecentItems.clear();

    int numRecentItems = CStudioPreferences::getNumRecentItems();
    if (numRecentItems > MAX_ITEMS)
        numRecentItems = MAX_ITEMS;

    for (int i = 0; i < numRecentItems; ++i) {
        QString theFile = CStudioPreferences::getRecentItem(i);
        if (!theFile.isEmpty() && QFileInfo(theFile).exists())
            m_RecentItems.push_back(theFile);
    }
}

// save the recent items from m_RecentItems to the preferences file. Also recreate the menu.
void CRecentItems::RebuildList()
{
    m_Menu->clear();

    CStudioPreferences::setNumRecentItems(GetItemCount());

    for (int i = 0; i < m_RecentItems.size(); ++i) {
        const QString &item_i = m_RecentItems.at(i);
        if (QFileInfo(item_i).exists()) {
            QAction *act = m_Menu->addAction(item_i, this, &CRecentItems::onTriggerRecent);
            act->setData(i);
            CStudioPreferences::setRecentItem(i, item_i);
        }
    }
}

QString CRecentItems::GetItem(long inIndex) const
{
    return m_RecentItems.at(inIndex);
}

void CRecentItems::onTriggerRecent()
{
    const int index = qobject_cast<QAction *>(sender())->data().toInt();
    Q_EMIT openRecent(index);
}

