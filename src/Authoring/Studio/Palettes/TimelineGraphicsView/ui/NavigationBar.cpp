/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "NavigationBar.h"
#include "NavigationBarItem.h"
#include <QtCore/qdebug.h>

NavigationBar::NavigationBar(QWidget *parent)
    : QWidget(parent)
{
    const int barHeight = 30;
    setMinimumHeight(barHeight);
    m_layout = new QHBoxLayout(this);
    m_layout->setMargin(4);
    m_layout->setSpacing(4);
    setLayout(m_layout);
}

void NavigationBar::updateNavigationItems(IBreadCrumbProvider *inBreadCrumbProvider)
{
    if (!inBreadCrumbProvider)
        return;

    m_breadCrumbProvider = inBreadCrumbProvider;

    const IBreadCrumbProvider::TTrailList &trailList = m_breadCrumbProvider->GetTrail();
    int listSize = (int)trailList.size();

    // Remove "stretch" from end
    QLayoutItem *stretch = m_layout->takeAt(m_layout->count() - 1);
    if (stretch)
        delete stretch;

    // Update current items or create new as needed
    // By design, if there is only 1 item in the list, nothing is shown.
    if (listSize > 1) {
        for (int i = 0; i < listSize; ++i) {
            SBreadCrumb item = trailList.at(i);
            NavigationBarItem *barItem = nullptr;
            bool newItem = (m_itemAmount <= 1) || (i > m_itemAmount - 1);
            if (newItem) {
                barItem = new NavigationBarItem(this);
            } else {
                // Every other item is NavigationBarItem, every other separator
                int barItemIndex = i * 2;
                barItem = static_cast<NavigationBarItem *>(
                            m_layout->itemAt(barItemIndex)->widget());
                barItem->setHighlight(false);
            }
            bool isLastItem = (i == listSize - 1);
            barItem->setEnabled(!isLastItem);
            barItem->setIndex(i);
            barItem->setText(item.m_String);
            if (i == 0)
                barItem->setIcon(m_breadCrumbProvider->GetRootImage());
            else
                barItem->setIcon(m_breadCrumbProvider->GetBreadCrumbImage());

            if (newItem) {
                QObject::connect(barItem, &NavigationBarItem::clicked,
                                 this, &NavigationBar::itemClicked);
                if (i != 0) {
                    // Separator before all items except first
                    QLabel *separator = new QLabel(this);
                    separator->setPixmap(m_breadCrumbProvider->GetSeparatorImage());
                    m_layout->addWidget(separator);
                }
                m_layout->addWidget(barItem);
            }
        }
    }

    // Remove possible extra items, when user navigates back
    // When list contains single item, remove all by design
    QLayoutItem *child;
    int lastIndex = (listSize <= 1) ? 0 : (listSize * 2) - 1;
    while ((child = m_layout->takeAt(lastIndex)) != 0) {
        if (child->widget())
            delete child->widget();
        delete child;
    }

    // Stretch at end for proper item sizing
    m_layout->addStretch(1);

    m_itemAmount = listSize;
}

void NavigationBar::itemClicked(int index)
{
    m_breadCrumbProvider->OnBreadCrumbClicked((long)index);
}
