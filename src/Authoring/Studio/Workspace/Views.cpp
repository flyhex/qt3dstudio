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

#include "Views.h"
#include "MainFrm.h"

//=============================================================================
/**
 *	Constructor
 */
CViews::CViews(CStudioApp * /*inStudioApp*/)
    : m_mainFrame(nullptr)
{
}

//=============================================================================
/**
 *	Destructor
 */
CViews::~CViews()
{
}

//=============================================================================
/**
 *
 */
void CViews::createViews(bool silent)
{
    // To create the main window, this code creates a new frame window
    // object and then sets it as the application's main window object
    m_mainFrame.reset(new CMainFrame);
    if (!silent)
        m_mainFrame->show();
}

//=============================================================================
/**
 * Register all Application specific shortcut keys.
 * Used to map hotkeys that are active through the whole app, not just one
 * view.
 * @param inHotKeys the handler to register on.
 */
void CViews::registerGlobalKeyboardShortcuts(CHotKeys *inHotKeys, QWidget *actionParent)
{
    if (m_mainFrame)
        m_mainFrame->RegisterGlobalKeyboardShortcuts(inHotKeys, actionParent);
}

CMainFrame *CViews::getMainFrame()
{
    return m_mainFrame.data();
}

//=============================================================================
/**
 *
 */
void CViews::recheckMainframeSizingMode()
{
    if (m_mainFrame != nullptr)
        m_mainFrame->RecheckSizingMode();
}
