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

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//==============================================================================
//	Includes
//==============================================================================

#include "TextLink.h"
#include "Resource.h"

/////////////////////////////////////////////////////////////////////////////
// CTextLink class

//==============================================================================
/**
 *	Constructor: Initializes the object.
 */
//==============================================================================
CTextLink::CTextLink()
    : CBaseLink()
{
    // Set default colors
    m_ColorText = CStudioPreferences::GetNormalColor();
    m_ColorBackground = CStudioPreferences::GetBaseColor();
}

//==============================================================================
/**
 *	Destructor: Releases the object.
 */
//==============================================================================
CTextLink::~CTextLink()
{
}

//==============================================================================
/**
 *	SetColor: Sets the text color and background color
 *
 *	@param	inText	Text color for the control.
 *	@param	inBackground	Background color for the control.
 */
//==============================================================================
void CTextLink::SetColor(COLORREF inText, COLORREF inBackground)
{
    m_ColorText = inText;
    m_ColorBackground = inBackground;
}

//==============================================================================
/**
 *	ShowLink: Shows the link.
 *
 *	@param	None
 */
//==============================================================================
void CTextLink::ShowLink()
{
    if (IsWindow(m_hWnd)) {
        ShowTextLink(m_ColorText, m_ColorBackground, DT_VCENTER | DT_SINGLELINE, m_bCapture);
    }
}
