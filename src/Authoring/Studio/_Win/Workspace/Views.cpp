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
#include "Views.h"

//==============================================================================
//	Includes
//==============================================================================
#include "MainFrm.h"

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
void CViews::CreateViews(bool silent)
{
    // To create the main window, this code creates a new frame window
    // object and then sets it as the application's main window object
    m_MainFrame = new CMainFrame();
    if (!silent)
        m_MainFrame->show();
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
void CViews::RegisterGlobalKeyboardShortcuts(CHotKeys *inHotKeys, QWidget *actionParent)
{
    if (m_MainFrame)
        m_MainFrame->RegisterGlobalKeyboardShortcuts(inHotKeys, actionParent);
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
