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
#ifndef INCLUDED_CONTROL_H
#define INCLUDED_CONTROL_H 1

#pragma once

#include "Pt.h"
#include "Rct.h"
#include "SafeArray.h"
#include "GenericFunctor.h"
#include "PlatformTypes.h"

#include "DropTarget.h"
#include "ControlGraphIterators.h"

class CRenderer;
class CContextMenu;
class Qt3DSFile;
class IDragable;
class CAsset;
class CStudioApp;

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Q3DStudio {
namespace Control {
    class CControlData;
    using std::pair;
    using std::make_pair;
    using std::vector;
}
}

// this functor is used to tell when a child loses focus the boolean should be true if the child
// gains focus
GENERIC_FUNCTOR_1(CChildFocusListener, OnChildFocusChange, bool)

#ifdef _WIN32
typedef HWND TPlatformView;
#else
typedef void *TPlatformView;
#endif

class CControlWindowListener
{
public:
    virtual void OnControlInvalidated() = 0;
    virtual long DoPopup(QMenu *inMenu, CPt inLocation) = 0;
    virtual CPt ClientToScreen(CPt inPoint) = 0;
    virtual CPt ScreenToClient(CPt inPoint) = 0;
    virtual TPlatformView GetPlatformView() = 0;
    virtual void SetIsDragging(bool inIsDragging) = 0;
    virtual void ShowTooltips(CPt inLocation, const QString &inText) = 0;
    virtual void HideTooltips() = 0;
    virtual void DoStartDrag(IDragable *inDragable) = 0;
    virtual void DoStartDrag(std::vector<Q3DStudio::CString> &inDragFileList) = 0;
    virtual void ShowMoveableWindow(CPt inLocation, const Q3DStudio::CString &inText,
                                    CRct inBoundingRct) = 0;
    virtual void HideMoveableWindow() = 0;
};

namespace Q3DStudio {
namespace Control {
    class CControlData;
}
}

class CControl
{

    CControl(CControl &inOther);
    CControl &operator=(CControl &inOther);

public:
    CControl();
    virtual ~CControl();

    DEFINE_OBJECT_COUNTER(CControl)

    CControl *GetParent();
    const CControl *GetParent() const;

    virtual void OnDraw(CRenderer *inRenderer, CRct &inDirtyRect, bool inIgnoreValidation = false);
    virtual void Draw(CRenderer *inRenderer);
    virtual void NotifyNotInClipRect();

    virtual CPt GetPosition() const;
    virtual void SetPosition(CPt inPosition);
    void SetPosition(long inX, long inY);

    virtual CPt GetSize() const;
    virtual void SetSize(CPt inSize);
    virtual void SetSize(long inWidth, long inHeight);
    virtual void OnSizeChanged(CPt inSize);

    virtual CPt GetMinimumSize();
    virtual void SetMinimumSize(CPt inSize);

    virtual CPt GetMaximumSize();
    virtual void SetMaximumSize(CPt inSize);

    virtual CPt GetPreferredSize();
    virtual void SetPreferredSize(CPt inSize);

    virtual void SetAbsoluteSize(CPt inSize);

    virtual void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual void OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual void OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual bool OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual void OnMouseRUp(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual void OnMouseClick(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual bool OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual bool OnMouseHover(CPt inPoint, Qt::KeyboardModifiers inFlags);
    /**
            Note that just overriding this isn't enough, you need to call
       m_ControlData->SetMouseWheelEnabled( true )
            in order to receive mouse wheel events
    */
    virtual bool OnMouseWheel(CPt inPoint, long inAmount, Qt::KeyboardModifiers inFlags);
    virtual bool OnKeyDown(unsigned int inChar, Qt::KeyboardModifiers inFlags);
    virtual bool OnKeyUp(unsigned int inChar, Qt::KeyboardModifiers inFlags);
    virtual bool OnChar(const QString &inChar, Qt::KeyboardModifiers inFlags);
    virtual void OnLoseFocus();
    virtual void OnGainFocus();
    virtual bool CanGainFocus();
    virtual bool IsInFocus();
    void OnTab();
    void OnReverseTab();
    void SetFocusToFirstAvailable();
    void SetFocusToLastAvailable();

    virtual CDropTarget *FindDropCandidate(CPt &inMousePoint, Qt::KeyboardModifiers inFlags,
                                           EStudioObjectType objectType);

    virtual void AddChild(CControl *inControl, CControl *inInsertBefore = NULL);
    virtual void RemoveChild(CControl *inControl);
    virtual void RemoveAllChildren();
    virtual long GetChildIndex(CControl *inChildControl);
    virtual CControl *FindChildByName(const Q3DStudio::CString &inName);
    CControl *FocusedChild();

    virtual bool IsMouseOver() const;

    virtual bool HitTest(const CPt &inPoint) const;
    virtual bool IsInRect(const CRct &inRect) const;

    virtual void Invalidate(bool inInvalidate = true);
    virtual void InvalidateRect(const CRct &inRect);
    virtual bool IsInvalidated() const;

    virtual void OnChildInvalidated();
    virtual bool IsChildInvalidated() const;

    virtual void SetVisible(bool inIsVisible);
    virtual bool IsVisible() const;
    virtual void OnVisibleStateChange(bool inIsVisible);
    virtual void OnParentVisibleStateChanged(bool inIsVisible);

    virtual void SetParentEnabled(bool inParentEnabled);
    virtual void SetEnabled(bool inIsEnabled);
    virtual bool IsEnabled() const;
    bool GetEnabledFlag();
    void SetEnabledFlag(bool inIsEnabled);

    void SetWindowListener(CControlWindowListener *inListener);
    CControlWindowListener *GetWindowListener();

    virtual void OnChildSizeChanged(CControl *inChild);
    virtual void ResetMinMaxPref(){}

    void SetName(const QString &inName);
    QString GetName();

    virtual void BeginDrawChildren(CRenderer *inRenderer);

    virtual long DoPopup(QMenu *inContextMenu, CPt inPoint);
    virtual void RemoveUberControl(CControl *inControl);
    virtual void OffsetPosition(CPt inOffset);

    virtual CPt GetGlobalPosition(CPt inChildPoint) const;
    virtual Qt3DSRenderDevice GetPlatformDevice();
    bool IsChildPlatformDevice(Qt3DSRenderDevice inDevice);
    virtual void ShowMoveableWindow(CPt inLocation, const Q3DStudio::CString &inText,
                                    CRct inBoundingRct);
    virtual void HideMoveableWindow();
    CControl *GetFirstChild();
    bool HasFocus(CControl *inControl);
    virtual void GrabFocus(CControl *inControl);

    virtual CPt ClientToScreen(CPt inPoint);
    virtual CPt ScreenToClient(CPt inPoint);

    void AddFocusListener(CChildFocusListener *inListener);
    void RemoveFocusListener(CChildFocusListener *inListener);
    void FireFocusEvent(bool inStatus);

    void SetTooltipText(const QString &inText);
    QString GetTooltipText();

    virtual void EnsureVisible(CRct inRect);
    void EnsureVisible();

    virtual void OnParentChanged(CControl *inNewParent);

    virtual void MarkChildrenNeedLayout();
    virtual void MarkNeedsLayout();
    // Tell our parent that we (and all our siblings) need to be
    // laid out.
    virtual void NotifyParentNeedsLayout();
    // Tell this control that one of its children need to be layed out.
    virtual void SetLayout(CPt inSize, CPt inPosition);
    virtual void LayoutChildren();
    virtual void EnsureLayout();

    virtual void ChildrenChanged();

    Q3DStudio::Control::ControlGraph::SIterator GetChildren();
    Q3DStudio::Control::ControlGraph::SReverseIterator GetReverseChildren();

protected: ///< Current size of this control
    // Member Functions

    long GetChildCount();
    TPlatformView GetPlatformView();
    bool IsMouseDown();
    void setCursorIfNotSet(long cursor);
    void resetCursor();
    QCursor getCursor() const;

    std::shared_ptr<Q3DStudio::Control::CControlData> m_ControlData;
    long m_cursorSet;
};
#endif // INCLUDED_CONTROL_H
