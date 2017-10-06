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
#ifndef INCLUDED_CONTEXT_MENU_H
#define INCLUDED_CONTEXT_MENU_H 1

#pragma once

#ifdef WIN32
#define MENUHANDLE HMENU
#endif

//==============================================================================
//	Includes
//==============================================================================
#include "GenericFunctor.h"
#include <vector>

class CContextMenu;

//=============================================================================
/**
 * Class declaration for Context Menu performers.
 */
class CContextMenuPerformer
{
public:
    CContextMenuPerformer()
        : m_SubMenu(nullptr)
    {
    }

    virtual void OnOptionSelected() = 0;

    void SetSubMenu(CContextMenu *inSubMenu) { m_SubMenu = inSubMenu; }
    CContextMenu *GetSubMenu() const { return m_SubMenu; }

protected:
    CContextMenu *m_SubMenu;
};

//=============================================================================
/**
 * Template declaration for Context Menu performers.
 */
template <class TClass>
class CSpecificCContextMenuPerformer : public CContextMenuPerformer
{
public:
    typedef void (TClass::*TFunction)();
    CSpecificCContextMenuPerformer(TClass *inObject, TFunction inFunction)
    {
        m_Object = inObject;
        m_Function = inFunction;
    }

    virtual void OnOptionSelected() { (m_Object->*m_Function)(); }

    TClass *m_Object;
    void (TClass::*m_Function)();
};

//=============================================================================
/**
 * Cross-platform class for creating context menus.
 */
class CContextMenu
{
    typedef std::vector<CContextMenuPerformer *> TPerformerList;
    typedef std::vector<Q3DStudio::CString> TOptionList;

public:
    enum ESelection { NO_SELECTION = -1 };

    CContextMenu();
    virtual ~CContextMenu();

    long DoPopup(CPt inLocation, HWND inParentWindow);
    void AddSeparator();
    void AddOption(int inOptionNameStringID, CContextMenuPerformer *inPerformer,
                   bool inIsEnabled = true);
    void AddOption(const Q3DStudio::CString &inOptionName, CContextMenuPerformer *inPerformer,
                   bool inIsEnabled = true);

    void Clear();
    void SetCheck(long inIndex, bool inChecked);
    void ClearChecked();
    Q3DStudio::CString GetStringAt(long inIndex);
    long GetItemCount();
    long GetSelectedOption() { return m_SelectedOption; }
    /*
            template< class TBase >
            void AddOption( const Q3DStudio::CString& inOptionName, TBase* inClass,
       CSpecificCContextMenuPerformer<TBase>::TFunction inFunction, bool inIsEnabled )
            {
                    AddOption( inOptionName, new CSpecificCContextMenuPerformer<TBase>( inClass,
       inFunction, inIsEnabled ) );
            }
    */

    virtual void Update();

    MENUHANDLE GetMenuHandle() const { return m_Menu; }
    CContextMenuPerformer *GetSelectedMenuPerformer(long inIndex);

    void EnableOptionByIndex(long inIndex, bool inEnabledState = true);
    void EnableOption(const Q3DStudio::CString &inOptionName, bool inEnabledState = true);
    void EnableOption(unsigned int inOptionNameStringID, bool inEnabledState = true);

protected:
    void ProcessSelection(long inIndex);
    void DeletePerformers();

    TPerformerList m_Performers;
    TOptionList m_Options;
    long m_SelectedOption;
    MENUHANDLE m_Menu;
    long m_IndexBase; ///< Base index for command ids associated with this context menu
};

#endif // INCLUDED_CONTEXT_MENU_H