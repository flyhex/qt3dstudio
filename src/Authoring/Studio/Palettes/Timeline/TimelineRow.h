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

#ifndef INCLUDED_TIMELINE_ROW_H
#define INCLUDED_TIMELINE_ROW_H 1

#pragma once

#include "TimelineFilter.h"
#include "CColor.h"
#include "StudioObjectTypes.h"

#include <vector>

class CSnapper;
class CControl;
class CBaseStateRow;
class ISnappingListProvider;
class ITimelineControl;

class CTimelineRow
{
public:
    static const long TREE_INDENT;

    CTimelineRow();
    virtual ~CTimelineRow();

    virtual CControl *GetColorControl() = 0;
    virtual CControl *GetTreeControl() = 0;
    virtual CControl *GetToggleControl() = 0;
    virtual CControl *GetTimebarControl() = 0;

    virtual void SetIndent(long inIndent);
    long GetIndent();

    virtual void Filter(const CFilter &inFilter, bool inFilterChildren = true) = 0;

    void SetParent(CBaseStateRow *inParent);
    CBaseStateRow *GetParentRow() const;
    virtual void SetTimeRatio(double inTimeRatio);
    virtual void OnChildVisibilityChanged();
    virtual bool IsViewable() const;
    virtual void PopulateSnappingList(CSnapper *inSnappingList);
    virtual ISnappingListProvider *GetSnappingListProvider() const = 0;
    virtual void SetSnappingListProvider(ISnappingListProvider *inProvider) = 0;
    virtual ITimelineControl *GetTopControl() const;

    virtual ::CColor GetTimebarBackgroundColor(EStudioObjectType inType);
    virtual ::CColor GetTimebarHighlightBackgroundColor(EStudioObjectType inType);

    virtual long GetLatestEndTime();

    virtual void Dispose();

protected:
    CBaseStateRow *m_ParentRow;
    bool m_IsViewable;
    long m_Indent;
};
#endif // INCLUDED_TIMELINE_ROW_H
