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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef INCLUDED_POP_UP_WND
#define INCLUDED_POP_UP_WND 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================

//==============================================================================
/**
 *	CPopupWnd:	Class for making pop-up windows (a replacement for tooltips).
 *
 *	To use, just create a CPopupWnd, set the text that you want displayed, then
 *	call ShowWindow() to toggle the window on and off.  Default color scheme of
 *	this window mimics the system settings for tooltips.
 */
class CPopupWnd : public CWnd
{
public:
    CPopupWnd();

    virtual ~CPopupWnd();

    /// Sets the text to display in this window
    void SetWindowText(LPCTSTR lpszString);

    /// Sets the center point of this window in screen coordinates
    void SetCenterPoint(CPoint inPoint);

    /// Sets the center point of this window from an x,y pair (screen coordinates)
    void SetCenterPoint(int inX, int inY) { SetCenterPoint(CPoint(inX, inY)); }

    /// Retrieves the center point of this window (screen coordinates)
    CPoint GetCenterPoint() { return m_CenterPt; }

    /// Sets the color of the text
    void SetTextColor(COLORREF inColor) { m_TextColor = inColor; }

    /// Sets the background color of the window
    void SetBkColor(COLORREF inColor);

    /// Returns a pointer to the CFont object used to draw the text.  You can make changes to the
    /// font using the pointer that's returned,
    /// just don't delete it.  The default font is set in the constructor as Arial 13.
    CFont *GetFont() { return m_Font; }

    /// Allows you to change multiple properties on this window at once.
    void UpdateToolTip(CPoint inCenter, CString inText, bool inShowWindow);

    ///
    void SetStationary(bool inIsStationary) { m_IsStationary = inIsStationary; }

protected:
    CPoint m_CenterPt; ///< Center of this window in screen coordinates
    CFont *m_Font; ///< Font for the window text
    COLORREF m_TextColor; ///< Color of the text
    COLORREF m_BkColor; ///< Background color of the window
    CBrush *m_BkBrush; ///< Brush used for paiting the background of the window
    bool m_IsStationary; ///< If the window is stationary, then it can only be moved while it is not
                         ///showing

    // Generated message map functions
protected:
    //{{AFX_MSG(CPopupWnd)
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC *pDC);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // INCLUDED_POP_UP_WND
