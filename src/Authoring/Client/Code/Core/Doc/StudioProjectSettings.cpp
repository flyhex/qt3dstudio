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
#include "Doc.h"
#include "CoreConst.h"
#include "Dispatch.h"
#include "StudioPreferences.h"
#include "Qt3DSLargeInteger.h"
#include "CColor.h"
#include "Qt3DSColor.h"

/**
 *	Constructor: Initializes the object.
 */
CStudioProjectSettings::CStudioProjectSettings(CCore *inCore)
    : m_core(inCore)
{
    this->reset();
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
void CStudioProjectSettings::reset()
{
    // Reset the internal variables
    m_author.clear();
    m_company.clear();
    m_presentationSize = CStudioPreferences::defaultClientSize();
    m_maintainAspect = false;
    m_rotatePresentation = false;
    m_preferCompressedTextures = false;
}

//==============================================================================
/**
 *	SetAuthor: Sets the author value
 *
 *	@param	inAuthor				Author name
 */
//==============================================================================
void CStudioProjectSettings::setAuthor(const QString &inAuthor)
{
    if (m_author != inAuthor) {
        m_author = inAuthor;
        m_core->GetDoc()->SetModifiedFlag();
    }
}

//==============================================================================
/**
 *	SetCompany: Sets the company value
 *
 *	@param	inCompany				Company name
 */
//==============================================================================
void CStudioProjectSettings::setCompany(const QString &inCompany)
{
    if (m_company != inCompany) {
        m_company = inCompany;
        m_core->GetDoc()->SetModifiedFlag();
    }
}

//=============================================================================
/**
 * Set the size of the client presentation.
 *
 * @param inSize the size that the presentation should be.
 */
//=============================================================================
void CStudioProjectSettings::setPresentationSize(const QSize &inSize)
{
    if (m_presentationSize != inSize) {
        m_core->GetDoc()->SetModifiedFlag();
        m_presentationSize = inSize;
    }
}

//=============================================================================
/**
 * @param inFlag true if we want to maintain the aspect ratio when changing sizes
 */
void CStudioProjectSettings::setMaintainAspect(bool inFlag)
{
    if (m_maintainAspect != inFlag) {
        m_maintainAspect = inFlag;
        m_core->GetDoc()->SetModifiedFlag();
    }
}

void CStudioProjectSettings::setRotatePresentation(bool inFlag)
{
    if (m_rotatePresentation != inFlag) {
        m_rotatePresentation = inFlag;
        m_core->GetDoc()->SetModifiedFlag();
    }
}

void CStudioProjectSettings::setPreferCompressedTextures(bool inFlag)
{
    if (m_preferCompressedTextures != inFlag) {
        m_preferCompressedTextures = inFlag;
        m_core->GetDoc()->SetModifiedFlag();
    }
}
