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
#include "SplashPalette.h"
#include "SplashControl.h" // Just for getting the control's size; the view owns the real splash control

//=============================================================================
/**
 * Constructor
 */
CSplashPalette::CSplashPalette()
{
}

//=============================================================================
/**
 * Destructor
 */
CSplashPalette::~CSplashPalette()
{
}

//==============================================================================
/**
 * A wrapper around the CMiniFrameWnd::Create() method.
 * Simplifies the create parameters for the palette bar.  Overridden because
 * the splash screen is different from all the other palettes.
 * @return The result from the Create() call (FALSE if unsuccessful)
 */
BOOL CSplashPalette::Create(CString inTitle, CCreateContext *inCreateContext,
                            CWnd *inParent /* = nullptr */)
{
    CString theWndClass = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW));
    BOOL theReturnValue;

    // Add the title of the bar to the registry location.
    m_DialogName = inTitle;

    // Get the rectangle that we want this splash screen to occupy in the middle of the screen
    CPt theScreenSize = GetAvailableDisplaySize();
    CSplashControl theSplashControl; // Just for getting the control's size; the view owns the real
                                     // splash control
    CPt theImageSize = theSplashControl.GetSize();
    long theXPos = theScreenSize.x / 2 - theImageSize.x / 2;
    long theYPos = theScreenSize.y / 2 - theImageSize.y / 2;
    CRect theWindowRect(theXPos, theYPos, theXPos + theImageSize.x, theYPos + theImageSize.y);

    // Create the dialog.
    DWORD theFlags;
#ifdef DEBUG
    theFlags = WS_EX_TOOLWINDOW;
#else
    theFlags = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
#endif
    // Create the palette bar.
    theReturnValue = CFrameWnd::CreateEx(theFlags, theWndClass, inTitle, WS_POPUP, theWindowRect,
                                         inParent, 0, inCreateContext);

    return theReturnValue;
}
