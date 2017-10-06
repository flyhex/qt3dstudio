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
#ifndef INCLUDED_FLOAT_EDIT_H
#define INCLUDED_FLOAT_EDIT_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "TextEdit.h"

#include <QCursor>

GENERIC_FUNCTOR_1(CRevertListener, OnDiscardChanges, CControl *);

//==============================================================================
//	Forwards
//==============================================================================
class CMouseCursor;

class CFloatEdit : public CTextEdit
{
protected:
    float m_Value; ///<
    short m_NumDecimalPlaces; ///<
    short m_FixedPlaces; ///<
    bool m_IsMouseDown; ///<
    bool m_Trapping; ///<
    float m_StartDragVal; ///<
    float m_Min; ///<
    float m_Max; ///<
    bool m_EditMode; ///<
    Q3DStudio::CString m_ValueString; ///<
    CPt m_MouseStartPos; ///< Used to reset the mouse pos when dragging values.
    CRevertListener *m_RevertListener; ///< Used when user presses escape.

public:
    CFloatEdit();
    virtual ~CFloatEdit();

    Q3DStudio::CString GetString() override;
    virtual void SetData(float inValue, bool inFireEvent = true);
    void SetData(const Q3DStudio::CString &inData, bool inFireEvent = true) override;
    bool CanAcceptChar(const Q3DStudio::CString &inCheckString, unsigned int inChar,
                               unsigned int inPosition) override;
    bool CanPaste() override;

    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseWheel(CPt inPoint, long inAmount, Qt::KeyboardModifiers inFlags) override;

    virtual void SetMin(float inMin);
    virtual void SetMax(float inMax);

    void OnLoseFocus() override;
    void OnGainFocus() override;
    bool CanGainFocus() override;

    float GetData();
    float GetDisplayData();

    void SetNumDecimalPlaces(short inNumDecimalPlaces);
    void SetFixedPlaces(short inFixedPlaces);
    void RefreshDisplayFromData() override;

    virtual void SetRevertListener(CRevertListener *inListener) { m_RevertListener = inListener; }

protected:
    bool HandleSpecialChar(unsigned int inChar, Qt::KeyboardModifiers inFlags) override;

    void SetFloatValue(float inValue);
    bool Validate(const Q3DStudio::CString &inString);

    void EnterEditMode();
    void ExitEditMode();

    void FormatString();

    void AddCharNegative();
    void AddCharPeriod();
};

#endif // INCLUDED_FLOAT_EDIT_H
