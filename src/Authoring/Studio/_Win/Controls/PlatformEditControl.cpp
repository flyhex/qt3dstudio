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
#include "Strings.h"

//==============================================================================
// Includes
//==============================================================================
#include "PlatformEditControl.h"
#include "Renderer.h"
#include "StudioPreferences.h"

#ifdef WIN32

CPlatformEditControl::CPlatformEditControl(UICRenderDevice inParent)
    : CPlatformWindowControl(inParent)
{
#ifdef USE_RICHEDIT
    BOOL theReturn = m_EditWindow.Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE
                                             | ES_WANTRETURN | ES_AUTOVSCROLL,
                                         CRect(0, 0, 0, 0), CWnd::FromHandle(inParent), 1);
#else
    BOOL theReturn = m_EditWindow.CreateEx(0, _T("EDIT"), "", WS_CHILD | WS_VSCROLL | ES_MULTILINE
                                               | ES_WANTRETURN | ES_AUTOVSCROLL,
                                           0, 0, 0, 0, inParent, nullptr);
#endif
    ASSERT(theReturn);

    Q_UNUSED(theReturn); // for release builds

    m_EditWindow.SigTextChanged.connect(std::bind(&CPlatformEditControl::OnTextChanged, this));
    m_EditWindow.SigTextCommit.connect(std::bind(&CPlatformEditControl::OnTextCommit, this));

    m_Font.CreatePointFont(
        80, CString(CStudioPreferences::GetFontFaceName())); // size specified in 1/10ths of a point
    m_EditWindow.SetFont(&m_Font);
    m_EditWindow.SetTextMode(TM_PLAINTEXT | TM_MULTILEVELUNDO);
    m_EditWindow.SetBackgroundColor(FALSE, CStudioPreferences::GetLightBaseColor());

    m_Window = m_EditWindow.GetSafeHwnd();
}

CPlatformEditControl::~CPlatformEditControl()
{
    m_EditWindow.DestroyWindow();
}

//=============================================================================
/**
 * Essentially, this just turns the window off.  Do not use the CControl::SetVisible
 * because that would screw with the control's size and you will notice the inspector
 * palette having cut off rows.
 *
 * @param inIsVisible If the window should be visible
 */
void CPlatformEditControl::SetWindowVisible(bool inIsVisible)
{
    if (inIsVisible)
        m_EditWindow.ShowWindow(SW_SHOWNORMAL);
    else
        m_EditWindow.ShowWindow(SW_HIDE);
}

void CPlatformEditControl::OnTextChanged()
{
    SigTextChanged();
}

void CPlatformEditControl::OnTextCommit()
{
    SigTextCommit();
}

void CPlatformEditControl::SetText(const Q3DStudio::CString &inText)
{
    m_EditWindow.SetText(inText);
}

Q3DStudio::CString CPlatformEditControl::GetText()
{
    return m_EditWindow.GetText();
}

void CPlatformEditControl::EnableWindow(bool inEnable)
{
    m_EditWindow.EnableWindow(inEnable);
}

//==============================================================================
// CControl
//==============================================================================

void CPlatformEditControl::Draw(CRenderer *inRenderer)
{
    // Create a clipping region for the MFC Edit
    CRct theUICClipRect = inRenderer->GetClippingRect();
    RECT theRect(theUICClipRect);
    CRgn theClipRgn;
    theClipRgn.CreateRectRgnIndirect(&theRect);
    HRGN theClipHandle = (HRGN)theClipRgn;
    m_EditWindow.SetWindowRgn(theClipHandle, TRUE);

    if (m_EditWindow.IsWindowVisible() == FALSE)
        m_EditWindow.ShowWindow(SW_SHOWNORMAL);

    // Draw a bounding box around the entire control
    CColor theTopColor = CStudioPreferences::GetControlRectTopLineColor();
    CColor theSideColor = CStudioPreferences::GetControlRectSideLineColor();
    CColor theBottomColor = CStudioPreferences::GetControlRectBottomLineColor();

    inRenderer->DrawRectOutline(GetSize(), theTopColor, theSideColor, theBottomColor, theSideColor);
}

#endif // WIN32
