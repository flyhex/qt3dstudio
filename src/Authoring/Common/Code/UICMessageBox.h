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
#ifndef INCLUDED_QT3DS_MESSAGE_BOX_H
#define INCLUDED_QT3DS_MESSAGE_BOX_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "UICString.h"

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

#ifndef _WIN32
#define IDOK 1
#define IDCANCEL 2
#define IDYES 6
#define IDNO 7
#define MB_ICONERROR 0x00000010L
#define MB_ICONWARNING 0x00000030L
#define MB_ICONINFORMATION 0x00000040L
#else
#include <WinUser.h>
#endif

//==============================================================================
/**
 * Generic cross-platform class for showing message boxes with an Ok and Cancel
 * button
 */
class CUICMessageBox
{
public:
    /// Return codes from the message box
    enum EMessageBoxReturn { MSGBX_ERROR = 0, MSGBX_OK = IDOK, MSGBX_CANCEL = IDCANCEL };

    /// Icons to be displayed on the message box
    enum EMessageBoxIcon {
        ICON_NONE = 0,
        ICON_ERROR = MB_ICONERROR,
        ICON_WARNING = MB_ICONWARNING,
        ICON_INFO = MB_ICONINFORMATION
    };

    typedef QWidget* TPlatformWindow;

    CUICMessageBox();
    virtual ~CUICMessageBox();
    static EMessageBoxReturn Show(const Q3DStudio::CString &inTitle, const Q3DStudio::CString &inText,
                                  EMessageBoxIcon inIcon, bool inShowCancel = false,
                                  TPlatformWindow inParentWindow = NULL);

protected:
};

#endif // INCLUDED_QT3DS_MESSAGE_BOX_H
