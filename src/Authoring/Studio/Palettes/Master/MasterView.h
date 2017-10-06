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
#ifndef INCLUDED_MASTER_VIEW_H
#define INCLUDED_MASTER_VIEW_H 1

#pragma once
//==============================================================================
// Includes
//==============================================================================

//==============================================================================
// Forwards
//==============================================================================
class CStudioApp;
class CWndControl;
class CMasterControl;
class IMasterControlProvider;

//=============================================================================
/**
 * Windows view encapsulating the inspector palette.
 */
class CMasterView : public CView
{
protected:
    CMasterView();
    DECLARE_DYNCREATE(CMasterView)
    virtual ~CMasterView();

    afx_msg void OnSize(UINT inType, int inX, int inY);
    afx_msg BOOL OnEraseBkgnd(CDC *inDC);
    afx_msg int OnMouseActivate(CWnd *pDesktopWnd, UINT nHitTest, UINT message);
    DECLARE_MESSAGE_MAP()

public:
    virtual void OnDraw(CDC *inDC);
    virtual LRESULT OnInitializePalettes(WPARAM inwParam, LPARAM inlParam);

    void SetProvider(IMasterControlProvider *inProvider) { m_Provider = inProvider; }
    CMasterControl *GetMasterControl() const { return m_MasterControl; }

protected:
    IMasterControlProvider *m_Provider; ///<
    CWndControl *m_WndControl; ///<
    Q3DStudio::CAutoMemPtr<CMasterControl> m_MasterControl; ///<
};

#endif // INCLUDED_MASTER_VIEW_H
