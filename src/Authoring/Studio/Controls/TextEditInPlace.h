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

#ifndef INCLUDED_TEXT_EDIT_IN_PLACE_H
#define INCLUDED_TEXT_EDIT_IN_PLACE_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "StringEdit.h"

//==============================================================================
//	Forwards
//==============================================================================
class CRenderer;

//==============================================================================
/**
 * @class CTextEditInPlace
 */
class CTextEditInPlace : public CStringEdit
{
public:
    CTextEditInPlace();
    virtual ~CTextEditInPlace();

    bool OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseClick(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnLoseFocus() override;
    void OnGainFocus() override;
    bool CanGainFocus() override;
    void Draw(CRenderer *inRenderer) override;
    void SetEditable(bool inIsEditable);
    void SetEditMode(bool inEditMode);
    void EnterText(bool inHighlight) override;
    void Invalidate(bool inInvalidate = true) override;
    bool GetEditMode();
    static long GetRightBuffer();
    bool OnChar(const QString &inChar, Qt::KeyboardModifiers inModifiers) override;
    bool HandleSpecialChar(unsigned int inChar, Qt::KeyboardModifiers inFlags) override;
    virtual void SetSingleClickEdit(bool inAllow);

protected:
    void DoFillBackground(CRenderer *inRenderer) override;

protected:
    bool m_IsInEditMode; ///< If in edit mode, the user is currently editing the control
    bool m_IsEditable; ///< Can only enter edit mode when the control is editable
    bool m_AllowSingleClickEdit; ///< True if clicking on the control while it has focus should
                                 ///cause it to enter edit mode.  False to force a double-click for
                                 ///entering edit mode.

    void RevertText() override;

private:
    static const long s_RightBuffer;
};

#endif // INCLUDED_TEXT_EDIT_IN_PLACE_H
