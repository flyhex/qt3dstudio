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
#include "MasterControl.h"
#include "Renderer.h"
#include "StudioPreferences.h"
#include "StudioApp.h"
#include "Doc.h"
#include "InspectorControl.h"
#include "ActionControl.h"
#include "GenericTabControl.h"

//===============================================================================
/**
 * Constructor
 * @param inStudioApp
 */
CMasterControl::CMasterControl(IMasterControlProvider *inProvider)
    : m_Provider(inProvider)
    , m_TabMenu(nullptr)
    , m_RenderDevice(nullptr)
{
    // Create the tab control
    m_TabControl = new CGenericTabControl();
    AddChild(m_TabControl);
    m_TabControl->SigTabSelected.connect(std::bind(&CMasterControl::OnTabSelected, this,
                                                   std::placeholders::_1, std::placeholders::_2));

    // Add the drop-down menu
    m_MenuButton = new CProceduralButton<CButtonControl>();
    m_MenuButton->SetUpImage("Storage-Dropdown-Normal.png");
    m_MenuButton->SetDisabledImage("Tab-Menu-Disabled.png");
    m_MenuButton->SetAbsoluteSize(m_MenuButton->GetImageSize());
    m_MenuButton->SetFillColorUp(CStudioPreferences::GetDarkBaseColor());
    m_MenuButton->SetFillColorDown(CStudioPreferences::GetDarkBaseColor());
    m_MenuButton->SetFillColorOver(CStudioPreferences::GetDarkBaseColor());
    m_MenuButton->SetFillColorDisabled(CStudioPreferences::GetDarkBaseColor());
    m_MenuButton->SetFillStyleAll(CProceduralButton<CButtonControl>::EFILLSTYLE_FLOOD);
    CProceduralButton<CButtonControl>::SBorderOptions theBorders(false, false, false, false);
    m_MenuButton->SetBorderVisibilityAll(theBorders);
    QObject::connect(m_MenuButton,&CButtonControl::SigButtonDown,
                     std::bind(&CMasterControl::OnMenuButtonDown, this,
                               std::placeholders::_1) );
    m_TabControl->SetTabBarSibling(m_MenuButton);
}

//===============================================================================
/**
 * Destructor
 */
CMasterControl::~CMasterControl()
{
    RemoveChild(m_TabControl);
}

//===============================================================================
/**
 * Display the context menu for this control
 */
void CMasterControl::DisplayContextMenu(CPt inPosition)
{
    ASSERT(m_Provider != nullptr);
    m_Provider->OnContextMenu(this, inPosition, m_TabMenu);
}

//=============================================================================
/**
 *	Callback when the menu button is selected
 */
void CMasterControl::AddControl(const Q3DStudio::CString &inName, long inType, CControl *inControl)
{
    inControl->SetName(inName);
    m_ControlList.insert(std::make_pair(inControl, inType));
    m_TabControl->AddTab(inName, inControl);
}

//=============================================================================
/**
 *	Callback when the menu button is selected
 */
void CMasterControl::RemoveControl(CControl *inControl)
{
    TControlMap::iterator theIterator = m_ControlList.begin();
    TControlMap::iterator theEndIterator = m_ControlList.end();
    for (; theIterator != theEndIterator; ++theIterator) {
        if (inControl == theIterator->first) {
            m_ControlList.erase(theIterator);
            break;
        }
    }

    if (m_ControlList.size())
        m_TabControl->SelectTab((long)0);

    m_TabControl->RemoveTab(inControl);
}

//=============================================================================
/**
 *	Callback when the menu button is selected
 */
void CMasterControl::SelectControl(long inIndex)
{
    m_TabControl->SelectTab(inIndex);
}

//=============================================================================
/**
 *	Callback when the menu button is selected
 */
void CMasterControl::SelectControl(CControl *inControl)
{
    m_TabControl->SelectTab(inControl);
}

//=============================================================================
/**
 *	Return the number of controls
 */
long CMasterControl::GetControlCount()
{
    return m_TabControl->GetTabCount();
}

//=============================================================================
/**
 *	Return the control for the specified type
 */
CControl *CMasterControl::FindControl(long inType)
{
    CControl *theControl = nullptr;
    TControlMap::iterator theIterator = m_ControlList.begin();
    TControlMap::iterator theEndIterator = m_ControlList.end();
    for (; theIterator != theEndIterator; ++theIterator) {
        if (inType == theIterator->second) {
            theControl = theIterator->first;
            break;
        }
    }
    return theControl;
}

//=============================================================================
/**
 *	Return the active control
 */
CControl *CMasterControl::GetActiveControl()
{
    return m_TabControl->GetSelectedTab();
}

//=============================================================================
/**
 *	Return the index of the active tab
 */
long CMasterControl::GetActiveIndex()
{
    return m_TabControl->GetSelectedTabIndex();
}

//=============================================================================
/**
 *	Return the active type
 */
long CMasterControl::GetActiveType()
{
    return m_ControlList[GetActiveControl()];
}

//=============================================================================
/**
 *	Callback when the menu button is selected
 */
void CMasterControl::OnMenuButtonDown(CControl *)
{
    CPt thePosition(GetSize().x, GetPosition().y + m_MenuButton->GetSize().y);
    DisplayContextMenu(thePosition);
}

//=============================================================================
/**
 *	Callback when a tab changes
 */
void CMasterControl::OnTabSelected(CControl *, CControl *inNewControl)
{
    // Notify the provider that the selection changed
    m_Provider->OnControlSelected(this, inNewControl, m_ControlList[inNewControl]);

    // Set the enabled state of the menu item
    m_MenuButton->SetEnabled((m_TabControl->GetTabCount() != 0));
}

//==============================================================================
//	 CControl
//==============================================================================

Q3DStudio::CString CMasterControl::GetName()
{
    CControl *theActiveControl = GetActiveControl();
    if (theActiveControl)
        return theActiveControl->GetName();
    return L"< Empty >";
}

//=============================================================================
/**
 * Fills the whole control with the base (gray) color, then other controls will
 * draw on top of that.
 * @param inRenderer renderer to draw to
 */
void CMasterControl::Draw(CRenderer *inRenderer)
{
    inRenderer->FillSolidRect(GetSize(), CStudioPreferences::GetDarkBaseColor());
}

//=============================================================================
/**
 *	Set the size of the tree control
 */
void CMasterControl::SetSize(CPt inSize)
{
    CControl::SetSize(inSize);
    m_TabControl->SetSize(inSize);
}

//=============================================================================
/**
 *	Check to see if the right mouse down occurred on the tab control
 */
bool CMasterControl::OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    if (m_TabControl->IsInTabBar(inPoint)) {
        DisplayContextMenu(inPoint);
        return true;
    }

    return CControl::OnMouseRDown(inPoint, inFlags); // not handled
}

//=============================================================================
/**
 *	Return the window handle to the master view
 */
Qt3DSRenderDevice CMasterControl::GetPlatformDevice()
{
    return m_RenderDevice;
}

//=============================================================================
/**
 *	Overriden from CControl. Since this control's parent is the CWnd, ensure
 *	that it gets the keyboard focus if this function is called.
 *	@see CControl::SetFocus
 */
void CMasterControl::GrabFocus(CControl *inControl)
{
    if (m_RenderDevice) {
        ::SetFocus(m_RenderDevice);
    }

    CControl::GrabFocus(inControl);
}
