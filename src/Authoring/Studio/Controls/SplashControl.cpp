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
// Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
// Includes
//==============================================================================
#include "SplashControl.h"
#include "Renderer.h"
#include "ResourceCache.h"
#include "StudioPreferences.h"
#include "OffscreenRenderer.h"
#include "StudioDefs.h"

//=============================================================================
/**
 * Constructor
 */
CSplashControl::CSplashControl()
{
    // Image
    m_Image = CResourceCache::GetInstance()->GetBitmap("obsolete_placeholder.png");
    SetAbsoluteSize(m_Image.rect().bottomRight());

    // First line of the copyright statement
    m_CopyrightLine1 = QObject::tr("Copyright %1 The Qt Company. All rights reserved.").arg(
                QString(STUDIO_COPYRIGHT_YEAR));

    // Second line of the copyright statement
    m_CopyrightLine2 = QObject::tr("Qt and Qt Logo are trademarks of The Qt Company");

    // Version text
    m_VersionInfo = QStringLiteral("Qt 3D Studio v")
            + CStudioPreferences::GetVersionString().toQString();

    // Calculate the number of pixels between each line of text
    COffscreenRenderer theOffscreenRenderer(CRct(0, 0, 1, 1));
    auto size = theOffscreenRenderer.GetTextSize(m_CopyrightLine1);
    m_SpaceBetweenLines = size.height();
}

//=============================================================================
/**
 * Destructor
 */
CSplashControl::~CSplashControl()
{
}

//=============================================================================
/**
 * Draws the splash screen.
 * @param inRenderer Renderer to draw to
 */
void CSplashControl::Draw(CRenderer *inRenderer)
{
    CRct theBounds(GetSize());
    CColor theTextColor(50, 50, 50);
    long theVertOffset = GetSize().y / 2 - 14; ///< Last line of the copyright starts here

    // Splash screen bitmap
    inRenderer->DrawBitmap(CPt(0, 0), m_Image);

    // Print the copyright text, starting at the last line and going up to the first line (just to
    // make sure that we didn't move where the text ends)
    inRenderer->DrawText(14, static_cast<float>(theVertOffset), m_VersionInfo, theBounds,
                         theTextColor);
    theVertOffset -= m_SpaceBetweenLines;
    inRenderer->DrawText(14, static_cast<float>(theVertOffset), m_CopyrightLine2, theBounds,
                         theTextColor);
    theVertOffset -= m_SpaceBetweenLines;
    inRenderer->DrawText(14, static_cast<float>(theVertOffset), m_CopyrightLine1, theBounds,
                         theTextColor);
}
