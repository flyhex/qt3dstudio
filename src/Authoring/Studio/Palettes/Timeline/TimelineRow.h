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

#include <QObject>

#include "TimelineFilter.h"
#include "CColor.h"
#include "StudioObjectTypes.h"

#include <vector>

class CSnapper;
class CControl;
class ISnappingListProvider;
class ITimelineItemBinding;
class CPropertyRow;

const double DEFAULT_TIME_RATIO = .05;

class CTimelineRow : public QObject
{
    Q_OBJECT
public:
    static const long TREE_INDENT;

    explicit CTimelineRow(CTimelineRow *parent);
    virtual ~CTimelineRow();

    virtual void Initialize(ITimelineItemBinding *inTimelineItemBinding) = 0;
    virtual void Filter(const CFilter &inFilter, bool inFilterChildren = true) = 0;

    virtual void Select(Qt::KeyboardModifiers inKeyState, bool inCheckKeySelection = true) = 0;
    virtual bool IsSelected() const = 0;

    void SetParent(CTimelineRow *inParent);
    CTimelineRow *GetParentRow() const;
    virtual void SetTimeRatio(double inTimeRatio);
    virtual double GetTimeRatio();
    virtual bool IsViewable() const;

    virtual ::CColor GetTimebarBackgroundColor(EStudioObjectType inType);
    virtual ::CColor GetTimebarHighlightBackgroundColor(EStudioObjectType inType);

    virtual long GetLatestEndTime();

    virtual void Dispose();

    virtual void Expand(bool inExpandAll = false, bool inExpandUp = false) = 0;
    virtual void Collapse(bool inCollapseAll = false) = 0;
    bool isExpanded() const;

    virtual bool CalculateActiveStartTime() = 0;
    virtual bool CalculateActiveEndTime() = 0;
    long GetActiveStart();
    long GetActiveEnd();

    void setDirty(bool dirty);

    virtual void LoadChildren() = 0;
    virtual bool HasVisibleChildren() = 0;

    ITimelineItemBinding *GetTimelineItemBinding() const;
    EStudioObjectType GetObjectType() const;
    ITimelineItem *GetTimelineItem() const;

    void RequestSelectKeysByTime(long inTime, bool inSelected);

Q_SIGNALS:
    void initialized();
    void dirtyChanged(bool dirty);
    void propertyRowAdded(CPropertyRow *newRow);
    void childrenLoaded();
    void selectKeysByTime(long inTime, bool inSelected);
    void selectedChanged(bool selected);
    void timeRatioChanged(double timeRatio);

protected:
    CTimelineRow *m_ParentRow;

    ITimelineItemBinding *m_TimelineItemBinding = nullptr;

    bool m_IsViewable;
    long m_ActiveStart;
    long m_ActiveEnd;
    bool m_Dirty = false;
    bool m_IsExpanded = false;

    double m_TimeRatio = DEFAULT_TIME_RATIO;
};
#endif // INCLUDED_TIMELINE_ROW_H
