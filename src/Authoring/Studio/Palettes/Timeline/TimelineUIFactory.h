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
#ifndef TIMELINEUIFACTORY_H
#define TIMELINEUIFACTORY_H

#include <QObject>
#include <QHash>

#include "TimelineRow.h"
#include "AbstractTimelineRowUI.h"

class CBaseStateRow;
class CPropertyRow;
class CSlideRow;
class CStateRow;

class TimelineUIFactory : public QObject
{
    Q_OBJECT

public:
    static TimelineUIFactory *instance();
    virtual ~TimelineUIFactory();

    CAbstractTimelineRowUI *uiForRow(CTimelineRow *row);

    CPropertyRow *createPropertyRow(CBaseStateRow *parentRow, CPropertyRow *nextRow, ITimelineItemProperty *inTimelineItemPropertyBinding);
    CStateRow *createStateRow(CBaseStateRow *parentRow, ITimelineItemBinding *inTimelineItem);
    CSlideRow *createSlideRow(CBaseStateRow *parentRow, ITimelineItemBinding *inTimelineItem);

    void deleteRowUI(CTimelineRow *row);
    void setParentForRowUI(CTimelineRow *row, CTimelineRow *parentRow);

private:
    CAbstractTimelineRowUI *createRowUI(CTimelineRow *row, CTimelineRow *parentRow);

    TimelineUIFactory(QObject *parent = nullptr);
    QHash<CTimelineRow *, CAbstractTimelineRowUI *> m_uiRows;
};

#endif // TIMELINEUIFACTORY_H
