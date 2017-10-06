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
#include "ProgressPalette.h"
#include "StudioConst.h"
#include "ResourceCache.h"
#include "ResImage.h"
#include "ProgressView.h"

//=============================================================================
/**
 * Constructor
 */
CProgressPalette::CProgressPalette()
{
}

//=============================================================================
/**
 * Destructor
 */
CProgressPalette::~CProgressPalette()
{
}

//==============================================================================
/**
 * A wrapper around the CMiniFrameWnd::Create() method.
 * Simplifies the create parameters for the palette bar.  Overridden because
 * the loading screen is different from all the other palettes.
 * @return The result from the Create() call (FALSE if unsuccessful)
 */
BOOL CProgressPalette::Create(CString inTitle, CCreateContext *inCreateContext,
                              CWnd *inParent /* = nullptr */)
{
    CString theWndClass = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_WAIT));
    BOOL theReturnValue = FALSE;

    // Add the title of the bar to the registry location.
    m_DialogName = inTitle;

    // Get the rectangle that we want this splash screen to occupy in the middle of the screen
    CPt theScreenSize = ::GetAvailableDisplaySize();
    CResImage *theImage = CResourceCache::GetInstance()->GetBitmap("progress-screen.png");
    CPt theImageSize = theImage->GetSize();
    long theTitleBarHeight = ::GetSystemMetrics(SM_CYCAPTION);
    long theXPos = theScreenSize.x / 2 - theImageSize.x / 2;
    long theYPos = theScreenSize.y / 2 - theImageSize.y / 2;
    CRect theWindowRect(theXPos, theYPos, theXPos + theImageSize.x,
                        theYPos + theImageSize.y + theTitleBarHeight);

    // Create the dialog.
    theReturnValue = CFrameWnd::Create(theWndClass, inTitle, WS_POPUP | WS_CAPTION, theWindowRect,
                                       inParent, nullptr, 0, inCreateContext);

    return theReturnValue;
}

//==============================================================================
/**
 * Sends a message to the view that the progress bar needs to be updated to a
 * new percentage.
 * @param inPercent New percent complete to be displayed
 */
void CProgressPalette::SetProgress(long inPercent)
{
    SendMessageToDescendants(WM_STUDIO_LOADPROGRESS, CProgressView::PROGRESSUPDATE_PERCENT,
                             static_cast<LPARAM>(inPercent));
    Invalidate();
    UpdateWindow();
}

//==============================================================================
/**
 * Sends a message to the view that the action text (saving/loading) needs to be changed.
 * @param inFileName New file name to be displayed
 */
void CProgressPalette::SetActionText(const Q3DStudio::CString &inText)
{
    SendMessageToDescendants(WM_STUDIO_LOADPROGRESS, CProgressView::PROGRESSUPDATE_ACTIONTEXT,
                             reinterpret_cast<LPARAM>(&inText));
}

//==============================================================================
/**
 * Sends a message to the view that the file name being opened needs to be changed.
 * @param inFileName New file name to be displayed
 */
void CProgressPalette::SetFileName(const Q3DStudio::CString &inFileName)
{
    SendMessageToDescendants(WM_STUDIO_LOADPROGRESS, CProgressView::PROGRESSUPDATE_FILENAME,
                             reinterpret_cast<LPARAM>(&inFileName));
}