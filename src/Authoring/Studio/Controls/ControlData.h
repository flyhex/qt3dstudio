/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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
#pragma once
#ifndef CONTROLDATAH
#define CONTROLDATAH

#include "DropTarget.h"
#include "Pt.h"
#include "Rct.h"
#include "Multicaster.h"
#include "Qt3DSString.h"

class CControl;
class CControlWindowListener;
class CChildFocusListener;
class CRenderer;

class QMenu;

namespace Q3DStudio {
namespace Control {

    using namespace std;

    struct ControlEventState
    {
        enum Enum {
            Unknown = 0,
            Listening,
            Ignoring,
        };
    };

    struct SControlEventData
    {
        bool m_Enabled;
        ControlEventState::Enum m_EventState;
        SControlEventData()
            : m_Enabled(false)
            , m_EventState(ControlEventState::Ignoring)
        {
        }
    };

    // Smart pointer object that referees access to a control.
    class CControlData
    {
        Q_DISABLE_COPY(CControlData)
    public:
        friend class ::CControl;
        friend class std::shared_ptr<CControlData>;

    private:
        // The physical address of the control is the key in the graph
        // that points to this data item.
        CControl *m_Control;

        CPt m_Size; ///< Current size of this control

        CPt m_Position; ///< Position of this control, relative to the parent
        CPt m_MinSize; ///< Minimum allowed size of this control
        CPt m_MaxSize; ///< Maximum allowed size of this contrl
        CPt m_PrefSize; ///< Preferred size of this control

        bool m_IsMouseOver; ///< True if the mouse is over this control
        bool m_IsInvalidated; ///< True if this control needs to be redrawn
        bool m_IsChildInvalidated; ///< True if a child of this control is invalidated.
        bool m_IsVisible; ///< True if this control is to be visible
        bool m_IsEnabled; ///< True if this control is enabled.
        bool m_IsParentEnabled;
        bool m_HasFocus;
        bool m_IsMouseDown;
        bool m_ShowTooltips; ///< Specifies whether or not tooltips should be shown
        bool m_NeedsLayout; ///< True if this control needs to be layed out.
        bool m_ChildrenNeedLayout; ///< True if my children need layout
        SControlEventData
            m_MouseWheelState; ///< Tracks whether this object cares about mouse wheel.

        Q3DStudio::CString m_TooltipText; ///< Text to be displayed for tooltips

        std::shared_ptr<CControlData> m_Focus; ///< Child control that has the focus.
        std::shared_ptr<CControlData> m_MouseFocus; ///< Child control that got the mouse down.
        CControlWindowListener
            *m_WindowListener; ///< External listener for when this control is invalidated.

        Q3DStudio::CString m_ControlName;
        CMulticaster<CChildFocusListener *> m_FocusListeners; ///< Used for focus changes

    public:
        CControlData(CControl &inControl);
        ~CControlData();

        CControl *GetControl();

        CControl *GetParent();

        void OnDraw(CRenderer *inRenderer, CRct &inDirtyRect, bool inIgnoreValidation = false);
        void Draw(CRenderer *inRenderer);
        void NotifyNotInClipRect();

        CPt GetPosition() const;
        void SetPosition(CPt inPosition);
        void SetPosition(long inX, long inY);

        CPt GetSize() const;
        void SetSize(CPt inSize);
        void SetSize(long inWidth, long inHeight);

        CPt GetMinimumSize();
        void SetMinimumSize(CPt inSize);

        CPt GetMaximumSize();
        void SetMaximumSize(CPt inSize);

        CPt GetPreferredSize();
        void SetPreferredSize(CPt inSize);

        void SetAbsoluteSize(CPt inSize);

        void SetMouseDown(bool inMouseDown);
        void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags);
        void OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags);
        void OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags);
        bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags);
        bool OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags);
        void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags);
        void OnMouseRUp(CPt inPoint, Qt::KeyboardModifiers inFlags);
        void OnMouseClick(CPt inPoint, Qt::KeyboardModifiers inFlags);
        bool OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags);
        bool OnMouseHover(CPt inPoint, Qt::KeyboardModifiers inFlags);
        bool OnMouseWheel(CPt inPoint, long inAmount, Qt::KeyboardModifiers inFlags);
        bool OnKeyDown(unsigned int inChar, Qt::KeyboardModifiers inFlags);
        bool OnKeyUp(unsigned int inChar, Qt::KeyboardModifiers inFlags);
        bool OnChar(const QString &inChar, Qt::KeyboardModifiers inFlags);
        void OnLoseFocus();
        void OnGainFocus();
        bool CanGainFocus();
        bool IsInFocus();
        void OnTab();
        void OnReverseTab();
        void SetFocusToFirstAvailable();
        void SetFocusToLastAvailable();
        void ChildrenChanged();

        // Mouse wheel event state.
        void SetMouseWheelEnabled(bool inEnabled);
        /**
                Callback so we can update any event states to unknown
                from ignoring forcing a refresh of hierarchy event information
                the next time the event state is queried.
        */
        void OnHierarchyChanged();
        /**
                If our event state is unknown, force resolution by asking children what their
                event state is.  Else return our event state.
         */
        ControlEventState::Enum GetMouseWheelEventState();

    protected:
        void UpdateMouseWheelEventState();
        void ChildMouseWheelEventStateChanged(ControlEventState::Enum inNewState);
        void SetMouseWheelEventState(ControlEventState::Enum inNewState);

    public:
        CDropTarget *FindDropCandidate(CPt &inMousePoint, Qt::KeyboardModifiers inFlags);

        void AddChild(CControl *inControl, CControl *inInsertBefore = NULL);
        void RemoveChild(CControl *inControl);
        void RemoveAllChildren();
        long GetChildIndex(CControl *inChildControl);
        CControl *FindChildByName(const Q3DStudio::CString &inName);

        bool IsMouseOver() const;

        bool HitTest(const CPt &inPoint) const;
        bool IsInRect(const CRct &inRect) const;

        void Invalidate(bool inInvalidate = true);
        void InvalidateRect(const CRct &inRect);
        bool IsInvalidated() const;

        void OnChildInvalidated();
        bool IsChildInvalidated() const;

        void SetVisible(bool inIsVisible);
        bool IsVisible() const;
        void OnVisibleStateChange(bool inIsVisible);
        void OnParentVisibleStateChanged(bool inIsVisible);

        void SetParentEnabled(bool inParentEnabled);
        void SetEnabled(bool inIsEnabled);
        bool IsEnabled() const;
        bool GetEnabledFlag();
        void SetEnabledFlag(bool inIsEnabled);

        void SetWindowListener(CControlWindowListener *inListener);
        CControlWindowListener *GetWindowListener();

        void OnChildSizeChanged(CControl *inChild);
        void ResetMinMaxPref();

        void SetName(Q3DStudio::CString inName);
        Q3DStudio::CString GetName();

        void BeginDrawChildren(CRenderer *inRenderer);

        long DoPopup(QMenu *inContextMenu, CPt inPoint);
        void RemoveUberControl(CControl *inControl);
        void OffsetPosition(CPt inOffset);

        CPt GetGlobalPosition(CPt inChildPoint) const;
        Qt3DSRenderDevice GetPlatformDevice();
        bool IsChildPlatformDevice(Qt3DSRenderDevice inDevice);
        void ShowMoveableWindow(CPt inLocation, Q3DStudio::CString inText, CRct inBoundingRct);
        void HideMoveableWindow();
        CControl *GetFirstChild();
        bool HasFocus(CControl *inControl);
        void GrabFocus(CControl *inControl);

        CPt ClientToScreen(CPt inPoint);
        CPt ScreenToClient(CPt inPoint);

        void AddFocusListener(CChildFocusListener *inListener);
        void RemoveFocusListener(CChildFocusListener *inListener);
        void FireFocusEvent(bool inStatus);

        void SetTooltipText(const Q3DStudio::CString &inText);
        Q3DStudio::CString GetTooltipText();

        void EnsureVisible(CRct inRect);
        void EnsureVisible();

        void OnParentChanged(CControl *inControl);

        void NotifyParentNeedsLayout();
        void MarkChildrenNeedLayout();
        void MarkNeedsLayout();
        // Tell this control that one of its children need to be layed out.
        void SetLayout(CPt inSize, CPt inPosition);
        void LayoutChildren();
        void EnsureLayout();

    private:
        static std::shared_ptr<CControlData> CreateControlData(CControl &inControl);
        void ReleaseControl(); // set the control to null, remove our graph representation
    };
}
}

#endif
