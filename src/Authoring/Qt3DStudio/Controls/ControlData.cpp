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
#include "Qt3DSCommonPrecompile.h"
#include "ControlData.h"
#include "ControlGraph.h"
#include "Control.h"

#include <QtWidgets/qmenu.h>

using namespace Q3DStudio::Control;
using Q3DStudio::CString;

CControlData::CControlData(CControl &inControl)
    : m_Control(&inControl)
    , m_MinSize(0, 0)
    , m_MaxSize(LONG_MAX, LONG_MAX)
    , m_PrefSize(10, 10)
    , m_IsMouseOver(false)
    , m_IsInvalidated(true)
    , m_IsChildInvalidated(false)
    , m_IsVisible(true)
    , m_IsEnabled(true)
    , m_IsParentEnabled(true)
    , m_HasFocus(false)
    , m_IsMouseDown(false)
    , m_ShowTooltips(false)
    , m_NeedsLayout(true)
    , m_ChildrenNeedLayout(true)
    , m_WindowListener(NULL)
{
}

CControlData::~CControlData()
{
}

void CControlData::ReleaseControl()
{
    if (m_Control) {
        ControlGraph::RemoveNode(*m_Control);
        m_Control = nullptr;
    }
}

std::shared_ptr<CControlData> CControlData::CreateControlData(CControl &inControl)
{
    std::shared_ptr<CControlData> retval =
        std::make_shared<CControlData>(std::ref(inControl));
    ControlGraph::AddNode(retval);
    return retval;
}

CControl *CControlData::GetControl()
{
    return m_Control;
}

CControl *CControlData::GetParent()
{

    if (m_Control) {
        std::shared_ptr<CControlData> theParent(ControlGraph::GetParent(*m_Control));
        if (theParent)
            return theParent->GetControl();
    }
    return nullptr;
}

void CControlData::OnDraw(CRenderer *inRenderer, CRct &inDirtyRect, bool inIgnoreValidation)
{
    if (m_Control)
        m_Control->OnDraw(inRenderer, inDirtyRect, inIgnoreValidation);
}

void CControlData::Draw(CRenderer *inRenderer)
{
    if (m_Control)
        m_Control->Draw(inRenderer);
}

void CControlData::NotifyNotInClipRect()
{
    if (m_Control)
        m_Control->NotifyNotInClipRect();
}

CPt CControlData::GetPosition() const
{
    if (m_Control)
        return m_Control->GetPosition();
    return CPt();
}

void CControlData::SetPosition(CPt inPosition)
{
    if (m_Control)
        m_Control->SetPosition(inPosition);
}

void CControlData::SetPosition(long inX, long inY)
{
    SetPosition(CPt(inX, inY));
}

CPt CControlData::GetSize() const
{
    if (m_Control)
        return m_Control->GetSize();
    return CPt();
}

void CControlData::SetSize(CPt inSize)
{
    if (m_Control)
        m_Control->SetSize(inSize);
}

void CControlData::SetSize(long inWidth, long inHeight)
{
    SetSize(CPt(inWidth, inHeight));
}

CPt CControlData::GetMinimumSize()
{
    if (m_Control)
        return m_Control->GetMinimumSize();
    return CPt();
}

void CControlData::SetMinimumSize(CPt inSize)
{
    if (m_Control)
        m_Control->SetMinimumSize(inSize);
}

CPt CControlData::GetMaximumSize()
{
    if (m_Control)
        return m_Control->GetMaximumSize();
    return CPt();
}

void CControlData::SetMaximumSize(CPt inSize)
{
    if (m_Control)
        m_Control->SetMaximumSize(inSize);
}

CPt CControlData::GetPreferredSize()
{
    if (m_Control)
        return m_Control->GetPreferredSize();
    return CPt();
}

void CControlData::SetPreferredSize(CPt inSize)
{
    if (m_Control)
        return m_Control->SetPreferredSize(inSize);
}

void CControlData::SetAbsoluteSize(CPt inSize)
{
    if (m_Control)
        m_Control->SetAbsoluteSize(inSize);
}

void CControlData::SetMouseDown(bool inMouseDown)
{
    m_IsMouseDown = inMouseDown;
}

void CControlData::OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        m_Control->OnMouseMove(inPoint, inFlags);
}

void CControlData::OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        m_Control->OnMouseOver(inPoint, inFlags);
}

void CControlData::OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        m_Control->OnMouseOut(inPoint, inFlags);
}

bool CControlData::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        return m_Control->OnMouseDown(inPoint, inFlags);
    return false;
}

bool CControlData::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        return m_Control->OnMouseRDown(inPoint, inFlags);
    return false;
}

void CControlData::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        m_Control->OnMouseUp(inPoint, inFlags);
}

void CControlData::OnMouseRUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        m_Control->OnMouseRUp(inPoint, inFlags);
}

void CControlData::OnMouseClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        m_Control->OnMouseClick(inPoint, inFlags);
}

bool CControlData::OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        return m_Control->OnMouseDoubleClick(inPoint, inFlags);
    return false;
}

bool CControlData::OnMouseHover(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        return m_Control->OnMouseHover(inPoint, inFlags);
    return false;
}

bool CControlData::OnMouseWheel(CPt inPoint, long inAmount, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        return m_Control->OnMouseWheel(inPoint, inAmount, inFlags);
    return false;
}

bool CControlData::OnKeyDown(unsigned int inChar, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        return m_Control->OnKeyDown(inChar, inFlags);
    return false;
}

bool CControlData::OnKeyUp(unsigned int inChar, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        return m_Control->OnKeyUp(inChar, inFlags);
    return false;
}

bool CControlData::OnChar(const QString &inChar, Qt::KeyboardModifiers inFlags)
{
    if (m_Control)
        return m_Control->OnChar(inChar, inFlags);
    return false;
}

void CControlData::OnLoseFocus()
{
    if (m_Control)
        m_Control->OnLoseFocus();
}

void CControlData::OnGainFocus()
{
    if (m_Control)
        m_Control->OnGainFocus();
}

bool CControlData::CanGainFocus()
{
    if (m_Control)
        return m_Control->CanGainFocus();
    return false;
}

bool CControlData::IsInFocus()
{
    if (m_Control)
        return m_Control->IsInFocus();
    return false;
}

void CControlData::OnTab()
{
    if (m_Control)
        m_Control->OnTab();
}

void CControlData::OnReverseTab()
{
    if (m_Control)
        m_Control->OnReverseTab();
}

void CControlData::SetFocusToFirstAvailable()
{
    if (m_Control)
        m_Control->SetFocusToFirstAvailable();
}

void CControlData::SetFocusToLastAvailable()
{
    if (m_Control)
        m_Control->SetFocusToLastAvailable();
}

void CControlData::ChildrenChanged()
{
    if (m_Control)
        m_Control->ChildrenChanged();
}

void CControlData::SetMouseWheelEnabled(bool inEnabled)
{
    if (m_Control == nullptr)
        return;

    if (m_MouseWheelState.m_Enabled != inEnabled) {
        m_MouseWheelState.m_Enabled = inEnabled;
        ControlEventState::Enum theNewState;
        if (m_MouseWheelState.m_Enabled == false)
            theNewState = ControlEventState::Unknown;
        else
            theNewState = ControlEventState::Listening;
        SetMouseWheelEventState(theNewState);
    }
}

void CControlData::OnHierarchyChanged()
{
    ControlEventState::Enum theNewState;
    if (m_MouseWheelState.m_Enabled)
        theNewState = ControlEventState::Listening;
    else
        theNewState = ControlEventState::Unknown;
    SetMouseWheelEventState(theNewState);
}

/**
        Look at hierarchy information to figure out which events should
        trickle down the tree to here (or deeper).  Recursive call that
        may take some time to complete.
*/
void CControlData::UpdateMouseWheelEventState()
{
    if (m_Control == nullptr)
        return;
    ControlEventState::Enum theNewState(ControlEventState::Ignoring);
    if (m_MouseWheelState.m_Enabled == true)
        theNewState = ControlEventState::Listening;
    else if (m_MouseWheelState.m_EventState == ControlEventState::Unknown && m_Control != nullptr) {
        for (ControlGraph::SIterator theIter(ControlGraph::GetChildren(*m_Control));
             theIter.IsDone() == false; ++theIter) {
            if (theIter->GetMouseWheelEventState() == ControlEventState::Listening) {
                theNewState = ControlEventState::Listening;
                break;
            }
        }
    }
    SetMouseWheelEventState(theNewState);
}

/**
        Update our mouse wheel state and notify our parent if necessary.  Note that we
        can't set our event state to ignoring without checking all of our children
        so the result of this function is that our event state is either listening
        or unknown.
*/
void CControlData::SetMouseWheelEventState(ControlEventState::Enum inNewState)
{
    if (m_MouseWheelState.m_EventState != inNewState) {
        m_MouseWheelState.m_EventState = inNewState;
        std::shared_ptr<CControlData> theParent = ControlGraph::GetParent(*m_Control);
        if (theParent)
            theParent->ChildMouseWheelEventStateChanged(m_MouseWheelState.m_EventState);
    }
}

/**
        When a given child notifies us that its event state has changed then we can update
        our state.  We can't set the state to ignoring unless we *know* all of our
        children are ignoring mouse wheel.
 */
void CControlData::ChildMouseWheelEventStateChanged(ControlEventState::Enum inNewState)
{
    ControlEventState::Enum theNewState;
    if (inNewState == ControlEventState::Listening || m_MouseWheelState.m_Enabled == true)
        theNewState = ControlEventState::Listening;
    else
        theNewState = ControlEventState::Unknown;

    SetMouseWheelEventState(theNewState);
}

ControlEventState::Enum CControlData::GetMouseWheelEventState()
{
    if (m_MouseWheelState.m_EventState == ControlEventState::Unknown)
        UpdateMouseWheelEventState();
    assert(m_MouseWheelState.m_EventState != ControlEventState::Unknown);
    return m_MouseWheelState.m_EventState;
}

CDropTarget *CControlData::FindDropCandidate(CPt &inMousePoint, Qt::KeyboardModifiers inFlags,
                                             EStudioObjectType objectType,
                                             Q3DStudio::DocumentEditorFileType::Enum fileType)
{
    if (m_Control)
        return m_Control->FindDropCandidate(inMousePoint, inFlags, objectType, fileType);
    return nullptr;
}

void CControlData::AddChild(CControl *inControl, CControl *inInsertBefore)
{
    if (m_Control)
        m_Control->AddChild(inControl, inInsertBefore);
}

void CControlData::RemoveChild(CControl *inControl)
{
    if (m_Control)
        m_Control->RemoveChild(inControl);
}

void CControlData::RemoveAllChildren()
{
    if (m_Control)
        m_Control->RemoveAllChildren();
}

long CControlData::GetChildIndex(CControl *inChildControl)
{
    if (m_Control)
        return m_Control->GetChildIndex(inChildControl);
    return 0;
}

CControl *CControlData::FindChildByName(const Q3DStudio::CString &inName)
{
    if (m_Control)
        m_Control->FindChildByName(inName);
    return nullptr;
}

bool CControlData::IsMouseOver() const
{
    if (m_Control)
        return m_Control->IsMouseOver();
    return false;
}

bool CControlData::HitTest(const CPt &inPoint) const
{
    if (m_Control)
        return m_Control->HitTest(inPoint);
    return false;
}
bool CControlData::IsInRect(const CRct &inRect) const
{
    if (m_Control)
        return m_Control->IsInRect(inRect);
    return false;
}

void CControlData::Invalidate(bool inInvalidate)
{
    if (m_Control)
        m_Control->Invalidate(inInvalidate);
}

void CControlData::InvalidateRect(const CRct &inRect)
{
    if (m_Control)
        m_Control->InvalidateRect(inRect);
}

bool CControlData::IsInvalidated() const
{
    return m_IsInvalidated;
}

void CControlData::OnChildInvalidated()
{
    if (m_Control)
        m_Control->OnChildInvalidated();
}

bool CControlData::IsChildInvalidated() const
{
    if (m_Control)
        return m_Control->IsChildInvalidated();
    return false;
}

void CControlData::SetVisible(bool inIsVisible)
{
    if (m_Control)
        m_Control->SetVisible(inIsVisible);
}

bool CControlData::IsVisible() const
{
    if (m_Control)
        return m_Control->IsVisible();
    return false;
}

void CControlData::OnVisibleStateChange(bool inIsVisible)
{
    if (m_Control)
        m_Control->OnVisibleStateChange(inIsVisible);
}

void CControlData::OnParentVisibleStateChanged(bool inIsVisible)
{
    if (m_Control)
        m_Control->OnParentVisibleStateChanged(inIsVisible);
}

void CControlData::SetParentEnabled(bool inParentEnabled)
{
    if (m_Control)
        m_Control->SetParentEnabled(inParentEnabled);
}

void CControlData::SetEnabled(bool inIsEnabled)
{
    if (m_Control)
        m_Control->SetEnabled(inIsEnabled);
}

bool CControlData::IsEnabled() const
{
    return m_IsEnabled && m_IsParentEnabled;
}

bool CControlData::GetEnabledFlag()
{
    return m_IsEnabled;
}

void CControlData::SetEnabledFlag(bool inIsEnabled)
{
    m_IsEnabled = inIsEnabled;
}

void CControlData::OnChildSizeChanged(CControl *inChild)
{
    if (m_Control)
        m_Control->OnChildSizeChanged(inChild);
}

void CControlData::ResetMinMaxPref()
{
    if (m_Control)
        m_Control->ResetMinMaxPref();
}

void CControlData::SetName(CString inName)
{
    m_ControlName = inName;
}

CString CControlData::GetName()
{
    return m_ControlName;
}

void CControlData::BeginDrawChildren(CRenderer *inRenderer)
{
    if (m_Control)
        m_Control->BeginDrawChildren(inRenderer);
}

long CControlData::DoPopup(QMenu *inContextMenu, CPt inPoint)
{
    if (m_Control)
        return m_Control->DoPopup(inContextMenu, inPoint);
    return 0;
}

void CControlData::RemoveUberControl(CControl *inControl)
{
    if (m_Control)
        m_Control->RemoveUberControl(inControl);
}

void CControlData::OffsetPosition(CPt inOffset)
{
    if (m_Control)
        m_Control->OffsetPosition(inOffset);
}

CPt CControlData::GetGlobalPosition(CPt inChildPoint) const
{
    if (m_Control)
        return m_Control->GetGlobalPosition(inChildPoint);
    return CPt();
}

Qt3DSRenderDevice CControlData::GetPlatformDevice()
{
    if (m_Control)
        return m_Control->GetPlatformDevice();
    return nullptr;
}

bool CControlData::IsChildPlatformDevice(Qt3DSRenderDevice inDevice)
{
    if (m_Control)
        return m_Control->IsChildPlatformDevice(inDevice);
    return false;
}

void CControlData::ShowMoveableWindow(CPt inLocation, Q3DStudio::CString inText, CRct inBoundingRct)
{
    if (m_Control)
        m_Control->ShowMoveableWindow(inLocation, inText, inBoundingRct);
}

void CControlData::HideMoveableWindow()
{
    if (m_Control)
        m_Control->HideMoveableWindow();
}

CControl *CControlData::GetFirstChild()
{
    if (m_Control)
        m_Control->GetFirstChild();
    return nullptr;
}

bool CControlData::HasFocus(CControl *inControl)
{
    if (m_Control)
        return m_Control->HasFocus(inControl);
    return false;
}

void CControlData::GrabFocus(CControl *inControl)
{
    if (m_Control)
        m_Control->GrabFocus(inControl);
}

CPt CControlData::ClientToScreen(CPt inPoint)
{
    if (m_Control)
        return m_Control->ClientToScreen(inPoint);
    return CPt();
}

CPt CControlData::ScreenToClient(CPt inPoint)
{
    if (m_Control)
        return m_Control->ScreenToClient(inPoint);
    return CPt();
}

void CControlData::AddFocusListener(CChildFocusListener *inListener)
{
    if (m_Control)
        m_Control->AddFocusListener(inListener);
}

void CControlData::RemoveFocusListener(CChildFocusListener *inListener)
{
    if (m_Control)
        m_Control->RemoveFocusListener(inListener);
}

void CControlData::FireFocusEvent(bool inStatus)
{
    if (m_Control)
        m_Control->FireFocusEvent(inStatus);
}

void CControlData::EnsureVisible(CRct inRect)
{
    if (m_Control)
        m_Control->EnsureVisible(inRect);
}

void CControlData::EnsureVisible()
{
    if (m_Control)
        m_Control->EnsureVisible();
}

void CControlData::OnParentChanged(CControl *inControl)
{
    if (m_Control)
        m_Control->OnParentChanged(inControl);
}

void CControlData::NotifyParentNeedsLayout()
{
    if (m_Control)
        m_Control->NotifyParentNeedsLayout();
}

void CControlData::MarkChildrenNeedLayout()
{
    if (m_Control)
        m_Control->MarkChildrenNeedLayout();
}

void CControlData::MarkNeedsLayout()
{
    if (m_Control)
        m_Control->MarkNeedsLayout();
}

// Tell this control that one of its children need to be layed out.
void CControlData::SetLayout(CPt inSize, CPt inPosition)
{
    if (m_Control)
        m_Control->SetLayout(inSize, inPosition);
}

void CControlData::LayoutChildren()
{
    if (m_Control)
        m_Control->LayoutChildren();
}

void CControlData::EnsureLayout()
{
    if (m_Control)
        m_Control->EnsureLayout();
}
