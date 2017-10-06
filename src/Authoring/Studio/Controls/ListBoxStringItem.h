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
#ifndef INCLUDED_LIST_BOX_STRING_ITEM_H
#define INCLUDED_LIST_BOX_STRING_ITEM_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================

#include "ListBoxItem.h"
#include "StringEdit.h"

//==============================================================================
//	Forwards
//==============================================================================

//==============================================================================
/**
 * CListBoxStringItem contains an immutable StringEdit, extending the base
 * class ListBoxItem to give basic String displaying and storage.
 */
class CListBoxStringItem : public CListBoxItem
{
public:
    CListBoxStringItem();
    virtual ~CListBoxStringItem();

    virtual void SetString(const Q3DStudio::CString &inString);
    virtual Q3DStudio::CString GetString(); ///< returns the string representation of this item

    virtual void SetSelectedState(bool inSelected); ///< Overridden so that we can change the color
                                                    ///of the text based upon selection

protected:
    CStringEdit m_StringEdit;
    CColor m_TextColor;
    CColor m_SelectedTextColor;
};

#endif // INCLUDED_LIST_BOX_STRING_ITEM_H