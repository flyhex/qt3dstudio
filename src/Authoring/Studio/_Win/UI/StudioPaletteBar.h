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

#ifndef INCLUDED_STUDIO_PALETTE_BAR_H
#define INCLUDED_STUDIO_PALETTE_BAR_H 1

#pragma once

#include "ViewBar.h"

class CStudioApp;
class CPaletteState;
class CStudioApp;

class CStudioPaletteBar : public CViewBar
{
public:
    CStudioPaletteBar();
    virtual ~CStudioPaletteBar();

    virtual BOOL Create(CString inTitle, CCreateContext *inCreateContext, long inControlId,
                        CWnd *theParent = nullptr);

    void ShowPalette(bool inState = true);
    void InitialUpdate();
    CView *GetView();
    CSize GetHorz() { return m_szHorz; }
    void SetHorz(CSize inSize);
    CSize GetVert() { return m_szVert; }
    void SetVert(CSize inSize);
    CSize GetFloat() { return m_szFloat; }
    void SetFloat(CSize inSize);

protected:
    Q3DStudio::CString m_PaletteName;

    //{{AFX_MSG(CStudioPaletteBar)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

class CStudioDialog : public CMiniFrameWnd
{
public:
    CStudioDialog();
    virtual ~CStudioDialog();

    virtual BOOL Create(CString inTitle, CCreateContext *inCreateContext, CWnd *theParent = nullptr);

    void ShowDialog(bool inState = true);
    void InitialUpdate();
    CView *GetView();

protected:
    Q3DStudio::CString m_DialogName;

    //{{AFX_MSG(CStudioDialog)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // INCLUDED_STUDIO_PALETTE_BAR_H
