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
#include "TimelineUIFactory.h"

#include "TimelineRow.h"
#include "StateRow.h"
#include "SlideRow.h"
#include "PropertyRow.h"

#include "Bindings/ITimelineItemProperty.h"

TimelineUIFactory *TimelineUIFactory::instance()
{
    static TimelineUIFactory *s_instance = new TimelineUIFactory();
    return s_instance;
}

TimelineUIFactory::~TimelineUIFactory()
{

}

CAbstractTimelineRowUI *TimelineUIFactory::uiForRow(CTimelineRow *row)
{
    auto uiRow = m_uiRows.value(row, nullptr);
    Q_ASSERT(uiRow);
    return uiRow;
}

CPropertyRow *TimelineUIFactory::createPropertyRow(CBaseStateRow *parentRow,
                                                   CPropertyRow *nextRow,
                                                   ITimelineItemProperty *inTimelineItemPropertyBinding)
{
    auto propertyRow = new CPropertyRow(inTimelineItemPropertyBinding, parentRow);

    inTimelineItemPropertyBinding->Bind(propertyRow);

    return propertyRow;
}

CStateRow *TimelineUIFactory::createStateRow(CBaseStateRow *parentRow,
                                             ITimelineItemBinding *inTimelineItem)
{
    auto stateRow = new CStateRow(parentRow);

    stateRow->Initialize(inTimelineItem);

    return stateRow;
}

CSlideRow *TimelineUIFactory::createSlideRow(CBaseStateRow *parentRow,
                                             ITimelineItemBinding *inTimelineItem)
{
    auto slideRow = new CSlideRow(parentRow);

    slideRow->Initialize(inTimelineItem);

    return slideRow;
}

void TimelineUIFactory::deleteRowUI(CTimelineRow *row)
{
    auto it = m_uiRows.find(row);
    if (it != m_uiRows.end()) {
        delete *it;
        m_uiRows.erase(it);
    }
}

void TimelineUIFactory::setParentForRowUI(CTimelineRow *row, CTimelineRow *parentRow)
{
    CAbstractTimelineRowUI *uiRow = m_uiRows.value(row, nullptr);
    if (uiRow) {
        CAbstractTimelineRowUI *parentUiRow = m_uiRows.value(parentRow, nullptr);
        uiRow->SetParentRow(parentUiRow);
    }
}

TimelineUIFactory::TimelineUIFactory(QObject *parent)
{

}
