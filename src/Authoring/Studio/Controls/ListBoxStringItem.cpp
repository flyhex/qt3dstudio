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
//	Include
//==============================================================================
#include "ListBoxStringItem.h"
#include "Renderer.h"
#include "SystemPreferences.h"

//==============================================================================
/**
 * Constructor
 *
 * A ListBoxStringItem contains an immutable StringEdit, extending the base
 * class ListBoxItem to give basic String display and containing funtionality.
 * TODO: Need to implement some way to set the alignment of the text.
 *       Right now, setting the alignment makes the drawing of the control
 *       all dirty. That's why it's taken out, for now.
 *
 * @param inString the String which this ListBoxStringItem is to hold
 */
CListBoxStringItem::CListBoxStringItem()
    : m_TextColor(0, 0, 0)
    , m_SelectedTextColor(CSystemPreferences::GetSelectedTextColor())
{
    m_StringEdit.SetReadOnly(true);
    m_StringEdit.AllowAutoSize(true);
    m_StringEdit.SetAlignment(CTextEdit::LEFT);
    m_StringEdit.SetFillBackground(false);
    m_StringEdit.SetTextColor(m_TextColor);
    AddChild(&m_StringEdit);
}

//==============================================================================
/**
 * Destructor
 */
CListBoxStringItem::~CListBoxStringItem()
{
}

//==============================================================================
/**
 * @return the String that this ListBoxStringItem contains
 */
void CListBoxStringItem::SetString(const Q3DStudio::CString &inString)
{
    m_StringEdit.SetData(inString, false);

    SetMinimumSize(m_StringEdit.GetSize());
    SetSize(m_StringEdit.GetSize());
}

//==============================================================================
/**
 * @return the String that this ListBoxStringItem contains
 */
Q3DStudio::CString CListBoxStringItem::GetString()
{
    return m_StringEdit.GetString();
}

//=============================================================================
/**
 * Overridden so that we can change the color of the text based upon selection.
 * @param inSelected true to select this item, false to deselect it
 */
void CListBoxStringItem::SetSelectedState(bool inSelected)
{
    CListBoxItem::SetSelectedState(inSelected);

    if (inSelected)
        m_StringEdit.SetTextColor(m_SelectedTextColor);
    else
        m_StringEdit.SetTextColor(m_TextColor);
}
