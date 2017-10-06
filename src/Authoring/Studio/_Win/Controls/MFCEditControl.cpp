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
#include "stdafx.h"

//==============================================================================
// Includes
//==============================================================================
#include "MFCEditControl.h"

//==============================================================================
// Implementation
//==============================================================================

#ifdef USE_RICHEDIT
IMPLEMENT_DYNAMIC(CMFCEditControl, CRichEditCtrl)
#else
IMPLEMENT_DYNAMIC(CMFCEditControl, CEdit)
#endif

CMFCEditControl::CMFCEditControl()
    : m_Changed(false)
    , m_HotKeys(nullptr)
{
}

CMFCEditControl::~CMFCEditControl()
{
    // This control's window has been destroyed by PlatformEditControl,
    // so a SigTextCommit is potentially dangerous as it will lead to GetText (below)
    // which will try to get text from a destroyed window.
    // However, if this is needed, note that CEdit works, but CRichEditCtrl don't.
    // Can revert to using CEdit by commenting out the line in MFCEditControl.h

    // if ( m_Changed )
    //{
    //	m_Changed = false;
    //	SigTextCommit( );
    //}
}

Q3DStudio::CString CMFCEditControl::GetText()
{
    Q3DStudio::CString theResult;

#ifdef USE_RICHEDIT
    GETTEXTLENGTHEX getTextLengthEx;
    getTextLengthEx.flags = GTL_DEFAULT | GTL_USECRLF;
    getTextLengthEx.codepage = 1200;
    LRESULT lResult = SendMessage(EM_GETTEXTLENGTHEX, (WPARAM)&getTextLengthEx, 0);
    if (lResult > 0) {
        GETTEXTEX getTextEx;
        getTextEx.cb = (DWORD)(lResult + 2);
        getTextEx.codepage = 1200;
        getTextEx.flags = GT_DEFAULT | GT_USECRLF;
        getTextEx.lpDefaultChar = nullptr;
        getTextEx.lpUsedDefChar = nullptr;
        wchar_t *ws = new wchar_t[lResult / 2 + 1];
        SendMessage(EM_GETTEXTEX, (WPARAM)&getTextEx, (LPARAM)ws);
        theResult = ws;
        delete[] ws;
    }
#else
    ::CString theWindowText;
    GetWindowText(theWindowText);
    // normal gettext, which depends on machine's locale
    theResult = theWindowText;
#endif

    return theResult;
}

void CMFCEditControl::SetText(const Q3DStudio::CString &inText)
{
    // The text manipulation is actually already being handled by the CEdit/CWnd
    // This SetText is called by our custom CPropertyMultilineEdit and screws with the MFC's
    // own text handling functions.
    // Essentially, we still need this to set up the initial use of the MFC Edit.
    if (GetText() == "") {
#ifdef USE_RICHEDIT
        SETTEXTEX setTextEx;
        setTextEx.codepage = 1200;
        setTextEx.flags = ST_DEFAULT;
        SendMessage(EM_SETTEXTEX, (WPARAM)&setTextEx,
                    (LPARAM)(static_cast<const wchar_t *>(inText)));
#else
        // normal settext, which depends on machine's locale
        SetWindowText(inText.GetMulti());
#endif
    }
}

#ifdef USE_RICHEDIT
BEGIN_MESSAGE_MAP(CMFCEditControl, CRichEditCtrl)
#else
BEGIN_MESSAGE_MAP(CMFCEditControl, CEdit)
#endif
ON_CONTROL_REFLECT(EN_CHANGE, OnEnChange)
ON_WM_LBUTTONDBLCLK()
ON_WM_SETFOCUS()
ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

// CMFCEditControl message handlers

void CMFCEditControl::OnEnChange()
{
    m_Changed = true;
    SigTextChanged();
}

void CMFCEditControl::OnLButtonDblClk(UINT inSomething, CPoint inPoint)
{
#ifdef USE_RICHEDIT
    CRichEditCtrl::OnLButtonDblClk(inSomething, inPoint);
#else
    CEdit::OnLButtonDblClk(inSomething, inPoint);
#endif

    // select all text in this CEdit/CRichEditCtrl
    SetSel(0, -1);
}

void CMFCEditControl::OnSetFocus(CWnd *inWnd)
{
#ifdef USE_RICHEDIT
    CRichEditCtrl::OnSetFocus(inWnd);
#else
    CEdit::OnSetFocus(inWnd);
#endif

    m_Changed = false;
}

void CMFCEditControl::OnKillFocus(CWnd *inWnd)
{
#ifdef USE_RICHEDIT
    CRichEditCtrl::OnKillFocus(inWnd);
#else
    CEdit::OnKillFocus(inWnd);
#endif

    if (m_Changed) {
        m_Changed = false;
        SigTextCommit();
    }
}

BOOL CMFCEditControl::PreTranslateMessage(MSG *pMsg)
{
    if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) {
        // Reserve some keys for Studio's hotkeys
        // This is only a subset of Studio's hotkeys as listed in
        // CWorkspace::RegisterGlobalKeyboardShortcuts( CHotKeys* inShortcutHandler )
        // We need to make sure that EditControl's hotkeys won't collide with Studio's hotkeys
        bool theReservedHotKeys = false;
        if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN
            || pMsg->message == WM_KEYUP || pMsg->message == WM_SYSKEYUP) {
            if (pMsg->wParam >= CHotKeys::KEY_F1
                && pMsg->wParam
                    <= CHotKeys::KEY_F12) // F1 to F12 keys, this may fail if not in windows
            {
                theReservedHotKeys = true;
            } else if (CHotKeys::IsKeyDown(VK_CONTROL)) {
                unsigned int theCharacterCode = ::MapVirtualKey(static_cast<UINT>(pMsg->wParam), 2);

                switch (theCharacterCode) {
                case 's':
                case 'S': // save file / save file as
                case 'o':
                case 'O': // open file
                case 'n':
                case 'N': // new file
                    theReservedHotKeys = true;
                }
            }
        }

        if (theReservedHotKeys) {
            if (m_HotKeys != nullptr)
                m_HotKeys->PreTranslateMessage(pMsg);
        } else {
            // no idea what all that filtering does, since Bug 1695 appears to be working
            // and it is causing problem with Japanese text input (2639)
            ::TranslateMessage(pMsg);
            ::DispatchMessage(pMsg);
        }
        return TRUE;

        //// This is REALLY bad, but it's basically filtering all the messages that it thinks it
        ///might want and handles those.
        // if ((::GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0 && (::GetAsyncKeyState(VK_RBUTTON) &
        // 0x8000) == 0)
        //{
        //	unsigned int theChar = pMsg->wParam;

        //	/*
        //	* VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
        //	* 0x40 : unassigned
        //	* VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
        //	*/

        //	/*
        //	#define VK_OEM_1          0xBA   // ';:' for US
        //	#define VK_OEM_PLUS       0xBB   // '+' any country
        //	#define VK_OEM_COMMA      0xBC   // ',' any country
        //	#define VK_OEM_MINUS      0xBD   // '-' any country
        //	#define VK_OEM_PERIOD     0xBE   // '.' any country
        //	#define VK_OEM_2          0xBF   // '/?' for US
        //	#define VK_OEM_3          0xC0   // '`~' for US
        //	#define VK_OEM_4          0xDB  //  '[{' for US
        //	#define VK_OEM_5          0xDC  //  '\|' for US
        //	#define VK_OEM_6          0xDD  //  ']}' for US
        //	#define VK_OEM_7          0xDE  //  ''"' for US
        //	#define VK_OEM_8          0xDF
        //	*/

        //	if ( theChar == VK_RETURN || theChar == VK_BACK || theChar == VK_MENU ||
        //		( theChar >= VK_SPACE && theChar < VK_LWIN ) ||
        //		( theChar >= VK_OEM_1 && theChar <= VK_OEM_8 )
        //		)
        //	{
        //		::TranslateMessage( pMsg );
        //		::DispatchMessage( pMsg );
        //		return TRUE;
        //	}
        //}
    }

#ifdef USE_RICHEDIT
    return CRichEditCtrl::PreTranslateMessage(pMsg);
#else
    return CEdit::PreTranslateMessage(pMsg);
#endif
}

BOOL CMFCEditControl::Create(DWORD dwStyle, const RECT &rect, CWnd *pParentWnd, UINT nID)
{
// TODO: Add your specialized code here and/or call the base class

#ifdef USE_RICHEDIT
    BOOL theResult = CRichEditCtrl::Create(dwStyle, rect, pParentWnd, nID);
    SetEventMask(GetEventMask() | ENM_CHANGE);
#else
    BOOL theResult = CEdit::Create(dwStyle, rect, pParentWnd, nID);
#endif

    return theResult;
}
