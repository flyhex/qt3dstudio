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

#include "TabOrderHandler.h"

TabOrderHandler::TabOrderHandler(QObject *parent)
    : QObject(parent)
{

}

TabOrderHandler::~TabOrderHandler()
{

}

void TabOrderHandler::addItem(int group, QQuickItem *item)
{
    m_itemMap[group].append(item);
}

void TabOrderHandler::clear()
{
    m_itemMap.clear();
}

void TabOrderHandler::clearGroup(int group)
{
    m_itemMap[group].clear();
}

void TabOrderHandler::tabNavigate(bool tabForward)
{
    // Find the currently focused control
    for (int i = 0; i < m_itemMap.size(); i++) {
        const QList<QQuickItem *> items = m_itemMap[i];
        for (int j = 0; j < items.size(); j++) {
            if (items[j]->hasActiveFocus()) {
                if (tabForward)
                    nextItem(i, j)->forceActiveFocus();
                else
                    previousItem(i, j)->forceActiveFocus();
                return;
            }
        }
    }
    // Activate the first item if could not find currently focused item
    for (int i = 0; i < m_itemMap.size(); i++) {
        if (m_itemMap[i].size() > 0)
            m_itemMap[i][0]->forceActiveFocus();
    }
}

QQuickItem *TabOrderHandler::nextItem(int group, int index)
{
    if (m_itemMap[group].size() > index + 1) {
        // Try next item in group
        index++;
    } else {
        // Get item in next available group
        int nextGroup = group + 1;
        while (nextGroup != group) {
            if (m_itemMap.size() >= nextGroup)
                nextGroup = 0;
            if (m_itemMap[nextGroup].size() == 0)
                nextGroup++;
            else
                group = nextGroup;
        }
        index = 0;
    }
    return m_itemMap[group][index];
}

QQuickItem *TabOrderHandler::previousItem(int group, int index)
{
    if (index - 1 >= 0) {
        // Try previous item in group
        index--;
    } else {
        // Get last item in previous available group
        int nextGroup = group - 1;
        while (nextGroup != group) {
            if (nextGroup < 0)
                nextGroup = m_itemMap.size() - 1;
            if (m_itemMap[nextGroup].size() == 0)
                nextGroup--;
            else
                group = nextGroup;
        }
        index = m_itemMap[group].size() - 1;
    }
    return m_itemMap[group][index];
}

