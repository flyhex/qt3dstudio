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

#include "AbstractTimelineRowUI.h"
#include "TimelineRow.h"
#include "ITimelineControl.h"

#include "Bindings/ITimelineItemBinding.h"

CAbstractTimelineRowUI::CAbstractTimelineRowUI(CTimelineRow *timelineRow,
                                               CAbstractTimelineRowUI *parentUiRow)
    : QObject(parentUiRow)
    , m_timelineRow(timelineRow)
    , m_parentRowUI(parentUiRow)
    , m_Indent(0)
{
}

CAbstractTimelineRowUI::~CAbstractTimelineRowUI()
{
}

void CAbstractTimelineRowUI::SetParentRow(CAbstractTimelineRowUI *parentUiRow)
{
    m_parentRowUI = parentUiRow;
    setParent(parentUiRow);
}

CTimelineRow *CAbstractTimelineRowUI::GetTimelineRow() const
{
    return m_timelineRow;
}

void CAbstractTimelineRowUI::SetTimelineControl(ITimelineControl *inTimelineControl)
{
    m_TimelineControl = inTimelineControl;
}

ITimelineControl *CAbstractTimelineRowUI::GetTopControl() const
{
    auto *parentRow = m_timelineRow->GetParentRow();
    ITimelineControl *theControl = parentRow ? m_parentRowUI->GetTopControl() : m_TimelineControl;
    // GetTopControl() should be not used if SetTimeLineControl() was not called in this item
    Q_ASSERT(theControl);
    return theControl;
}

void CAbstractTimelineRowUI::SetIndent(long indent)
{
    m_Indent = indent;
}

long CAbstractTimelineRowUI::GetIndent() const
{
    return m_Indent;
}
