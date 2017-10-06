/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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
#include "ContextMenu.h"
#include "StringLoader.h"

//=============================================================================
/**
 * Constructor
 */
CContextMenu::CContextMenu()
    : m_SelectedOption(-1)
    , m_IndexBase(0)
{
    m_Menu = ::CreatePopupMenu();
}

//=============================================================================
/**
 * Destructor
 */
CContextMenu::~CContextMenu()
{
    ::DestroyMenu(m_Menu);
}

//=============================================================================
/**
 * Display the popup menu and perform the actions for whatever choice the
 * user makes.
 * @param inLocation the screen coordinates of the location to display this menu.
 * @param inParentWindow the window to attach this menu to.
 */
long CContextMenu::DoPopup(CPt inLocation, HWND inParentWindow)
{
    // Allow derived classes to update the menu state
    Update();

    ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));

    // Display the menu and get the user's input
    unsigned int theCommandID = (unsigned int)::TrackPopupMenuEx(
        m_Menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
        inLocation.x, inLocation.y, inParentWindow, nullptr);

    // If an option was selected then get the performer for that option and execute it.
    m_SelectedOption = theCommandID - 1;
    if (m_SelectedOption >= 0) {
        CContextMenuPerformer *thePerformer = GetSelectedMenuPerformer(m_SelectedOption);
        if (thePerformer != nullptr) {
            thePerformer->OnOptionSelected();
        }
    }

    return m_SelectedOption;
}

//=============================================================================
/**
 * Append a separator to the list of items in this menu.
 */
void CContextMenu::AddSeparator()
{
    ::AppendMenu(m_Menu, MF_SEPARATOR, 0, nullptr);
}

//=============================================================================
/**
 * Append the option onto the end of this menu.
 * @param inOptionNameStringID ID indicating the display name for the option.
 * @param inPerformer the performer to be called if this option is selected.
 * @param inIsEnabled false if the option is to be grayed out and disabled.
 */
void CContextMenu::AddOption(int inOptionNameStringID, CContextMenuPerformer *inPerformer,
                             bool inIsEnabled /* = true */)
{
    Q3DStudio::CString theOptionName(::LoadResourceString(inOptionNameStringID));
    AddOption(theOptionName, inPerformer, inIsEnabled);
}

//=============================================================================
/**
 * Append the option onto the end of this menu.
 * @param inOptionName the display name for the option.
 * @param inPerformer the performer to be called if this option is selected.
 * @param inIsEnabled false if the option is to be grayed out and disabled.
 */
void CContextMenu::AddOption(const Q3DStudio::CString &inOptionName,
                             CContextMenuPerformer *inPerformer, bool inIsEnabled /*= true */)
{
    size_t theIndex = m_IndexBase; // start from a base, so that submenu items can be distinguished

    long theFlags = MF_STRING;
    if (!inIsEnabled)
        theFlags |= MF_GRAYED;

    m_Performers.push_back(inPerformer);
    m_Options.push_back(inOptionName);
    CContextMenu *theSubMenu = (inPerformer ? inPerformer->GetSubMenu() : nullptr);
    if (!theSubMenu)
        theIndex += m_Performers.size();
    else // if performer has a submenu.
    {
        theIndex = (UINT)theSubMenu->GetMenuHandle();
        theFlags |= MF_POPUP;
    }

    ::AppendMenuW(m_Menu, theFlags, theIndex, inOptionName);
}

//=============================================================================
/**
 * Remove all options from the menu.
 */
void CContextMenu::Clear()
{
    long theCount = ::GetMenuItemCount(m_Menu);
    for (long theIndex = theCount - 1; theIndex >= 0; theIndex--)
        ::DeleteMenu(m_Menu, theIndex, MF_BYPOSITION);

    m_Performers.clear();
    m_Options.clear();
}

//=============================================================================
/**
 * Delete all the performers on this object.
 * This is an ease of use method for subclasses if they allocated their
 * performers on the heap. This just deletes all of them and clears the list.
 */
void CContextMenu::DeletePerformers()
{
    TPerformerList::iterator thePos = m_Performers.begin();
    for (; thePos != m_Performers.end(); ++thePos) {
        delete (*thePos);
    }
    m_Performers.clear();
}

//=============================================================================
/**
 * Adds or removes a check mark from the specified item.
 * @param inIndex Index of context menu item to be edited
 * @param inChecked true to set a check, false to remove a check
 */
void CContextMenu::SetCheck(long inIndex, bool inChecked)
{
    long theFlags = MF_BYPOSITION;
    theFlags |= inChecked ? MF_CHECKED : MF_UNCHECKED;

    long theActualIndex = inIndex;
    for (long thePos = 0; thePos < (long)::GetMenuItemCount(m_Menu) && thePos <= inIndex;
         ++thePos) {
        // Account for separators that doesn't "contribute" to the number of performers.
        MENUITEMINFO theMenuInfo;
        ZeroMemory(&theMenuInfo, sizeof(MENUITEMINFO));
        theMenuInfo.cbSize = sizeof(MENUITEMINFO);
        theMenuInfo.fMask = MIIM_TYPE;
        ::GetMenuItemInfo(m_Menu, thePos, TRUE, &theMenuInfo);
        if (theMenuInfo.fType & MFT_SEPARATOR)
            ++theActualIndex;
    }
    ::CheckMenuItem(m_Menu, theActualIndex, theFlags);
}

//=============================================================================
/**
 * Set all the menu items to being unchecked.
 * This will uncheck any items that are currently checked.
 */
void CContextMenu::ClearChecked()
{
    for (long thePos = 0; thePos < (long)::GetMenuItemCount(m_Menu); ++thePos)
        ::CheckMenuItem(m_Menu, thePos, MF_BYPOSITION | MF_UNCHECKED);
}

//=============================================================================
/**
 * Gets the string located at a certain index in the context menu list.  Index
 * is zero-based.
 * @param inIndex zero-based index of the item whose string you want
 * @return the string at the specified index
 */
Q3DStudio::CString CContextMenu::GetStringAt(long inIndex)
{
    Q3DStudio::CString theString;
    if (inIndex >= 0 && inIndex < (long)m_Options.size())
        theString = m_Options.at(inIndex);
    return theString;
}

//=============================================================================
/**
 * @return the number of items in this context menu
 */
long CContextMenu::GetItemCount()
{
    return (long)m_Options.size();
}

//=============================================================================
/**
 *	Override this to set enable state of options before the menu pops up
 */
void CContextMenu::Update()
{
}

static inline long GetEnableMenuItemFlags(bool inEnabledState)
{
    return MF_BYPOSITION | (inEnabledState ? (MF_ENABLED) : (MF_GRAYED));
}

void CContextMenu::EnableOptionByIndex(long inIndex, bool inEnabledState /*= true*/)
{
    if (inIndex >= 0 && inIndex < (long)m_Options.size())
        ::EnableMenuItem(m_Menu, inIndex, GetEnableMenuItemFlags(inEnabledState));
}

void CContextMenu::EnableOption(const Q3DStudio::CString &inOptionName, bool inEnabledState)
{
    long theCount = (long)m_Options.size();
    for (long theIndex = 0; theIndex < theCount; ++theIndex) {
        if (m_Options[theIndex] == inOptionName) {
            ::EnableMenuItem(m_Menu, theIndex, GetEnableMenuItemFlags(inEnabledState));
            break;
        }
    }
}

void CContextMenu::EnableOption(unsigned int inOptionNameStringID, bool inEnabledState)
{
    Q3DStudio::CString theOptionName(::LoadResourceString(inOptionNameStringID));
    EnableOption(theOptionName, inEnabledState);
}

//=============================================================================
/**
 * Find the menu performer associated with this index, recursing down any submenu if necessary.
 * @param inIndex  index of the menu performer
 */
CContextMenuPerformer *CContextMenu::GetSelectedMenuPerformer(long inIndex)
{
    if (inIndex - m_IndexBase < static_cast<long>(m_Performers.size())) {
        m_SelectedOption =
            inIndex - m_IndexBase; // update this for any references to the selected option
        return m_Performers.at(m_SelectedOption);
    }
    // if not found, recurse down submenus
    CContextMenuPerformer *theResult = nullptr;
    TPerformerList::iterator theIter = m_Performers.begin();
    for (; theIter != m_Performers.end() && !theResult; ++theIter) {
        CContextMenu *theSubMenu = (*theIter)->GetSubMenu();
        if (theSubMenu)
            theResult = theSubMenu->GetSelectedMenuPerformer(inIndex);
    }
    return theResult;
}