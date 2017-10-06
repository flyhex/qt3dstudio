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

#if !defined(AFX_CONTROLBUTTON_H__6B61A743_FB7E_11D4_A87A_005004D48D91__INCLUDED_)
#define AFX_CONTROLBUTTON_H__6B61A743_FB7E_11D4_A87A_005004D48D91__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CControlButton window

class CControlButton : public CButton
{
    // Construction
public:
    CControlButton();

    // Attributes
public:
protected:
    HBITMAP m_ImageUp, m_ImageDown, m_ImageDisabled, m_ImageActive, m_ImageMouseTrack,
        m_ImageLimbo; // resource bitmap values for various button states
    BOOL m_DepressFlag, m_3DColorsFlag; // drawing flags
    BOOL m_ActiveFlag; // TRUE if the button is active (ie - selected)
    BOOL m_ActiveDepressedFlag; // TRUE if the active state should be drawn depressed
    CPoint m_ImageOffset; // Image offset from (0,0)
    BOOL m_MouseOverFlag; // TRUE if the mouse is currently over this button
    BOOL m_HotTrackFlag; // TRUE if the mouse should be hot-tracked
    BOOL m_SimpleFrame; // TRUE if the button is a simple frame
    BOOL m_DrawFrame; // TRUE if the frame around the button should be drawn
    BOOL m_LimboEnabled; ///< TRUE if this button is capable of being in limbo (decided at creation
                         ///time)
    BOOL m_LimboFlag; ///< TRUE if the button is currently in limbo state
    COLORREF m_FillColor;
    COLORREF m_TransparentColor;

    // Operations
public:
    void AddImages(COLORREF inTransparentColor, short inImageUp, short inImageDown = 0,
                   short inImageDisabled = 0, short inImageActive = 0, short inImageMouseTrack = 0,
                   short inImageLimbo = 0);

    BOOL Create(DWORD inStyle, const RECT &inRect, CWnd *inParentWnd, short inID,
                BOOL inDepressFlag = TRUE, BOOL inTriState = FALSE);

    BOOL GetActive();

    void SetActive(BOOL inActiveFlag);
    void SetActiveDepressed(BOOL inActiveDepressedFlag);
    /// Sets the "limbo" state of the button.  Usefully for making tri-state buttons.  Limbo state
    /// only works if the limbo flag was set during creation.
    void SetLimbo(BOOL inLimboFlag) { m_LimboFlag = inLimboFlag; }
    /// Returns TRUE if the button is in "limbo" state
    BOOL GetLimbo() { return m_LimboFlag; }
    void SetHotTrack(BOOL inHotTrackFlag);
    void SetImageOffset(CPoint inOffset);
    void SetDrawFrame(BOOL inDrawFrame) { m_DrawFrame = inDrawFrame; }
    /// Use this function to manually set the fill color behind the button.  Use this if you notice
    /// that the wrong color is showing up.
    void SetFillColor(COLORREF inColor) { m_FillColor = inColor; }
protected:
    void DrawButtonFrame(CDC *inDC, CRect *inRect, BOOL inDownFlag);
    void Redraw();
    void TrackMouse(BOOL inTrackEnableFlag = TRUE);
    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CControlButton)
public:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
    //}}AFX_VIRTUAL

    // Implementation
public:
    virtual ~CControlButton();

    // Generated message map functions
protected:
    //{{AFX_MSG(CControlButton)
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg BOOL OnEraseBkgnd(CDC *pDC);
    //}}AFX_MSG
    afx_msg LRESULT OnMouseLeave(WPARAM inwParam, LPARAM inlParam);
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONTROLBUTTON_H__6B61A743_FB7E_11D4_A87A_005004D48D91__INCLUDED_)
