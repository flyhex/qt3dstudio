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

#ifndef INCLUDED_BASE_TIMELINE_TREE_CONTROL_H
#define INCLUDED_BASE_TIMELINE_TREE_CONTROL_H 1

#pragma once

#include "Control.h"
#include "SIcon.h"
#include "ToggleButton.h"
#include "ITickTock.h"

class CButtonControl;
class CBaseStateRow;
class CDropTarget;
class CNameEdit;
class CTickTockProc;
struct STickTockHandle;
class CPt;
class CToggleButton;
class ITimelineItem;

class CBaseTimelineTreeControl : public CControl
{

public:
    enum ECONTROLREGION { ECONTROLREGION_ON, ECONTROLREGION_ABOVE, ECONTROLREGION_BELOW };

    CBaseTimelineTreeControl(CBaseStateRow *inStateRow, bool inMaster);
    virtual ~CBaseTimelineTreeControl();

    void Draw(CRenderer *inRenderer) override;
    void OnChildSizeChanged(CControl *inChild) override;

    void SetIndent(long inIndent);
    long GetIndent();

    void SetExpanded(bool inIsExpanded);

    void SetToggleVisible(bool inIsToggleVisible);
    void OnSelect();
    void OnDeselect();
    void GrabTextFocus();
    void OnGainFocus() override;
    void OnLoseFocus() override;
    void SetBackgroundColor(::CColor inBackgroundColor);

    void OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnKeyDown(unsigned int inChar, Qt::KeyboardModifiers inFlags) override;

    void OnTimer();

    void Refresh(ITimelineItem *inTimelineItem);

    void SetEnabled(bool inIsEnabled) override;

    CDropTarget *BuildDropTarget(CPt &inMousePoint, Qt::KeyboardModifiers inFlags);
    CDropTarget *FindDropCandidate(CPt &inMousePoint, Qt::KeyboardModifiers inFlags) override;
    void AcceptDropAfter(bool inAccept);
    void AcceptDropBefore(bool inAccept);
    void DoRename();

    void SetNameReadOnly(bool inReadOnly);

protected:
    ECONTROLREGION FindHitRegion(CPt &inMousePoint);
    void CreateText(); // delay text creation until row is exposed
    void UpdateTextSelection();

    long m_Indent;

    CBaseStateRow *m_StateRow;

    CToggleButton *m_ExpandButton;
    CNameEdit *m_Text;

    CSIcon *m_Icon;
    bool m_Selected;

    ::CColor m_BackgroundColor;

    CPt m_TrackingPoint;
    CPt m_MouseMovePoint;
    bool m_MouseDown;
    bool m_DrawAcceptBefore;
    bool m_DrawAcceptAfter;
    std::shared_ptr<qt3dsdm::ISignalConnection> m_TimerHandler;
    ::CColor m_NormalTextColor;
    ::CColor m_SelectedTextColor;
    ::CColor m_LockedTextColor;
};
#endif // INCLUDED_BASE_TIMELINE_TREE_CONTROL_H
