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
#ifndef INCLUDED_PLATFORM_EDIT_CONTROL_H
#define INCLUDED_PLATFORM_EDIT_CONTROL_H 1

#pragma once

//==============================================================================
// Include
//==============================================================================
#include "PlatformWindowControl.h"
#include "MFCEditControl.h"

//==============================================================================
// Class
//==============================================================================

#ifdef WIN32

class CPlatformEditControl : public CPlatformWindowControl
{
protected:
    CFont m_Font; ///< MFC font for the edit text
    CMFCEditControl m_EditWindow; ///< MFC Edit Window

public:
    std::signal0<void> SigTextChanged;
    std::signal0<void> SigTextCommit;

public:
    CPlatformEditControl(UICRenderDevice inParent);
    virtual ~CPlatformEditControl();

    void SetWindowVisible(bool inIsVisible);

    void SetText(const Q3DStudio::CString &inText);
    Q3DStudio::CString GetText();
    void EnableWindow(bool inEnable);

    void SetGlobalKeyboardShortcuts(CHotKeys *inHotKeys)
    {
        m_EditWindow.SetGlobalKeyboardShortcuts(inHotKeys);
    }

    // CControl
    virtual void Draw(CRenderer *inRenderer);

protected:
    void OnTextChanged();
    void OnTextCommit();
};

#endif // WIN32

#endif // INCLUDED_PLATFORM_EDIT_CONTROL_H
