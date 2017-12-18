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

//==============================================================================
//	Includes
//==============================================================================
#include "NameEdit.h"
#include "INamable.h"

//=============================================================================
/**
 * Constructor
 * @param inPropertyProvider interface that will provide properties by name
 * (such as a CAsset object).  Can be nullptr.
 */
CNameEdit::CNameEdit(INamable *inNamable)
    : m_Namable(inNamable)
    , m_IsSettingData(false)
    , m_LastMoustDown(0, 0)
    , m_LastFocused(true)
    , m_CanEdit(false)
{
    ASSERT(m_Namable); // if you don't have a namable, use CTextEditInPlace instead.

    SetData(m_Namable->GetName(), false);
    AddCommitListener(this);
    AllowAutoSize(true);
}

//=============================================================================
/**
 * Destructor
 */
CNameEdit::~CNameEdit()
{
    RemoveCommitListener(this);
}

//=============================================================================
/**
 * Called when a CTextEdit control's data is changed.  In this case, the name
 * field is being changed, so the new value is pushed down to Client.
 * @param inControl the control that had it's data changed
 */
void CNameEdit::OnSetData(CControl *inControl)
{
    if (!m_IsSettingData && inControl == this && m_Namable) {
        if (m_Namable->GetName() != GetString()) {
            m_IsSettingData = true;
            m_Namable->SetName(GetString());
            m_IsSettingData = false;

            // Refresh what it is in m_Nameable, because the UI data is already changed and the
            // underlying data model may not agree ( ie not accepting empty or renaming isn't
            // allowed )
            // Ideally UI shouldn't change, it should informing the data model through some kind of
            // transaction (like how blackfish does it)
            SetData(m_Namable->GetName(), false);
        }
    }
}

//=============================================================================
/**
 * Called when something on an asset changes, making it dirty.  Resets the name
 * displayed by this control, in case it was the name that changed on the asset.
 */
void CNameEdit::OnDirty()
{
    SetData(m_Namable->GetName());
    RefreshDisplayFromData();
}

bool CNameEdit::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    m_LastMoustDown = inPoint;
    m_CanEdit = !m_LastFocused;

    return CTextEditInPlace::OnMouseDown(inPoint, inFlags);
}

void CNameEdit::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (IsInFocus() && !m_IsInEditMode && !(inFlags & CHotKeys::MOUSE_RBUTTON)) {
        if (m_CanEdit && m_LastMoustDown == inPoint) {
            SetEditMode(true);
            SelectAllText();
        }
    } else {
        CTextEditInPlace::OnMouseUp(inPoint, inFlags);
    }

    m_CanEdit = false;
}

void CNameEdit::OnGainFocus()
{
    CTextEditInPlace::OnGainFocus();
    m_LastFocused = false;
}

void CNameEdit::OnLoseFocus()
{
    m_LastFocused = true;
    CTextEditInPlace::OnLoseFocus();
}
