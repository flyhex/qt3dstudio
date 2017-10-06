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
#include "ProgressView.h"
#include "WndControl.h"
#include "StudioApp.h"
#include "ProgressControl.h"

//==============================================================================
//	Message Maps, etc.
//==============================================================================
IMPLEMENT_DYNCREATE(CProgressView, CView)

BEGIN_MESSAGE_MAP(CProgressView, CView)
//{{AFX_MSG_MAP(CProgressView)
ON_WM_SIZE()
ON_WM_ERASEBKGND()
//}}AFX_MSG_MAP
ON_MESSAGE(WM_STUDIO_INITIALIZE_PALETTES, OnInitializePalettes)
ON_MESSAGE(WM_STUDIO_LOADPROGRESS, OnUpdateProgress)
END_MESSAGE_MAP()

//=============================================================================
/**
 * Constructor: Protected because the view is always created dynamically.
 * You must call Initialize() before trying to use this class.
 */
CProgressView::CProgressView()
    : m_WndControl(nullptr)
    , m_ProgressControl(nullptr)
{
}

//=============================================================================
/**
 * Destructor
 */
CProgressView::~CProgressView()
{
    if (m_WndControl) {
        m_WndControl->DestroyWindow();
        m_WndControl = nullptr;

        SAFE_DELETE(m_ProgressControl);
    }
}

//==============================================================================
/**
 *	Handles the WM_INITIALUPDATE message.  Responsible for preparing the view
 *	before it is displayed for the first time.
 */
LRESULT CProgressView::OnInitializePalettes(WPARAM, LPARAM)
{
    if (!m_WndControl) {
        m_ProgressControl = new CProgressControl;
        m_WndControl = new CWndControl(m_ProgressControl);
        m_ProgressControl->SetName("Progress Control");

        if (!::IsWindow(m_WndControl->m_hWnd))
            m_WndControl->CreateEx(0, AfxRegisterWndClass(CS_DBLCLKS, LoadCursor(nullptr, IDC_WAIT),
                                                          (HBRUSH)GetStockObject(BLACK_BRUSH)),
                                   L"LoadView", WS_CHILD | WS_VISIBLE | WS_MAXIMIZE,
                                   CRect(0, 0, 200, 200), this, 100);
    }

    return 0;
}

//==============================================================================
/**
 *	Handles the WM_STUDIO_LOADPROGRESS message.  Changes text displayed on the
 *	load control, depending on what we are trying to update.
 *	@param inwParam Should be an EProgressMessage to indicate what we are updating
 *	@param inlParam Depends on inwParam.  If the message type is PROGRESSUPDATE_PERCENT,
 *	this parameter contains a long value indicating the percent.  If the message
 *	type is PROGRESSUPDATE_FILENAME, this parameter contains a pointer to an
 *	Q3DStudio::CString, which is the name of the file that we are loading.
 *	@return 0
 */
LRESULT CProgressView::OnUpdateProgress(WPARAM inwParam, LPARAM inlParam)
{
    EProgressMessage theMessageType = static_cast<EProgressMessage>(inwParam);

    switch (theMessageType) {
    // Update the percentage completed
    case PROGRESSUPDATE_PERCENT: {
        long thePercent = (long)inlParam;
        m_ProgressControl->SetProgress(thePercent);
    } break;

    // Update the name of the file being loaded
    case PROGRESSUPDATE_FILENAME: {
        Q3DStudio::CString *theName = reinterpret_cast<Q3DStudio::CString *>(inlParam);
        m_ProgressControl->SetFileName(*theName);
    } break;

    // Update the text above the file name (loading or saving)
    case PROGRESSUPDATE_ACTIONTEXT: {
        Q3DStudio::CString *theText = reinterpret_cast<Q3DStudio::CString *>(inlParam);
        m_ProgressControl->SetActionText(*theText);
    } break;

    // Nothing to do in the default case
    default:
        break;
    }

    return 0;
}

//=============================================================================
/**
 * Required by base class but does nothing since all drawing is handled by the
 * child control.
 */
void CProgressView::OnDraw(CDC *inDC)
{
    Q_UNUSED(inDC);
}

//=============================================================================
/**
 * Resizes the wnd control to fill the whole view.
 */
void CProgressView::OnSize(UINT inType, int inX, int inY)
{
    CView::OnSize(inType, inX, inY);
    if (::IsWindow(m_WndControl->GetSafeHwnd()))
        m_WndControl->MoveWindow(0, 0, inX, inY);
}

//==============================================================================
/**
 * Tells the view to erase before redrawing.  Overridden because erasing
 * before each draw produces a flashing effect.
 * @param inDC the DC to erase on.
 * @return FALSE.
 */
BOOL CProgressView::OnEraseBkgnd(CDC *inDC)
{
    Q_UNUSED(inDC);
    return FALSE;
}
