/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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
#ifndef INCLUDED_ITIMELINE_ITEM_BINDINGS_H
#define INCLUDED_ITIMELINE_ITEM_BINDINGS_H 1

#pragma once

#include "ITimelineItem.h"
#include "ITimelineItemProperty.h"
#include "SIterator.h"

class RowTree;
class CControlWindowListener;

// Data model specific ??
class CDropTarget;

class ITimelineItemKeyframesHolder
{
public:
    virtual ~ITimelineItemKeyframesHolder() {}

    virtual void InsertKeyframe() = 0;
    virtual void DeleteAllChannelKeyframes() = 0;
    virtual IKeyframe *GetKeyframeByTime(long inTime) const = 0;
};

//=============================================================================
/**
 * Interface to encapsulate data model specific functions, that Timeline UI objects can talk to.
 */
//=============================================================================
class ITimelineItemBinding : public ITimelineItemKeyframesHolder
{
public:
    // List of possible transactions that requires querying the data model if they are valid
    enum EUserTransaction {
        EUserTransaction_None,
        EUserTransaction_Rename,
        EUserTransaction_Duplicate,
        EUserTransaction_Cut,
        EUserTransaction_Copy,
        EUserTransaction_Paste,
        EUserTransaction_Delete,
        EUserTransaction_MakeComponent,
        EUserTransaction_EditComponent,
        EUserTransaction_MakeAnimatable,
        EUserTransaction_Group,
        EUserTransaction_Ungroup,
        EUserTransaction_AddLayer,
    };

public:
    virtual ~ITimelineItemBinding() {}

    virtual ITimelineItem *GetTimelineItem() = 0;
    virtual RowTree *getRowTree() const = 0; // UI
    virtual void setRowTree(RowTree *row) = 0;

    // Events
    virtual void SetSelected(bool multiSelect) = 0;
    virtual void OnCollapsed() = 0;
    virtual bool OpenAssociatedEditor() = 0;
    virtual void SetDropTarget(CDropTarget *inTarget) = 0;

    // Hierarchy
    virtual long GetChildrenCount() = 0;
    virtual ITimelineItemBinding *GetChild(long inIndex) = 0;
    virtual QList<ITimelineItemBinding *> GetChildren() = 0;
    virtual ITimelineItemBinding *GetParent() = 0;
    virtual void SetParent(ITimelineItemBinding *parent) = 0;
    // Properties
    virtual long GetPropertyCount() = 0;
    virtual ITimelineItemProperty *GetProperty(long inIndex) = 0;

    // Eye/Lock toggles
    virtual bool ShowToggleControls() const = 0;
    virtual bool IsLockedEnabled() const = 0;
    virtual bool IsVisibleEnabled() const = 0;

    // ContextMenu
    virtual bool IsValidTransaction(EUserTransaction inTransaction) = 0;
    virtual void PerformTransaction(EUserTransaction inTransaction) = 0;
    virtual QString GetObjectPath() = 0;

    virtual bool IsExternalizeable() { return false; }
    virtual void Externalize() {}
    virtual bool IsInternalizeable() { return false; }
    virtual void Internalize() {}

    void setCreateUIRow(bool create) { m_createUIRow = create; }

protected:
    bool m_createUIRow = true; // control creation of UI row for old style timeline UI
};

//=============================================================================
/**
 * Helper iterator class that iterates over a ITimeline's children in a ordered (priority) list.
 */
//=============================================================================
class CTimelineItemOrderedIterator : public CSIterator<ITimelineItemBinding *>
{
public:
    CTimelineItemOrderedIterator(ITimelineItemBinding *inRootTimelineItem)
    {
        m_RootTimelineItem = inRootTimelineItem;
        Reset();
    }
    bool IsDone() override { return (m_Index >= m_Total); }
    void operator++() override { m_Index++; }
    void operator+=(const long inNumToInc) override { m_Index += inNumToInc; }
    ITimelineItemBinding *GetCurrent() override { return m_RootTimelineItem->GetChild(m_Index); }
    virtual void Reset()
    {
        m_Index = 0;
        m_Total = m_RootTimelineItem->GetChildrenCount();
    }

protected:
    ITimelineItemBinding *m_RootTimelineItem;
    long m_Index;
    long m_Total;
};

//=============================================================================
/**
 * Helper iterator class that iterates over a ITimeline's properties
 */
//=============================================================================
class CTimelineItemPropertyIterator : public CSIterator<ITimelineItemProperty *>
{
public:
    CTimelineItemPropertyIterator(ITimelineItemBinding *inTimelineItem)
    {
        m_TimelineItem = inTimelineItem;
        Reset();
    }
    bool IsDone() override { return (m_Index >= m_Total); }
    void operator++() override { m_Index++; }
    void operator+=(const long inNumToInc) override { m_Index += inNumToInc; }
    ITimelineItemProperty *GetCurrent() override { return m_TimelineItem->GetProperty(m_Index); }
    virtual void Reset()
    {
        m_Index = 0;
        m_Total = m_TimelineItem->GetPropertyCount();
    }

protected:
    ITimelineItemBinding *m_TimelineItem;
    long m_Index;
    long m_Total;
};

#endif // INCLUDED_ITIMELINE_ITEM_BINDINGS_H
