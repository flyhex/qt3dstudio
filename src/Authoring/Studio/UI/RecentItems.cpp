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

#include "Qt3DSCommonPrecompile.h"

#include "RecentItems.h"
#include "Preferences.h"

#include <QtWidgets/qmenu.h>
#include <QtCore/qfileinfo.h>

const QString CRecentItems::RECENTITEM_KEY = QStringLiteral("RecentItem");
const QString CRecentItems::RECENTIMPORT_KEY = QStringLiteral("RecentImport");
const QString CRecentItems::RECENTITEM_VALID = QStringLiteral("RecentValid");

CRecentItems::CRecentItems(QMenu *inMenuID, long inCommandID, QString inPreferenceKey)
{
    Q_UNUSED(inCommandID)

    m_Menu = inMenuID;
    m_ValidItems = 10;
    m_PreferenceKey = inPreferenceKey;

    connect(m_Menu, &QMenu::aboutToShow, this, &CRecentItems::handleAboutToShow);

    ReconstructList();
}

CRecentItems::~CRecentItems()
{
}

void CRecentItems::AddRecentItem(const QString &inItem)
{
    RemoveRecentItem(inItem);

    m_RecentItems.insert(m_RecentItems.begin(), inItem);

    while (m_RecentItems.size() > 10)
        m_RecentItems.pop_back();

    RebuildList();
}

void CRecentItems::RemoveRecentItem(const QString &inItem)
{
    TFileList::iterator thePos = m_RecentItems.begin();
    for (; thePos != m_RecentItems.end(); ++thePos) {
        if ((*thePos) == inItem) {
            m_RecentItems.erase(thePos);
            break;
        }
    }

    RebuildList();
}

void CRecentItems::ReconstructList()
{
    ClearMenu();
    m_RecentItems.clear();

    CPreferences thePrefs = CPreferences::GetUserPreferences();

    m_ValidItems = thePrefs.GetLongValue(RECENTITEM_VALID, m_ValidItems);

    for (long theIndex = 0; theIndex < (m_ValidItems > 10 ? 10 : m_ValidItems); ++theIndex) {
        QString key = QStringLiteral("%1%2").arg(m_PreferenceKey).arg(theIndex);
        QString filename = thePrefs.GetStringValue(key);
        if (!filename.isEmpty()) {
            QFileInfo info(filename);
            if (info.exists())
                m_RecentItems.push_back(filename);
        }
    }
}

void CRecentItems::RebuildList()
{
    ClearMenu();

    CPreferences thePrefs = CPreferences::GetUserPreferences();
    thePrefs.SetLongValue(RECENTITEM_VALID, GetItemCount());
    TFileList::iterator thePos = m_RecentItems.begin();
    for (long theIndex = 0; thePos != m_RecentItems.end(); ++thePos, ++theIndex) {
        QFileInfo theFile = *thePos;
        if (theFile.exists()) {
            QAction *act = m_Menu->addAction(theFile.fileName(),
                                             this, &CRecentItems::onTriggerRecent);
            act->setData(static_cast<int>(theIndex));
            QString key = QStringLiteral("%1%2").arg(m_PreferenceKey).arg(theIndex);
            thePrefs.SetStringValue(key, (*thePos));
        }
    }
}

void CRecentItems::handleAboutToShow()
{
    RebuildList();
}

void CRecentItems::ClearMenu()
{
    m_Menu->clear();
}

QString CRecentItems::GetItem(long inIndex)
{
    return m_RecentItems.at(inIndex);
}

void CRecentItems::onTriggerRecent()
{
    const int index = qobject_cast<QAction *>(sender())->data().toInt();
    Q_EMIT openRecent(index);
}

