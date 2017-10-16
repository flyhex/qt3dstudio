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
#include "Views.h"

//==============================================================================
//	Includes
//==============================================================================
#ifdef KDAB_TEMPORARILY_REMOVED
#include "Doc.h"
#endif
#include "MainFrm.h"
#ifdef KDAB_TEMPORARILY_REMOVED
#include "TimelineTimelineLayout.h"
#include "ScalableScroller.h"
#include "PaletteManager.h"
#include "HotKeys.h"
#include "Dispatch.h"
#include "ActionControl.h"
#include "StudioPaletteBar.h"
#endif

//==============================================================================
//	Implementation
//==============================================================================

//=============================================================================
/**
 *	Constructor
 */
CViews::CViews(CStudioApp * /*inStudioApp*/)
    : m_MainFrame(nullptr)
{
}

//=============================================================================
/**
 *	Destructor
 */
CViews::~CViews()
{
    DestroyViews();
}

//=============================================================================
/**
 *
 */
void CViews::CreateViews()
{
    // To create the main window, this code creates a new frame window
    // object and then sets it as the application's main window object
    m_MainFrame = new CMainFrame();
    m_MainFrame->show();
#ifdef KDAB_TEMPORARILY_REMOVED
    // create and load the frame with its resources
    m_MainFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, nullptr, nullptr);

    m_MainFrame->RestoreLayout();
#endif
}

//=============================================================================
/**
 *
 */
void CViews::DestroyViews()
{
    delete m_MainFrame;
    m_MainFrame = nullptr;
}

//==============================================================================
//	Keyboard
//==============================================================================

//=============================================================================
/**
 * Register all Application specific shortcut keys.
 * Used to map hotkeys that are active through the whole app, not just one
 * view.
 * @param inHotKeys the handler to register on.
 */
void CViews::RegisterGlobalKeyboardShortcuts(CHotKeys *inHotKeys)
{
    if (m_MainFrame)
        m_MainFrame->RegisterGlobalKeyboardShortcuts(inHotKeys);
}

//=============================================================================
/**
 *	NO ONE SHOULD USE THIS FUNCTION OTHER THAN THE MAINFRAME ON WINDOWS
 */
CMainFrame *CViews::GetMainFrame()
{
    return m_MainFrame;
}

//=============================================================================
/**
 *
 */
void CViews::RecheckMainframeSizingMode()
{
    if (m_MainFrame != nullptr)
        m_MainFrame->RecheckSizingMode();
}

//==============================================================================
//	Static
//==============================================================================

//=============================================================================
/**
 * Creates a palette and the associated view.
 * @param inClass Should be RUNTIME_CLASS( CCLASS ) where CCLASS is a class that
 * inherits from MFC::CView (must implement dynamic creation) and is the view
 * that you want associated with the palette.
 * @param inPalette Palette that you want the view attached to
 * @param inParent Parent window or nullptr for top level windows
 * @param inWindowTitle Title caption to appear on the title bar of the palette
 * @param inShowWindow true to show the window after creation, false to hide it
 */
#ifdef KDAB_TEMPORARILY_REMOVED
void CViews::CreatePaletteAndView(CRuntimeClass *inClass, CStudioPaletteBar *inPalette,
                                  CFrameWnd *inParent, Q3DStudio::CString inWindowTitle)
{
    CCreateContext theCreateContext;
    memset(&theCreateContext, 0, sizeof(CCreateContext));
    theCreateContext.m_pNewViewClass = inClass;
    theCreateContext.m_pCurrentFrame = inParent;
    theCreateContext.m_pCurrentDoc = nullptr;

    inPalette->Create(::CString(inWindowTitle.GetMulti()), &theCreateContext,
                      CPaletteManager::GetUniquePaletteId(), inParent);
}
#endif

//=============================================================================
/**
 * Creates a dialog and the associated view.
 * @param inClass Should be RUNTIME_CLASS( CCLASS ) where CCLASS is a class that
 * inherits from MFC::CView (must implement dynamic creation) and is the view
 * that you want associated with the palette.
 * @param inPalette Palette that you want the view attached to
 * @param inParent Parent window or nullptr for top level windows
 * @param inWindowTitle Title caption to appear on the title bar of the palette
 * @param inShowWindow true to show the window after creation, false to hide it
 */
#ifdef KDAB_TEMPORARILY_REMOVED
void CViews::CreateDialogAndView(CRuntimeClass *inClass, CStudioDialog *inDialog,
                                 CFrameWnd *inParent, Q3DStudio::CString inWindowTitle)
{
    CCreateContext theCreateContext;
    memset(&theCreateContext, 0, sizeof(CCreateContext));
    theCreateContext.m_pNewViewClass = inClass;
    theCreateContext.m_pCurrentFrame = inParent;
    theCreateContext.m_pCurrentDoc = nullptr;

    inDialog->Create(::CString(inWindowTitle.GetMulti()), &theCreateContext, inParent);
}
#endif
