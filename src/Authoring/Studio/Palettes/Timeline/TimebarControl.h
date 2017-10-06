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

#ifndef INCLUDED_TIMEBAR_CONTROL_H
#define INCLUDED_TIMEBAR_CONTROL_H 1

#pragma once

#include "Control.h"
#include "Snapper.h"
#include "CommentEdit.h"
#include "TimebarTip.h"
#include "TimeEditDlg.h"
#include "KeyframeContextMenu.h"

class CStateTimebarRow;
class ITimelineItemBinding;
class ISnappingListProvider;

//=============================================================================
/**
 * Interface to a timebar control
 */
class ITimebarControl
{
public:
    virtual ~ITimebarControl() {}

    virtual ITimelineItemKeyframesHolder *GetKeyframesHolder() = 0;
    virtual void OnEditTimeComment() = 0;
    virtual void OnToggleTimebarHandles() = 0;
    virtual void SetTimebarTime() = 0;
    virtual ::CColor GetTimebarColor() = 0;
    virtual void SetTimebarColor(const ::CColor &inColor) = 0;
};

class CTimebarControl : public CControl, public ITimeChangeCallback, public ITimebarControl
{
public:
    CTimebarControl(CStateTimebarRow *inRow, ITimelineItemBinding *inTimelineItemBinding);
    virtual ~CTimebarControl();
    void Draw(CRenderer *inRenderer) override;
    void SetSize(CPt inSize) override;
    void SetSelected(bool inIsSelected);
    void SetTimeRatio(double inTimeRatio);
    void Refresh();
    void Refresh(long inStartTime, long inEndTime);
    void RefreshMetaData();

    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void ResizeTimebarLeftTo(long inTime);
    void ResizeTimebarRightTo(long inTime);
    void SetText(const Q3DStudio::CString &inText);
    void SetTextColor(::CColor inColor = ::CColor(0, 0, 0));
    void SetEnabled(bool inIsEnabled) override;

    long GetStartTime();
    long GetEndTime();

    virtual void PopulateSnappingList(CSnapper *inSnapper);
    void OnLoseFocus() override;

    void OnBeginDrag();

    // ITimeChangeCallback
    void ChangeStartTime(long) override;
    void ChangeEndTime(long) override;
    void Commit() override;
    void Rollback() override;

    void ShowContextMenu(CPt inPoint, bool inShowTimebarPropertiesOptions);
    void CommitTimeChange();

    // ITimebarControl
    ITimelineItemKeyframesHolder *GetKeyframesHolder() override;
    void OnEditTimeComment() override;
    void OnToggleTimebarHandles() override;
    void SetTimebarTime() override;
    ::CColor GetTimebarColor() override;
    void SetTimebarColor(const ::CColor &inColor) override;

    void SetSnappingListProvider(ISnappingListProvider *inProvider);
    ISnappingListProvider &GetSnappingListProvider() const;

    CRct GetTopControlBounds() const;

protected:
    ITimelineTimebar *GetTimebar();

    void RefreshToolTip(CPt inPoint);
    void DrawHashedBackgroundX(CRenderer *inRenderer, ::CColor inFirstColor, ::CColor inSecondColor,
                               CRct inRect);

    CStateTimebarRow *m_TimebarRow;
    bool m_IsSelected;
    double m_TimeRatio;
    bool m_IsMouseDown;
    bool m_MaybeDragStart;
    CPt m_MouseDownLoc;
    long m_StartTime;
    long m_EndTime;

    CTimebarTip m_LeftLeftTip;
    CTimebarTip m_LeftTip;

    CTimebarTip m_RightTip;
    CTimebarTip m_RightRightTip;
    CCommentEdit *m_EditControl;
    CSnapper m_Snapper;

    ITimelineItemBinding *m_TimelineItemBinding;
    ISnappingListProvider *m_SnappingListProvider;
};
#endif // INCLUDED_TIMEBAR_CONTROL_H
