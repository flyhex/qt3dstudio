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

#include "TimelineRow.h"
#include "ListLayout.h"
#include "ToggleButton.h"
#include "DispatchListeners.h"

class CPropertyRow;
class CBaseTimelineTreeControl;
class CColorControl;
class CBlankToggleControl;
class CBaseTimebarlessRow;
class CStateRow;
class CCmdBatch;
class ITimelineItem;
class ITimelineItemBinding;

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
public:
    typedef std::vector<CPropertyRow *> TPropertyRowList;
    typedef std::vector<CStateRow *> TStateRowList;
    static const long DEFAULT_TOGGLE_LENGTH;

public:
    CBaseStateRow();
    virtual ~CBaseStateRow();

    virtual void Initialize(ITimelineItemBinding *inTimelineItemBinding);

    bool IsExpanded();
    bool IsLoaded();
    virtual void Expand(bool inExpandAll = false, bool inExpandUp = false);
    virtual void Collapse(bool inCollapseAll = false);
    void ToggleExpansion(CToggleButton *, CButtonControl::EButtonState);

    void SetTimeRatio(double inTimePerPixel) override;

    CControl *GetColorControl() override;
    CControl *GetTreeControl() override;
    CControl *GetToggleControl() override;
    CControl *GetTimebarControl() override;

    void Select(SBaseStateRowSelectionKeyState inKeyState, bool inCheckKeySelection = true);
    void SelectKeysInRect(CRct inRect, bool inModifierKeyDown, bool inGlobalCommitSelectionFlag);
    void DeleteAllKeys();

    virtual void OnMouseOver();
    virtual void OnMouseOut();
    virtual void OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual void OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual void OnDirty();
    virtual void OnSelected(bool inSelected);

    void LoadChildren();
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
    void OnChildVisibilityChanged() override;

    virtual bool HasVisibleChildren();

    void PopulateSnappingList(CSnapper *inSnappingList) override;

    virtual void SetEnabled(bool inEnabled);

    void DoStartDrag(CControlWindowListener *inWndListener);
    void AcceptDropAfter(bool inAccept);
    void AcceptDropBefore(bool inAccept);
    void SetDropTarget(CDropTarget *inDropTarget);

    // CTimelineRow
    virtual long GetEarliestStartTime();
    long GetLatestEndTime() override;

    long GetStartTime();
    long GetEndTime();
    long GetActiveStart();
    long GetActiveEnd();
    virtual bool CalculateActiveStartTime() = 0;
    virtual bool CalculateActiveEndTime() = 0;
    void Dispose() override;

    virtual QPixmap GetIcon();
    virtual QPixmap GetDisabledIcon();

    EStudioObjectType GetObjectType() const;
    ITimelineItemBinding *GetTimelineItemBinding() const;
    ITimelineItem *GetTimelineItem() const;

    void UpdateActionStatus();
    void SetFocus();

    CBaseTimebarlessRow *GetTimebar() const;

    void SetNameReadOnly(bool inReadOnly);

    void ClearDirty();

protected:
    void DeletePropertyRow(CPropertyRow *inPropertyRow);
    virtual CBlankToggleControl *CreateToggleControl();
    virtual CBaseTimebarlessRow *CreateTimebarRow() = 0;
    virtual bool PerformFilter(const CFilter &inFilter) = 0;
    CStateRow *GetRow(ITimelineItem *inTimelineItem);
    void DeleteRow(CStateRow *inRow);
    void SetTimelineLatestTime(long inLength);

    virtual void LoadProperties() {}
    void InitializePropertyRow(CPropertyRow *inRow, CTimelineRow *inNextRow = nullptr);

    void AddRowToUILists(CTimelineRow *inRow, CTimelineRow *inNextRow, CFilter &inFilter);
    CStateRow *CreateChildRow(ITimelineItemBinding *inChildBinding, CStateRow *inNextRow);

    double m_TimeRatio;
    CFilter m_Filter;
    CListLayout m_ColorList;
    CListLayout m_TreeList;
    CListLayout m_ToggleList;
    CListLayout m_TimebarList;

    CBaseTimelineTreeControl *m_TreeControl;
    CColorControl *m_ColorControl;
    CBlankToggleControl *m_ToggleControl;
    CBaseTimebarlessRow *m_TimebarControl;

    TStateRowList m_StateRows;
    TPropertyRowList m_PropertyRows;

    bool m_Loaded;
    bool m_IsExpanded;
    bool m_Highlighted;
    bool m_Dirty;
    bool m_Selected;

    ITimelineItemBinding *m_TimelineItemBinding;

    long m_ActiveStart;
    long m_ActiveEnd;
};
#endif // INCLUDED_BASE_STATE_ROW_H
