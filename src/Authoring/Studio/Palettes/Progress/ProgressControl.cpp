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
#include "Strings.h"

//==============================================================================
// Includes
//==============================================================================
#include "ProgressControl.h"
#include "Renderer.h"
#include "ResourceCache.h"
#include "StringLoader.h"

//==============================================================================
/**
 * Constructor
 */
CProgressControl::CProgressControl()
    : m_Percent(0)
{
    // Load the image
    m_Image = CResourceCache::GetInstance()->GetBitmap("progress-screen.png");

    // Load the default string for the window for now
    m_ActionText = LoadResourceString(IDS_WAIT_LOADING).toQString();
}

//==============================================================================
/**
 * Destructor
 */
CProgressControl::~CProgressControl()
{
}

//==============================================================================
/**
 * The size of this control is equal to the size of the image that we display.
 * @return the size (in pixels) of this control.
 */
CPt CProgressControl::GetPreferredSize()
{
    return CPt(m_Image.width(), m_Image.height());
}

//==============================================================================
/**
 * Draws this control.
 * @param inRenderer renderer to draw to.
 */
void CProgressControl::Draw(CRenderer *inRenderer)
{
    QRect r(QPoint(), GetSize());

    // Draw the image over the whole window
    inRenderer->DrawBitmap(CPt(0, 0), m_Image);

    inRenderer->PushPen(QColor(255, 255, 255));

    // Show "Loading..."
    inRenderer->DrawText(105, 20, m_ActionText);

    // Show the file name
    if (!m_FileName.isEmpty())
        inRenderer->DrawText(105, 35, m_FileName);

    // Show the percentage
    inRenderer->DrawText(105, 50, m_PercentString);

    inRenderer->PopPen();
}

//==============================================================================
/**
 * Sets the text displayed above the file name.  For instance: "Loading..."
 * @param inText text to be shown above the file name
 */
void CProgressControl::SetActionText(const Q3DStudio::CString &inText)
{
    m_ActionText = inText.toQString();
    Invalidate();
}

//==============================================================================
/**
 * Changes the percentage complete displayed by this control.
 * @param inPercent new percentage complete.
 */
void CProgressControl::SetProgress(long inPercent)
{
    m_Percent = inPercent;
    m_PercentString = QStringLiteral("%1 %").arg(inPercent);
    Invalidate();
}

//==============================================================================
/**
 * Sets the name of the file that is being opened.  This is displayed on the
 * control.
 * @param inFileName File name to display in the middle of this control
 */
void CProgressControl::SetFileName(const Q3DStudio::CString &inFileName)
{
    m_FileName = inFileName.toQString();
    Invalidate();
}
