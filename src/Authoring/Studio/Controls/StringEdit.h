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
#ifndef INCLUDED_STRING_EDIT_H
#define INCLUDED_STRING_EDIT_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================

#include "TextEdit.h"

class CStringEdit : public CTextEdit
{
public:
    CStringEdit();
    virtual ~CStringEdit();
    Q3DStudio::CString GetString() override;
    void SetData(const Q3DStudio::CString &inValue, bool inFireEvent = true) override;
    void OnLoseFocus() override;
    void EnterText(bool inHighlight) override;
    void RefreshDisplayFromData() override;
    bool CanPaste() override;
    virtual void ResetSize();
    void AllowAutoSize(bool inAllow);
    bool GetAllowAutoSize();

    // overload functions
    bool HandleSpecialChar(unsigned int inChar, Qt::KeyboardModifiers inFlags) override;
    void OnGainFocus() override;
    bool OnChar(const QString &inChar, Qt::KeyboardModifiers inFlags) override;

protected:
    Q3DStudio::CString m_Value;
    Q3DStudio::CString m_PreviousValue;
    bool m_AutoSize;

    virtual void RevertText();
    void SetDirty(bool inDirty) override;
};

#endif // INCLUDED_STRING_EDIT_H
