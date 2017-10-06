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
#ifndef INCLUDED_PROGRESS_VIEW_H
#define INCLUDED_PROGRESS_VIEW_H 1

#pragma once

//==============================================================================
// Includes
//==============================================================================

//==============================================================================
// Forwards
//==============================================================================
class CWndControl;
class CProgressControl;
class CStudioApp;

//=============================================================================
/**
 * Windows view encapsulating the splash screen.
 */
class CProgressView : public CView
{
public:
    /// Used to pass messages from the palette, down to this view
    enum EProgressMessage {
        PROGRESSUPDATE_PERCENT = 0, ///< Inidicates that the percentage displayed should be updated
        PROGRESSUPDATE_FILENAME, ///< Indicates that the file name displayed should be updated
        PROGRESSUPDATE_ACTIONTEXT, ///< Indicates that the text above the file name needs to be
                                   ///updated
    };

    virtual void OnDraw(CDC *inDC);
    virtual LRESULT OnInitializePalettes(WPARAM inwParam, LPARAM inlParam);
    virtual LRESULT OnUpdateProgress(WPARAM inwParam, LPARAM inlParam);

protected:
    CWndControl *m_WndControl;
    CProgressControl *m_ProgressControl;

    CProgressView(); ///< Constructor is protected because you can only create this view dynamically
    DECLARE_DYNCREATE(CProgressView)
    virtual ~CProgressView();
    afx_msg void OnSize(UINT inType, int inX, int inY);
    afx_msg BOOL OnEraseBkgnd(CDC *inDC);
    DECLARE_MESSAGE_MAP()
};

#endif // INCLUDED_PROGRESS_VIEW_H