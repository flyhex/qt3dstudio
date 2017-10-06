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
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "StudioPaletteBar.h"
#include "Preferences.h"
#include "StudioConst.h"
#include "PaletteState.h"
#include "StudioApp.h"

//==============================================================================
// Message Maps
//==============================================================================
BEGIN_MESSAGE_MAP(CStudioPaletteBar, CViewBar)
//{{AFX_MSG_MAP(CStudioPaletteBar)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//==============================================================================
// Constructors/Destructors
//==============================================================================

CStudioPaletteBar::CStudioPaletteBar()
{
}

CStudioPaletteBar::~CStudioPaletteBar()
{
}

void CStudioPaletteBar::SetHorz(CSize inSize)
{
    m_szHorz.cx = max(m_szMinHorz.cx, inSize.cx);
    m_szHorz.cy = max(m_szMinHorz.cy, inSize.cy);
}

void CStudioPaletteBar::SetVert(CSize inSize)
{
    m_szVert.cx = max(m_szMinVert.cx, inSize.cx);
    m_szVert.cy = max(m_szMinVert.cy, inSize.cy);
}

void CStudioPaletteBar::SetFloat(CSize inSize)
{
    m_szFloat.cx = max(m_szMinFloat.cx, inSize.cx);
    m_szFloat.cy = max(m_szMinFloat.cy, inSize.cy);
}

//==============================================================================
/**
 *	Create: A wrapper around the CViewBar::Create() method.
 *
 *	Simplifies the create parameters for the palette bar.
 *
 *	@return	BOOL	The result from the Create() call (FALSE if unsuccessful)
 */
BOOL CStudioPaletteBar::Create(CString inTitle, CCreateContext *inCreateContext, long inControlId,
                               CWnd *theParent /* = nullptr */)
{
    BOOL theReturnValue;

    // Add the title of the bar to the registry location.
    m_PaletteName = inTitle;

    // Create the palette bar.
    theReturnValue =
        CViewBar::Create(theParent, inCreateContext, inTitle, WS_CHILD | CBRS_TOP, inControlId);
    ASSERT(theReturnValue == TRUE);

    SetBarStyle(GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
    EnableDocking(CBRS_ALIGN_ANY);

    return theReturnValue;
}

//==============================================================================
/**
 * Call this function when the palette's view needs to be updated for the first
 * time on application launch.
 */
void CStudioPaletteBar::InitialUpdate()
{
    // Send the update message to the attached CView
    SendMessageToDescendants(WM_STUDIO_INITIALIZE_PALETTES, reinterpret_cast<WPARAM>(&g_StudioApp));
}

//==============================================================================
/**
 *	Shows or hides the palette and saves the visibility.
 *
 *	This function should be called instead of ShowWindow(), so that the visibility
 *	will be saved in the registry.
 */
void CStudioPaletteBar::ShowPalette(bool inState /* = true */)
{
    ShowWindow(inState ? SW_SHOW : SW_HIDE);
    m_pDockSite->ShowControlBar(this, inState, FALSE);
}

//==============================================================================
/**
 *	Query the view associated with this palette
 */
CView *CStudioPaletteBar::GetView()
{
    // Looks like the first grandchild is the view
    return (CView *)GetWindow(GW_CHILD)->GetWindow(GW_CHILD);
}

//==============================================================================
// Message Maps
//==============================================================================
BEGIN_MESSAGE_MAP(CStudioDialog, CMiniFrameWnd)
//{{AFX_MSG_MAP(CStudioDialog)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//==============================================================================
// Constructors/Destructors
//==============================================================================

CStudioDialog::CStudioDialog()
{
}

CStudioDialog::~CStudioDialog()
{
}

//==============================================================================
/**
 *	Create: A wrapper around the CMiniFrameWnd::Create() method.
 *
 *	Simplifies the create parameters for the palette bar.
 *
 *	@return	BOOL	The result from the Create() call (FALSE if unsuccessful)
 */
/*
BOOL CStudioDialog::Create( CString inTitle, CCreateContext* inCreateContext, long inControlId,
CWnd* theParent )
{
        BOOL	theReturnValue;

        // Add the title of the bar to the registry location.
        m_DialogName = inTitle;

        // Create the palette bar.
        theReturnValue = CMiniFrameWnd::Create( theParent, inCreateContext, _T( inTitle ), WS_CHILD
| CBRS_TOP, inControlId );
        ASSERT( theReturnValue == TRUE );

        SetBarStyle( GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC );
        EnableDocking(CBRS_ALIGN_ANY);

        return theReturnValue;
}*/

//==============================================================================
/**
*	Create: A wrapper around the CMiniFrameWnd::Create() method.
*
*	Simplifies the create parameters for the palette bar.
*
*	@return	BOOL	The result from the Create() call (FALSE if unsuccessful)
*/
//==============================================================================
BOOL CStudioDialog::Create(CString inTitle, CCreateContext *inCreateContext,
                           CWnd *theParent /* = nullptr */)
{
    CString theWndClass = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW));
    BOOL theReturnValue;

    // Add the title of the bar to the registry location.
    m_DialogName = inTitle;

    // Create the palette bar.
    theReturnValue = CFrameWnd::Create(
        theWndClass, inTitle, WS_SYSMENU | MFS_SYNCACTIVE | WS_POPUP | WS_CAPTION | MFS_THICKFRAME,
        CRect(300, 300, 550, 800), theParent, nullptr, 0, inCreateContext);

    // Get the view to initialize.
    this->InitialUpdateFrame(inCreateContext->m_pCurrentDoc, FALSE);

    // Get a handle to the system menu on the palette window
    CMenu *theSystemMenu = GetSystemMenu(FALSE);

    // Remove the unwanted system menu items
    theSystemMenu->DeleteMenu(SC_MINIMIZE, MF_BYCOMMAND);
    theSystemMenu->DeleteMenu(SC_MAXIMIZE, MF_BYCOMMAND);
    theSystemMenu->DeleteMenu(SC_RESTORE, MF_BYCOMMAND);

    return theReturnValue;
}

//==============================================================================
/**
 * Call this function when the palette's view needs to be updated for the first
 * time on application launch.
 */
void CStudioDialog::InitialUpdate()
{
    // Send the update message to the attached CView
    SendMessageToDescendants(WM_STUDIO_INITIALIZE_PALETTES, reinterpret_cast<WPARAM>(&g_StudioApp));
}

//==============================================================================
/**
 *	Shows or hides the dialog and saves the visibility.
 *
 *	This function should be called instead of ShowWindow(), so that the visibility
 *	will be saved in the registry.
 */
void CStudioDialog::ShowDialog(bool inState /* = true */)
{
    ShowWindow(inState ? SW_SHOW : SW_HIDE);
    // m_pDockSite->ShowControlBar( this, inState, FALSE );
}

//==============================================================================
/**
 *	Query the view associated with this palette
 */
CView *CStudioDialog::GetView()
{
    // Looks like the first grandchild is the view
    return (CView *)GetWindow(GW_CHILD)->GetWindow(GW_CHILD);
}
