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
//	Includes
//==============================================================================

#include "stdafx.h"
#include "MenuEdit.h"
#include "BCMenu.h"
#include "StudioColors.h"

//==============================================================================
//	Message Map
//==============================================================================

BEGIN_MESSAGE_MAP(CMenuEdit, CEdit)
//{{AFX_MSG_MAP(CMenuEdit)
ON_WM_SETFOCUS()
ON_WM_CONTEXTMENU()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//==============================================================================
//	IMPLEMENTATION
//==============================================================================

//==============================================================================
/**
 *		Constructor:	Creates a CMenuEdit
 */
//==============================================================================
CMenuEdit::CMenuEdit()
{
}

//==============================================================================
/**
 *	OnSetFocus:	The message handler for accepting focus.
 *
 *	@param	pOldWnd	The window that had focus last.
 */
//==============================================================================
void CMenuEdit::OnSetFocus(CWnd *pOldWnd)
{
    // Allow the parent to handle the request.
    CEdit::OnSetFocus(pOldWnd);

    // Select the entire text.
    this->SetSel(0, -1);
}

//******************************************************************************
//	DYNAMIC CREATION
//******************************************************************************

IMPLEMENT_DYNAMIC(CMenuEdit, CEdit)

//==============================================================================
/**
 *	OnContextMenu:	WM_CONTEXTMENU handler for the CMenuEdit.
 *
 *	@param	inWnd	This window.
 *	@param	inPoint	Mouse position when the right button was clicked.
 */
//==============================================================================
void CMenuEdit::OnContextMenu(CWnd *inWnd, CPoint inPoint)
{
    Q_UNUSED(inWnd);

    // Added just because Justin has an eye for detail. :)

    BCMenu theMenu;
    BCMenu *thePopupMenu = nullptr;
    long theMenuCmd;

    if (GetFocus() != this)
        this->SetFocus();

    // Load the context menu
    theMenu.LoadMenu(IDR_CONTEXT_EDIT);
#ifdef _USECONTENTMENUIMAGES_
    theMenu.SetBitmapBackground(MENU_IMAGELIST_BACKCOLOR);
    theMenu.LoadToolbar(IDC_EDITCONTEXTTOOLBAR);
#endif

    thePopupMenu = (BCMenu *)theMenu.GetSubMenu(0);

    // Enable/disable menu items
    if (!this->CanUndo())
        thePopupMenu->EnableMenuItem(IDC_UNDO, MF_BYCOMMAND | MF_GRAYED);

    // Disable the Select All if there is no text in the control
    if (this->GetWindowTextLength() == 0)
        thePopupMenu->EnableMenuItem(IDC_SELECTALL, MF_BYCOMMAND | MF_GRAYED);

    // Disable paste if there is no CF_TEXT on the clipboard
    if (!IsClipboardFormatAvailable(CF_TEXT))
        thePopupMenu->EnableMenuItem(IDC_PASTE, MF_BYCOMMAND | MF_GRAYED);

    // Check if any text is selected
    DWORD theSelection = this->GetSel();
    if (LOWORD(theSelection) == HIWORD(theSelection)) {
        // Disable cut, copy and delete commands if no text is selected
        thePopupMenu->EnableMenuItem(IDC_CUT, MF_BYCOMMAND | MF_GRAYED);
        thePopupMenu->EnableMenuItem(IDC_COPY, MF_BYCOMMAND | MF_GRAYED);
        thePopupMenu->EnableMenuItem(IDC_DELETE, MF_BYCOMMAND | MF_GRAYED);
    }

    // Post the menu and handle the command here.
    theMenuCmd = (long)thePopupMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD
                                                        | TPM_NONOTIFY,
                                                    inPoint.x, inPoint.y, this, nullptr);
    switch (theMenuCmd) {
    case IDC_UNDO:
        this->Undo();
        break;

    case IDC_CUT:
        this->Cut();
        break;

    case IDC_COPY:
        this->Copy();
        break;

    case IDC_PASTE:
        this->Paste();
        break;

    case IDC_DELETE:
        // Delete the selected text
        this->ReplaceSel(L"", FALSE);
        break;

    case IDC_SELECTALL:
        // Select the entire text.
        this->SetSel(0, -1);
        break;
    }

    theMenu.DestroyMenu();
}
