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

#include "Qt3DSCommonPrecompile.h"
#include "StudioProjectSettings.h"
#include "Core.h"
#include "Preferences.h"
#include "Doc.h"
#include "CoreConst.h"
#include "Dispatch.h"
#include "CommonConstants.h"
#include "StudioPreferences.h"
#include "Qt3DSLargeInteger.h"
#include <QtGui/qcolor.h>

/////////////////////////////////////////////////////////////////////////////
// CStudioProjectSettings property page

//==============================================================================
/**
 *	Constructor: Initializes the object.
 */
CStudioProjectSettings::CStudioProjectSettings(CCore *inCore)
    : m_Core(inCore)
{
    this->Reset();
}

//==============================================================================
/**
 *	Destructor: Releases the object.
 */
//==============================================================================
CStudioProjectSettings::~CStudioProjectSettings()
{
}

//==============================================================================
/**
 *	Reset: Reset internal values
 *
 *	@param	None
 */
//==============================================================================
void CStudioProjectSettings::Reset()
{
    // Reset the internal variables

    m_Author.clear();
    m_Company.clear();

    m_PresentationSize = CStudioPreferences::GetDefaultClientSize();

    // fsaa
    m_FSAAMode = FSAA_OFF;

    m_MaintainAspect = false;

    // do not embed fonts by default
    m_EmbedFonts = false;

    m_RotatePresentation = false;
}

//==============================================================================
/**
 *	SetAuthor: Sets the author value
 *
 *	@param	inAuthor				Author name
 */
//==============================================================================
void CStudioProjectSettings::SetAuthor(const QString &inAuthor)
{
    if (m_Author != inAuthor) {
        m_Author = inAuthor;
        m_Core->GetDoc()->SetModifiedFlag();
    }
}

//==============================================================================
/**
 *	SetCompany: Sets the company value
 *
 *	@param	inCompany				Company name
 */
//==============================================================================
void CStudioProjectSettings::SetCompany(const QString &inCompany)
{
    if (m_Company != inCompany) {
        m_Company = inCompany;
        m_Core->GetDoc()->SetModifiedFlag();
    }
}

//=============================================================================
/**
 * Set the size of the client presentation.
 *
 * @param inSize the size that the presentation should be.
 */
//=============================================================================
void CStudioProjectSettings::SetPresentationSize(CPt inSize)
{
    if (m_PresentationSize != inSize) {
        m_Core->GetDoc()->SetModifiedFlag();
        m_PresentationSize = inSize;
    }
}

//=============================================================================
/**
 * Restore all parameters to what they would be if they were fresh from the installation.
 * Slightly different from a Reset, this will nuke the registry settings for the custom preview
 * options and
 * reset the preview app.
 */
void CStudioProjectSettings::RestoreDefaults()
{
    Reset();
}

//=============================================================================
/**
 * @return true if we are maintaining the aspect ratio for the scene
 */
bool CStudioProjectSettings::GetMaintainAspect()
{
    return m_MaintainAspect;
}

//=============================================================================
/**
 * @param inFlag true if we want to maintain the aspect ratio when changing sizes
 */
void CStudioProjectSettings::SetMaintainAspect(bool inFlag)
{
    if (m_MaintainAspect != inFlag) {
        m_MaintainAspect = inFlag;
        m_Core->GetDoc()->SetModifiedFlag();
    }
}

//=============================================================================
/**
 * @param inEngine the rendering engine we want to use
 */
void CStudioProjectSettings::SetFSAAMode(long inFSAAMode)
{
    m_FSAAMode = inFSAAMode;
    m_Core->GetDoc()->SetModifiedFlag();
}

//=============================================================================
/**
 * @return the flag on whether to embed fonts in am file
 */
bool CStudioProjectSettings::GetEmbedFonts()
{
    return m_EmbedFonts;
}

bool CStudioProjectSettings::GetRotatePresentation()
{
    return m_RotatePresentation;
}

void CStudioProjectSettings::SetRotatePresentation(bool inFlag)
{
    m_RotatePresentation = inFlag;
}

//=============================================================================
/**
 * @param inFlag		the flag on whether to embed fonts in am file
 */
void CStudioProjectSettings::SetEmbedFonts(bool inFlag)
{
    if (m_EmbedFonts != inFlag) {
        m_EmbedFonts = inFlag;
        m_Core->GetDoc()->SetModifiedFlag();
    }
}
