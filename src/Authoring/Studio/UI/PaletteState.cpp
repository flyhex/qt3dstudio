/****************************************************************************
**
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

#include "stdafx.h"

#include "PaletteState.h"
#include "Preferences.h"

CPaletteState::CPaletteState(Q3DStudio::CString inWindowName)
    : m_WindowName(inWindowName)
    , m_Maximized(false)
    , m_IsVisible(false)
{
}

CPaletteState::~CPaletteState()
{
}

CPt CPaletteState::GetPosition() const
{
    return m_Position;
}

void CPaletteState::SetPosition(CPt inPosition)
{
    m_Position = inPosition;
}

CPt CPaletteState::GetSize() const
{
    return m_Size;
}

void CPaletteState::SetSize(CPt inSize)
{
    m_Size = inSize;
}

bool CPaletteState::IsVisible() const
{
    return m_IsVisible;
}

void CPaletteState::SetVisible(bool inIsVisible)
{
    m_IsVisible = inIsVisible;
}

void CPaletteState::LoadState()
{
    RestoreDefaults();

    CPreferences thePrefs = CPreferences::GetUserPreferences(m_WindowName);

    m_Maximized = static_cast<bool>(thePrefs.GetValue("maximized", m_Maximized));
    m_Position.x = thePrefs.GetLongValue("positionX", m_Position.x);
    m_Position.y = thePrefs.GetLongValue("positionY", m_Position.y);
    m_Size.x = thePrefs.GetLongValue("sizeX", m_Size.x);
    m_Size.y = thePrefs.GetLongValue("sizeY", m_Size.y);
    m_IsVisible = static_cast<bool>(thePrefs.GetValue("visible", m_IsVisible));
}

void CPaletteState::SaveState() const
{
    CPreferences thePrefs = CPreferences::GetUserPreferences(m_WindowName);

    thePrefs.SetValue("maximized", m_Maximized);
    if (!m_Maximized) {
        thePrefs.SetLongValue("positionX", m_Position.x);
        thePrefs.SetLongValue("positionY", m_Position.y);
        thePrefs.SetLongValue("sizeX", m_Size.x);
        thePrefs.SetLongValue("sizeY", m_Size.y);
    }
    thePrefs.SetValue("visible", m_IsVisible);
}

void CPaletteState::RestoreDefaults()
{
    CPt theDisplaySize = ::GetAvailableDisplaySize();
    CPt theInitialPos = GetDisplaySize();

    if (m_WindowName == "Library") {
        m_Position.x = theInitialPos.x;
        m_Position.y = theInitialPos.y;
        m_Size.x = ::dtol(theDisplaySize.x * .25);
        m_Size.y = ::dtol(theDisplaySize.y * .60);
        m_IsVisible = true;
    } else if (m_WindowName == "Timeline") {
        m_Position.x = ::dtol(theDisplaySize.x * .25) + theInitialPos.x;
        m_Position.y = ::dtol(theDisplaySize.y * .60) + theInitialPos.y;
        m_Size.x = ::dtol(theDisplaySize.x * .50);
        m_Size.y = ::dtol(theDisplaySize.y * .40);
        m_IsVisible = true;
    } else if (m_WindowName == "Storage") {
        m_Position.x = ::dtol(theDisplaySize.x * .75) + theInitialPos.x;
        m_Position.y = ::dtol(theDisplaySize.y * .60) + theInitialPos.y;
        m_Size.x = ::dtol(theDisplaySize.x * .25);
        m_Size.y = ::dtol(theDisplaySize.y * .40);
        m_IsVisible = true;
    } else if (m_WindowName == "MainWindow") {
        // m_Position.x = ::dtol( theDisplaySize.x * .25 ) + theInitialPos.x;
        m_Position.x = theInitialPos.x;
        m_Position.y = theInitialPos.y;
        // m_Size.x = ::dtol( theDisplaySize.x * .50 );
        // m_Size.y = ::dtol( theDisplaySize.y * .60 );
        m_Size.x = ::dtol(theDisplaySize.x * 1.0);
        m_Size.y = ::dtol(theDisplaySize.y * 1.0);
        m_IsVisible = true;
    } else if (m_WindowName == "Inspector") {
        m_Position.x = theInitialPos.x;
        m_Position.y = ::dtol(theDisplaySize.y * .60) + theInitialPos.y;
        m_Size.x = ::dtol(theDisplaySize.x * .25);
        m_Size.y = ::dtol(theDisplaySize.y * .40);
        m_IsVisible = true;
    } else {
        m_Position.x = theInitialPos.x;
        m_Position.y = ::dtol(theDisplaySize.y * .50);
        m_Size.x = ::dtol(theDisplaySize.x * .25);
        m_Size.y = ::dtol(theDisplaySize.y * .50);
        m_IsVisible = true;
    }
}

//=============================================================================
/**
* @return Returns the resolution in pixels of the current primary display.
 */
CPt CPaletteState::GetDisplaySize() const
{
    //	return CPt( 0, 0 );
    CRect theWorkArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &theWorkArea, FALSE);
    return CPt(theWorkArea.left, theWorkArea.top);
}
