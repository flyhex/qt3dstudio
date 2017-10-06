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

#if !defined(AFX_MENUITEM_H__6383095D_C084_44EB_A6D0_04CC1DA1B74F__INCLUDED_)
#define AFX_MENUITEM_H__6383095D_C084_44EB_A6D0_04CC1DA1B74F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//==============================================================================
//	CMenuItem Class
//==============================================================================

//==============================================================================
/**
 *	CMenuItem: Object for creating dynamic menus
 */
//==============================================================================
class CMenuItem : public CObject
{
public:
    CMenuItem(CString inName, CString inPath, UINT inID);
    virtual ~CMenuItem();
    UINT GetID() { return m_ID; };
    CString GetPath() { return m_Path; };
    CString GetName() { return m_Name; };
protected:
    CString m_Path; ///< Path to the file
    CString m_Name; ///< Name to use in the menu
    UINT m_ID; ///< ID of this menu item
};

#endif // !defined(AFX_MENUITEM_H__6383095D_C084_44EB_A6D0_04CC1DA1B74F__INCLUDED_)
