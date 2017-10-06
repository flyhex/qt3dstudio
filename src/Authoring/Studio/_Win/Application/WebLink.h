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

#ifndef _WEBLINK_H_
#define _WEBLINK_H_

#include "BaseLink.h"

/////////////////////////////////////////////////////////////////////////////
// CWebLink window

class CWebLink : public CBaseLink
{
    // Construction
public:
    CWebLink();

    // Attributes
public:
    // Operations
public:
    virtual void SetImage(UINT uiResBmp, bool bAutoSize = true);
    virtual void SetImage(HBITMAP hBmp, bool bAutoSize = true);
    virtual void SetText(CString inText);
    virtual void SetURL(CString inURL);

protected:
    COLORREF m_ColorLink;
    COLORREF m_ColorHighlight;
    COLORREF m_ColorVisited;
    COLORREF m_ColorBackground;

    bool m_bVisited;
    bool m_bPopup;

    HBITMAP m_Bitmap;

    CString m_URL;

    virtual COLORREF GetColor();

    virtual void CopyShortcut();

    virtual void JumpLink();

    virtual void OpenURL(CString inURL);

    virtual void ShowLink();

    virtual void ShowBitmapLink();

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWebLink)
    //}}AFX_VIRTUAL

    // Implementation
public:
    virtual ~CWebLink();

    // Generated message map functions
protected:
    //{{AFX_MSG(CWebLink)
    virtual afx_msg BOOL OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message);
    virtual afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    virtual afx_msg void OnContextMenu(CWnd *pWnd, CPoint pos);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif
