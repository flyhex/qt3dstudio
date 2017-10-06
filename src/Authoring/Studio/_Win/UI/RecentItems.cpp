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

#include "stdafx.h"

#include "RecentItems.h"
#include "Preferences.h"

#include <QMenu>

// using namespace Q3DStudio;  <-- Do not do this here because it will conflict with CList and make
// the template generator go blah

const Q3DStudio::CString CRecentItems::RECENTITEM_KEY = "RecentItem";
const Q3DStudio::CString CRecentItems::RECENTIMPORT_KEY = "RecentImport";
const Q3DStudio::CString CRecentItems::RECENTITEM_VALID = "RecentValid";

CRecentItems::CRecentItems(QMenu *inMenuID, long inCommandID, Q3DStudio::CString inPreferenceKey)
{
    Q_UNUSED(inCommandID)

    m_Menu = inMenuID;
    m_ValidItems = 10;
    m_PreferenceKey = inPreferenceKey;

    ReconstructList();
}

CRecentItems::~CRecentItems()
{
}

void CRecentItems::AddRecentItem(const CUICFile &inItem)
{
    RemoveRecentItem(inItem);

    m_RecentItems.insert(m_RecentItems.begin(), inItem);

    while (m_RecentItems.size() > 10)
        m_RecentItems.pop_back();

    RebuildList();
}

void CRecentItems::RemoveRecentItem(const CUICFile &inItem)
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
        Q3DStudio::CString theKey;
        theKey.Format(_UIC("%ls%d"), static_cast<const wchar_t *>(m_PreferenceKey), theIndex);

        Q3DStudio::CString theFilename = thePrefs.GetStringValue(theKey, "");
        if (theFilename != "") {
            CUICFile theFile(theFilename);

            QAction *act = m_Menu->addAction(theFile.GetName().toQString(),
                                             this, &CRecentItems::onTriggerRecent);
            act->setData(static_cast<int>(m_RecentItems.size()));
            m_RecentItems.push_back(theFile);
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
        Q3DStudio::CString theFilename = (*thePos).GetName();

        QAction *act = m_Menu->addAction(theFilename.toQString(),
                                         this, &CRecentItems::onTriggerRecent);
        act->setData(static_cast<int>(theIndex));

        Q3DStudio::CString theKey;
        theKey.Format(_UIC("%ls%d"), static_cast<const wchar_t *>(m_PreferenceKey), theIndex);

        thePrefs.SetStringValue(theKey, (*thePos).GetAbsolutePath());
    }
}

void CRecentItems::ClearMenu()
{
    m_Menu->clear();
}

CUICFile CRecentItems::GetItem(long inIndex)
{
    return m_RecentItems.at(inIndex);
}

void CRecentItems::onTriggerRecent()
{
    const int index = qobject_cast<QAction *>(sender())->data().toInt();
    Q_EMIT openRecent(index);
}

