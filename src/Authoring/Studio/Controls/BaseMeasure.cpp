/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#include "BaseMeasure.h"
#include "Renderer.h"
#include "StudioPreferences.h"

//=============================================================================
/**
 * Create a new measure.
 * @param inRatio the current ratio.
 * @param inFillBackground true if the background of this control is to be drawn.
 */
CBaseMeasure::CBaseMeasure(double inRatio, bool inFillBackground /*= true */)
    : m_SmallHashInterval(0)
    , m_MediumHashInterval(0)
    , m_LargeHashInterval(0)
    , m_Ratio(inRatio)
    , m_Offset(0)
    , m_EdgeMargin(0)
    , m_LargeHashOffset(0)
    , m_MediumHashOffset(6)
    , m_SmallHashOffset(3)
    , m_FillBackground(inFillBackground)
{
    m_BackgroundColor = CStudioPreferences::GetBaseColor();
}

CBaseMeasure::~CBaseMeasure()
{
}

//=============================================================================
/**
 * Overrides the control to set up clipping rects.
 * @param inRenderer the renderer to be rendered to.
 * @param inDirtyRect the resulting dirty rect.
 * @param inIgnoreValidation true if everything needs to be drawn.
 */
void CBaseMeasure::OnDraw(CRenderer *inRenderer, CRct &inDirtyRect, bool inIgnoreValidation)
{
    inRenderer->PushClippingRect(CRct(GetSize()));

    CControl::OnDraw(inRenderer, inDirtyRect, inIgnoreValidation);

    inRenderer->PopClippingRect();
}

//=============================================================================
/**
 * Draw the measure.
 * @param inRenderer the renderer to draw to.
 */
void CBaseMeasure::Draw(CRenderer *inRenderer)
{
    CRct theRect(GetSize());

    if (m_FillBackground)
        inRenderer->FillSolidRect(theRect, m_BackgroundColor);

    long theLength = GetDisplayLength();
    long theHeight = GetDisplayHeight();

    double theTotalMeasure = theLength / m_Ratio;
    long thePos;

    long theNumLargeHashes = (long)(theTotalMeasure / m_LargeHashInterval) + 1;

    inRenderer->PushPen(CStudioPreferences::GetRulerTickColor());

    long theOffset = m_Offset - (m_Offset % ::dtol(m_LargeHashInterval));

    for (long i = 0; i < theNumLargeHashes + 1; ++i) {
        double theMeasure = m_LargeHashInterval * i + theOffset;

        thePos = CalculatePos(theMeasure - m_Offset);

        if (thePos > 0)
            DrawLine(inRenderer, thePos, theHeight, theHeight - m_LargeHashOffset);

        DrawMeasureText(inRenderer, thePos, long(theMeasure));

        // sanity check
        if (m_MediumHashInterval > 0) {
            // in cases where the medium and small hashes must be filled up to the right of the
            // first Large hash
            if (m_Offset < 0 && i == 0) {
                thePos = CalculatePos(theMeasure - m_Offset - m_MediumHashInterval);
                if (thePos > 0)
                    DrawLine(inRenderer, thePos, theHeight, theHeight - m_MediumHashOffset);

                for (double theSmallInterval = 0; theSmallInterval < m_LargeHashInterval;
                     theSmallInterval += m_SmallHashInterval) {
                    thePos = CalculatePos(theMeasure - m_Offset - theSmallInterval);
                    if (thePos > 0)
                        DrawLine(inRenderer, thePos, theHeight, theHeight - m_SmallHashOffset);
                }
            }

            thePos = CalculatePos(theMeasure - m_Offset + m_MediumHashInterval);
            if (thePos > 0)
                DrawLine(inRenderer, thePos, theHeight, theHeight - m_MediumHashOffset);

            for (double theSmallInterval = 0; theSmallInterval < m_LargeHashInterval;
                 theSmallInterval += m_SmallHashInterval) {
                thePos = CalculatePos(theMeasure - m_Offset + theSmallInterval);

                if (thePos > 0)
                    DrawLine(inRenderer, thePos, theHeight, theHeight - m_SmallHashOffset);
            }
        } // if medium is valid
    }
    // Draws the top outline
    DrawOutline(inRenderer, theHeight, 0, theLength);
    inRenderer->PushPen(CStudioPreferences::GetButtonHighlightColor());
    // Draws the bottom outline
    DrawOutline(inRenderer, theHeight + 1, 0, theLength);
    inRenderer->PopPen();

    // Dark line at left and right ends of the measure
    inRenderer->PushPen(CStudioPreferences::GetButtonShadowColor());
    DrawLine(inRenderer, theRect.size.x - 1, 0, theRect.size.y - 1);

    // Dark line across the top of the measure
    DrawOutline(inRenderer, 0, 0, theRect.size.x - 1);
    inRenderer->PopPen();

    // pop for the first PushPen
    inRenderer->PopPen();
}

//=============================================================================
/**
 * Return the display length, by default, this is horizontal and returns the x
 */
long CBaseMeasure::GetDisplayLength()
{
    return GetSize().x;
}

//=============================================================================
/**
 * Return the display height, by default, this is horizontal and returns the y
 */
long CBaseMeasure::GetDisplayHeight()
{
    return GetSize().y - m_EdgeMargin;
}

//=============================================================================
/**
 * By default, this draws a vertical line.
 * A subclass can override this and draw a horizontal one, if its orientation requires so.
 * @parma inRenderer	the renderer
 * @param inPos			the position that in the x-coordinate
 * @param inStart		start position of the line
 * @param inEnd			end position of the line
 */
void CBaseMeasure::DrawLine(CRenderer *inRenderer, long inPos, long inStart, long inEnd)
{
    inRenderer->MoveTo(CPt(inPos, inStart));
    inRenderer->LineTo(CPt(inPos, inEnd));
}

//=============================================================================
/**
 * This draws a horizontal outline
 * A subclass can override this and draw a vertical one, if its orientation requires so.
 * @parma inRenderer	the renderer
 * @param inPos			the position that in the x-coordinate
 * @param inStart	start position of the line
 * @param inEnd		end position of the line
 */
void CBaseMeasure::DrawOutline(CRenderer *inRenderer, long inPos, long inStart, long inEnd)
{
    inRenderer->MoveTo(CPt(inStart, inPos));
    inRenderer->LineTo(CPt(inEnd, inPos));
}
