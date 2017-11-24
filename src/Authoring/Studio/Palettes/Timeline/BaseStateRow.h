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

#ifndef INCLUDED_BASE_STATE_ROW_H
#define INCLUDED_BASE_STATE_ROW_H 1

#pragma once

#include "stdafx.h"

#include "TimelineRow.h"
#include "ToggleButton.h"
#include "DispatchListeners.h"

class CPropertyRow;
class CStateRow;
class CCmdBatch;
class ITimelineItem;

struct SBaseStateRowSelectionKeyState
{
    enum Enum {
        NoKeyDown = 0,
        ShiftKey = 1 << 0,
        ControlKey = 1 << 1,
    };
    qt3ds::QT3DSU32 m_KeyState;
    SBaseStateRowSelectionKeyState()
        : m_KeyState(0)
    {
    }
    void SetShiftDown() { m_KeyState = m_KeyState | ShiftKey; }
    void SetControlDown() { m_KeyState = m_KeyState | ControlKey; }
    bool IsShiftDown() const { return (m_KeyState & ShiftKey) != 0; }
    bool IsControlDown() const { return (m_KeyState & ControlKey) != 0; }
};

class CBaseStateRow : public CTimelineRow
{
    Q_OBJECT
public:
    typedef std::vector<CPropertyRow *> TPropertyRowList;
    typedef std::vector<CStateRow *> TStateRowList;

public:
    CBaseStateRow(CTimelineRow *parent);
    virtual ~CBaseStateRow();

    void Initialize(ITimelineItemBinding *inTimelineItemBinding) override;

    bool IsExpanded();
    bool IsLoaded();
    void Expand(bool inExpandAll = false, bool inExpandUp = false) override;
    void Collapse(bool inCollapseAll = false) override;
    void ToggleExpansion(CToggleButton *, CButtonControl::EButtonState);

    void SetTimeRatio(double inTimePerPixel) override;

    void Select(SBaseStateRowSelectionKeyState inKeyState, bool inCheckKeySelection = true);
    void DeleteAllKeys();

    void LoadChildren() override;
    void AddChildRow(ITimelineItemBinding *inTimeLineItem, ITimelineItemBinding *inNextItem);
    void RemoveChildRow(ITimelineItemBinding *inTimeLineItem);

    void RemoveRow(CStateRow *inRow);
    void AddStateRow(CStateRow *inRow, CStateRow *inNextRow);
    void AddPropertyRow(CPropertyRow *inRow, CTimelineRow *inNextRow = nullptr);
    void RemovePropertyRow(const CPropertyRow *inRow);
    void RemoveAllChildren();
    void RemoveAllProperties();

    long GetNumNonPropertyRows() const;
    CBaseStateRow *GetNonPropertyRow(long inIndex) const;
    long GetNumPropertyRows() const;
    CPropertyRow *GetPropertyRow(long inIndex) const;

    void Filter(const CFilter &inFilter, bool inFilterChildren = true) override;
    CFilter *GetFilter() { return &m_Filter; }

    virtual bool HasVisibleChildren();

    // CTimelineRow
    virtual long GetEarliestStartTime();
    long GetLatestEndTime() override;

    long GetStartTime();
    long GetEndTime();
    void Dispose() override;

    virtual QPixmap GetIcon();
    virtual QPixmap GetDisabledIcon();

    TPropertyRowList GetPropertyRows() const;
    TStateRowList GetStateRows() const;

    virtual void OnSelected(bool inSelected);
    void RequestRefreshRowMetaData();
    void ForceEmitChildrenChanged();
    void requestSetNameReadOnly();
    void requestUpdateActionStatus();

Q_SIGNALS:
    void expanded(bool isExpanded);
    void timeRatioChanged(double timeRatio);
    void visibleChanged(bool visible);
    void hasChildrenChanged(bool hasChildren);
    void rowAboutToBeRemoved(CTimelineRow *row);
    void selectAllKeys();
    void selectedChanged(bool selected);
    void addRowToUILists(CTimelineRow *inRow, CTimelineRow *inNextRow, CFilter &inFilter);
    void rowAdded(CBaseStateRow *row);
    void refreshRowMetaData();
    void setNameReadOnly();
    void updateActionStatus();

protected:
    void DeletePropertyRow(CPropertyRow *inPropertyRow);
    virtual bool PerformFilter(const CFilter &inFilter) = 0;
    CStateRow *GetRow(ITimelineItem *inTimelineItem);
    void DeleteRow(CStateRow *inRow);
    // KDAB_TODO unused?
//    void SetTimelineLatestTime(long inLength);

    virtual void LoadProperties() {}
    void InitializePropertyRow(CPropertyRow *inRow, CTimelineRow *inNextRow = nullptr);

    CStateRow *CreateChildRow(ITimelineItemBinding *inChildBinding, CStateRow *inNextRow);

    CFilter m_Filter;

    TStateRowList m_StateRows;
    TPropertyRowList m_PropertyRows;

    bool m_Loaded;
    bool m_Selected;
};
#endif // INCLUDED_BASE_STATE_ROW_H
