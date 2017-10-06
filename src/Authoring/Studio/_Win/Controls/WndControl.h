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

#ifndef INCLUDED_WND_CONTROL_H
#define INCLUDED_WND_CONTROL_H 1

#pragma once

#include "Control.h"
#include "PopupWnd.h"
#include "DropContainer.h"

#define WNDCONTROL_CLASSNAME _T("WNDCONTROL")

class CWndControl;
class IDragable;

class CWndControlControlListener : public CControlWindowListener
{
public:
    CWndControlControlListener(CWndControl *inParent);
    virtual void OnControlInvalidated();
    virtual long DoPopup(CContextMenu *inMenu, CPt inLocation);
    virtual CPt ClientToScreen(CPt inPoint);
    virtual CPt ScreenToClient(CPt inPoint);
    virtual TPlatformView GetPlatformView();
    virtual void SetIsDragging(bool inIsDragging);
    virtual void ShowTooltips(CPt inLocation, Q3DStudio::CString inText);
    virtual void HideTooltips();
    virtual void DoStartDrag(IDragable *inDragable);
    virtual void DoStartDrag(std::vector<Q3DStudio::CString> &inDragFileNameList);
    virtual void ShowMoveableWindow(CPt inLocation, Q3DStudio::CString inText, CRct inBoundingRct);
    virtual void HideMoveableWindow();

protected:
    CWndControl *m_Parent;
};

class CWndControl : public CWnd, public CWinDropContainer
{

public:
    CWndControl(CControl *inAgent);
    virtual ~CWndControl();

    DEFINE_OBJECT_COUNTER(CWndControl)

    BOOL RegisterWindowClass();

    virtual void OnControlInvalidated();
    long DoPopup(CContextMenu *inMenu, CPt inLocation);
    void SetIsDragging(bool inIsDragging);
    void DeleteBuffers();
    bool OnKeyDown(UINT inChar, UINT inRepCnt, UINT inFlags);
    bool OnKeyUp(UINT inChar, UINT inRepCnt, UINT inFlags);
    bool OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    void ShowTooltips(CPt inLocation, Q3DStudio::CString inText);
    void HideTooltips();
    void DoStartDrag(IDragable *inDragable);
    void DoStartDrag(std::vector<Q3DStudio::CString> &inDragFileNameList);
    virtual void ShowMoveableWindow(CPt inLocation, Q3DStudio::CString inText, CRct inBoundingRct);
    virtual void HideMoveableWindow();

    bool OnDragWithin(CDropSource &inSource);
    bool OnDragReceive(CDropSource &inSource);
    void OnDragLeave();
    void OnReflectMouse(CPt &inPoint, long inFlags);

    //{{AFX_MSG(CWndControl)
    afx_msg void OnKillFocus(CWnd *pNewWnd);
    afx_msg void OnSetFocus(CWnd *pOldWnd);
    afx_msg void OnPaint();
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnSizing(UINT nFlags, LPRECT pRect);
    afx_msg BOOL OnEraseBkgnd(CDC *pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT OnRegisteredMouseWheel(WPARAM wParam, LPARAM lParam);
    afx_msg int OnMouseActivate(CWnd *pDesktopWnd, UINT nHitTest, UINT message);
    afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);
    afx_msg void OnEnChange();
    //}}AFX_MSG

    afx_msg LRESULT OnMouseLeave(WPARAM inwParam, LPARAM inlParam);
    afx_msg LRESULT OnMouseHover(WPARAM inwParam, LPARAM inlParam);
    DECLARE_MESSAGE_MAP()
protected:
    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLibraryView)
public:
protected:
    virtual BOOL PreTranslateMessage(MSG *pMsg);
    //}}AFX_VIRTUAL

    CControl *m_Control;
    CControl *m_Focus;
    CPopupWnd m_Tooltip;
    CPopupWnd m_MoveableTooltip;
    CBrush m_Brush;
    bool m_MouseOver;
    CDC *m_MemDC;
    HBITMAP m_MemBitmap;
    HBITMAP m_OldBitmap;
    bool m_IsMouseDown; /// This is used for a hack to make controls get a mouse up when we lose
                        /// focuse and
/// the mouse is down... say we try and delete a layer while dragging and it is the last layer

#ifdef _DEBUG
    bool m_IsPainting;
#endif
    bool m_IsDragging;
    CWndControlControlListener m_ControlListener;
};

#endif // INCLUDED_WND_CONTROL_H
