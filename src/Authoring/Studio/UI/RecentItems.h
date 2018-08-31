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

#ifndef INCLUDED_RECENT_ITEMS_H
#define INCLUDED_RECENT_ITEMS_H 1

#pragma once

#include <QObject>

#include <vector>

QT_FORWARD_DECLARE_CLASS(QMenu)

class CRecentItems : public QObject
{
    Q_OBJECT

    typedef std::vector<QString> TFileList;

public:
    static const QString RECENTITEM_KEY;
    static const QString RECENTIMPORT_KEY;
    static const QString RECENTITEM_VALID;

Q_SIGNALS:
    void openRecent(int index);
public:
    CRecentItems(QMenu *inMenu, long inCommandID,
                 QString inPreferenceKey = RECENTITEM_KEY);
    virtual ~CRecentItems();

    void AddRecentItem(const QString &inItem);
    void RemoveRecentItem(const QString &inItem);

    QString GetItem(long inIndex);
    long GetItemCount() const { return (long)m_RecentItems.size(); }

protected:
    void ClearMenu();
    void ReconstructList();
    void RebuildList();
    void SaveRecentList();
    void handleAboutToShow();

    TFileList m_RecentItems;

    long m_CommandID;
    long m_ValidItems;
    QMenu *m_Menu;
    QString m_PreferenceKey;

private Q_SLOTS:
    void onTriggerRecent();
};
#endif // INCLUDED_RECENT_ITEMS_H
