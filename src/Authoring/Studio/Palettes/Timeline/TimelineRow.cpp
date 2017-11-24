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

#include "TimelineRow.h"
#include "TimelineUIFactory.h"
#include "StudioPreferences.h"
#include "StudioObjectTypes.h"

#include "Bindings/ITimelineItemBinding.h"

const long CTimelineRow::TREE_INDENT = CStudioPreferences::GetRowSize();

CTimelineRow::CTimelineRow(CTimelineRow *parent)
    : QObject(parent)
    , m_ParentRow(nullptr)
    , m_IsViewable(false)
    , m_ActiveStart(0)
    , m_ActiveEnd(0)
    , m_TimeRatio(0.0f)
{
}

CTimelineRow::~CTimelineRow()
{
    TimelineUIFactory::instance()->deleteRowUI(this);
}

void CTimelineRow::SetParent(CTimelineRow *inParent)
{
    if (m_ParentRow != inParent) {
        m_ParentRow = inParent;
        TimelineUIFactory::instance()->setParentForRowUI(this, m_ParentRow);
    }
}

//=============================================================================
/**
 * Gets the Parent Row
 */
CTimelineRow *CTimelineRow::GetParentRow() const
{
    return m_ParentRow;
}

void CTimelineRow::SetTimeRatio(double inTimeRatio)
{
    m_TimeRatio = inTimeRatio;
}

double CTimelineRow::GetTimeRatio()
{
    return m_TimeRatio;
}

bool CTimelineRow::IsViewable() const
{
    return m_IsViewable;
}


//=============================================================================
/**
 * Retrieves the background color for the row based upon the type of asset
 * passed in.
 * @param inType specifies which asset type you want the color for
 * @return background color to use for this row
 */
::CColor CTimelineRow::GetTimebarBackgroundColor(EStudioObjectType inType)
{
    ::CColor theColor;

    switch (inType) {
    case OBJTYPE_LAYER:
        theColor = CStudioPreferences::GetLayerBackgroundColor();
        break;

    case OBJTYPE_GROUP:
    case OBJTYPE_COMPONENT:
        theColor = CStudioPreferences::GetGroupBackgroundColor();
        break;

    default:
        theColor = CStudioPreferences::GetObjectBackgroundColor();
        break;
    }

    return theColor;
}

//=============================================================================
/**
 * Retrieves the background color for the row when the mouse is over the row,
 * based upon the type of asset passed in.
 * @param inType specifies which asset type you want the color for
 * @return background color to use for this row when the mouse is over the row
 */
::CColor CTimelineRow::GetTimebarHighlightBackgroundColor(EStudioObjectType inType)
{
    ::CColor theColor;

    switch (inType) {
    case OBJTYPE_LAYER:
        theColor = CStudioPreferences::GetMouseOverHighlightColor();
        break;

    case OBJTYPE_GROUP:
    case OBJTYPE_COMPONENT:
        theColor = CStudioPreferences::GetMouseOverHighlightColor();
        break;

    default:
        theColor = CStudioPreferences::GetMouseOverHighlightColor();
        break;
    }

    return theColor;
}

long CTimelineRow::GetLatestEndTime()
{
    return 0;
}

void CTimelineRow::Dispose()
{
    delete this;
}

bool CTimelineRow::isExpanded() const
{
    return m_IsExpanded;
}

long CTimelineRow::GetActiveStart()
{
    return m_ActiveStart;
}

long CTimelineRow::GetActiveEnd()
{
    return m_ActiveEnd;
}

void CTimelineRow::setDirty(bool dirty)
{
    if (m_Dirty == dirty)
        return;
    m_Dirty = dirty;
    emit dirtyChanged(dirty);
}

ITimelineItemBinding *CTimelineRow::GetTimelineItemBinding() const
{
    return m_TimelineItemBinding;
}

//=============================================================================
/**
 * @return the studio type of the object represented by this row
 */
EStudioObjectType CTimelineRow::GetObjectType() const
{
    return GetTimelineItem()->GetObjectType();
}

ITimelineItem *CTimelineRow::GetTimelineItem() const
{
    return m_TimelineItemBinding->GetTimelineItem();
}

void CTimelineRow::RequestSelectKeysByTime(long inTime, bool inSelected)
{
    emit selectKeysByTime(inTime, inSelected);
}
