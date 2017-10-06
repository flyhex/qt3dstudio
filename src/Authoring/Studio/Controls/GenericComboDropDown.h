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
#ifndef INCLUDED_GENERIC_COMBO_DROP_DOWN_H
#define INCLUDED_GENERIC_COMBO_DROP_DOWN_H 1

//==============================================================================
//	Includes
//==============================================================================
#include "GenericEdit.h"
#include "ComboTextBox.h"

#include <QMenu>

//==============================================================================
//	Forwards
//==============================================================================
class CButtonControl;
class CButtonUpListener;

//==============================================================================
/**
 * Combo-box GUI element.  Allows user to choose from a list of strings, and the
 * selected string is displayed in a box.
 */
class CGenericComboDropDown : public CGenericEdit, public IChangeDataListener
{
public:
    CGenericComboDropDown(const bool inUseFloat = false);
    virtual ~CGenericComboDropDown();

    void OnDropDownSelect();
    void OnButtonUp(CControl *inButton);
    bool CanGainFocus() override;
    void AddItem(const Q3DStudio::CString &inString, bool inEnable = true);
    void RemoveAllItems();
    void SelectItem(long inIndex, bool inFireChangeEvent = true);
    long GetSelectedItem();
    long GetItemCount();
    Q3DStudio::CString GetItemText(long inIndex);
    Q3DStudio::CString GetCurrentText();

    virtual long DoDropDownMenu();
    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseWheel(CPt inPoint, long inAmount, Qt::KeyboardModifiers inFlags) override;
    bool OnKeyDown(unsigned int inChar, Qt::KeyboardModifiers inFlags) override;
    void LayoutChildren() override { CGenericEdit::LayoutChildren(); }

    // IChangeDataListener
    void OnChangeData(const Q3DStudio::CString &inOldString,
                      const Q3DStudio::CString &inNewString) override;

protected:
    virtual void ChangeSelection(long inNewSelection);
    virtual void OffsetSelection(long inAmount);
    void SetDisplayText(const Q3DStudio::CString &inString, bool inFireChangeEvent = true);
    void UpdateReadOnly(bool inReadOnlyFlag);

    long m_SelectedItem;
    Q3DStudio::CString m_CurrentText;
    CComboTextBox *m_TextBox; ///< ComboTextBox (StringEdit derivative) that processes text data
    CButtonControl *m_Button;
    QMenu m_DropDownMenu; ///< Context menu used to display a list of options for the control
    QActionGroup m_actionGroup;
    bool m_ReverseOffset; ///< True if we want the mousewheel to scroll the other direction in the
                          ///list;useful for numerical lists like
};

#endif // INCLUDED_GENERIC_COMBO_DROP_DOWN_H
