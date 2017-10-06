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

#if !defined(AFX_NUMERICEDIT_H__F08CC602_8CD7_4A27_AC46_48A80A7D8FD0__INCLUDED_)
#define AFX_NUMERICEDIT_H__F08CC602_8CD7_4A27_AC46_48A80A7D8FD0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//==============================================================================
//	Includes
//==============================================================================

#include "MenuEdit.h"

/////////////////////////////////////////////////////////////////////////////
// CNumericEdit window

class CNumericEdit : public CMenuEdit
{
    // Construction
public:
    CNumericEdit();

    // Attributes
public:
    void SetAllowWrap(BOOL inWrapFlag) { m_WrapFlag = inWrapFlag; }

protected:
    long m_RangeLow, m_RangeHigh;
    BOOL m_WrapFlag;

    // Operations
public:
    long GetValue();

    void SetRange(long inRangeLow, long inRangeHigh);
    void SetValue(long inValue);

protected:
    void IncrementValue(long inAmount);
    BOOL IsEditKey(UINT inChar);
    BOOL IsNumeric(UINT inChar);

    void ValidateData();

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CNumericEdit)
    //}}AFX_VIRTUAL

    // Implementation
public:
    virtual ~CNumericEdit();

    // Generated message map functions
protected:
    //{{AFX_MSG(CNumericEdit)
    afx_msg void OnKillFocus();
    afx_msg void OnUpdate();
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NUMERICEDIT_H__F08CC602_8CD7_4A27_AC46_48A80A7D8FD0__INCLUDED_)
