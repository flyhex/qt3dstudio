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
// Prefix
//==============================================================================

#pragma once

//==============================================================================
// Includes
//==============================================================================
#include "HotKeys.h"

//==============================================================================
// Class
//==============================================================================

// RichEditCtrl, though super useful, has some stability issue to be ironed out.
// Use CEdit by commenting out the line below if it is causing crashes.
#define USE_RICHEDIT

#ifdef USE_RICHEDIT
class CMFCEditControl : public CRichEditCtrl
#else
class CMFCEditControl : public CEdit
#endif
{
protected:
    DECLARE_DYNAMIC(CMFCEditControl)

public:
    boost::signal<void()> SigTextChanged;
    boost::signal<void()> SigTextCommit;

protected:
    bool m_Changed;
    CHotKeys *m_HotKeys; // global keyboard shortcut (Studio's hotkeys)

public:
    CMFCEditControl();
    virtual ~CMFCEditControl();

    Q3DStudio::CString GetText();
    void SetText(const Q3DStudio::CString &inText);

    void SetGlobalKeyboardShortcuts(CHotKeys *inHotKeys) { m_HotKeys = inHotKeys; }

protected:
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnEnChange();
    afx_msg void OnLButtonDblClk(UINT, CPoint);
    afx_msg void OnSetFocus(CWnd *);
    afx_msg void OnKillFocus(CWnd *);

    virtual BOOL PreTranslateMessage(MSG *inMessage);
    virtual BOOL Create(DWORD dwStyle, const RECT &rect, CWnd *pParentWnd, UINT nID);
};
