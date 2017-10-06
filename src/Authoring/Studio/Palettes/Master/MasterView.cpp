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
#include "MasterView.h"
#include "WndControl.h"
#include "StudioApp.h"
#include "MasterControl.h"
#include "IDragable.h"

//==============================================================================
//	Message Maps, etc.
//==============================================================================
IMPLEMENT_DYNCREATE(CMasterView, CView)

BEGIN_MESSAGE_MAP(CMasterView, CView)
//{{AFX_MSG_MAP(CMasterView)
ON_WM_SIZE()
ON_WM_ERASEBKGND()
ON_WM_MOUSEACTIVATE()
//}}AFX_MSG_MAP
ON_MESSAGE(WM_STUDIO_INITIALIZE_PALETTES, OnInitializePalettes)
END_MESSAGE_MAP()

int CMasterView::OnMouseActivate(CWnd *, UINT, UINT)
{
    return MA_NOACTIVATE;
}

//=============================================================================
/**
 * Constructor: Protected because the view is always created dynamically.
 * You must call Initialize() before trying to use this class.
 */
CMasterView::CMasterView()
    : m_WndControl(nullptr)
    , m_Provider(nullptr)
{
}

//=============================================================================
/**
 * Destructor
 */
CMasterView::~CMasterView()
{
    m_WndControl->DestroyWindow();
    m_WndControl = nullptr;

    m_MasterControl = nullptr;
}

//==============================================================================
/**
 *	Handles the WM_INITIALUPDATE message.  Responsible for preparing the view
 *	before it is displayed for the first time.
 */
LRESULT CMasterView::OnInitializePalettes(WPARAM inwParam, LPARAM)
{
    if (!m_WndControl) {
        CStudioApp *theStudioApp = reinterpret_cast<CStudioApp *>(inwParam);

        m_MasterControl = new CMasterControl(m_Provider);
        m_WndControl = new CWndControl(m_MasterControl);

        m_WndControl->RegiserForDnd(this);

        m_WndControl->AddMainFlavor(EUIC_FLAVOR_LISTBOX);
        m_WndControl->AddMainFlavor(EUIC_FLAVOR_FILE);
        m_WndControl->AddMainFlavor(EUIC_FLAVOR_ASSET_UICFILE);
        m_WndControl->AddMainFlavor(EUIC_FLAVOR_ASSET_LIB);
        m_WndControl->AddMainFlavor(EUIC_FLAVOR_ASSET_TL);
        m_WndControl->AddMainFlavor(EUIC_FLAVOR_BASIC_OBJECTS);

        CRect theClientRect;
        GetClientRect(&theClientRect);

        m_WndControl->CreateEx(
            WS_EX_NOPARENTNOTIFY, AfxRegisterWndClass(CS_DBLCLKS, LoadCursor(nullptr, IDC_ARROW),
                                                      (HBRUSH)GetStockObject(BLACK_BRUSH)),
            L"MasterViewWndCtrl", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            theClientRect, this, 100);
        m_MasterControl->SetRenderDevice(m_WndControl->GetSafeHwnd());
    }

    return 0;
}

//=============================================================================
/**
 * Required by base class but does nothing since all drawing is handled by the
 * child control.
 */
void CMasterView::OnDraw(CDC *inDC)
{
    Q_UNUSED(inDC);
}

//=============================================================================
/**
 * Resizes the wnd control to fill the whole view.
 */
void CMasterView::OnSize(UINT inType, int inX, int inY)
{
    CView::OnSize(inType, inX, inY);
    if (::IsWindow(m_WndControl->GetSafeHwnd()))
        m_WndControl->MoveWindow(0, 0, inX, inY);
}

//==============================================================================
/**
 * Tells the Inspector to erase before redrawing.  Overridden because we erasing
 * before each draw produces a flashing effect.
 * @param inDC the DC to erase on.
 * @return FALSE.
 */
BOOL CMasterView::OnEraseBkgnd(CDC *inDC)
{
    Q_UNUSED(inDC);
    return FALSE;
}
