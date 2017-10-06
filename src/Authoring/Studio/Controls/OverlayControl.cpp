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

//=============================================================================
// Prefix
//=============================================================================
#include "stdafx.h"

#include "OverlayControl.h"
#include "Renderer.h"

IMPLEMENT_OBJECT_COUNTER(COverlayControl)

//=============================================================================
/**
 * Create a new OverlayControl.
 */
COverlayControl::COverlayControl()
    : m_HasSizeChanged(false)
    , m_WasVisible(false)
    , m_Alpha(255)
{
    ADDTO_OBJECT_COUNTER(COverlayControl)
}

COverlayControl::~COverlayControl()
{
    REMOVEFROM_OBJECT_COUNTER(COverlayControl)
}

//=============================================================================
/**
 * Notification from the parent control that it is going to begin drawing children.
 * This is used to erase this component before the children are drawn.
 * @param inRenderer the renderer that is being drawn to.
 */
void COverlayControl::BeginDrawChildren(CRenderer *inRenderer)
{
    const auto size = GetParent()->GetSize();
    QRect theClippingRect(0, 0, size.x, size.y);

// doing this = lockup on MacX.
// not doing this = playhead redraw errors on Windows
#ifdef WIN32
    inRenderer->PushClippingRect(theClippingRect);
#endif
    // If we don't have a renderer then create one.
    if (IsVisible() && m_BufferedRenderer == NULL) {
        m_BufferedRenderer = new CBufferedRenderer(QSize(size.x, size.y));
    }

    else if (m_HasSizeChanged && m_BufferedRenderer != NULL) {
        CRct theRect(m_PreviousSize);
        theRect.Offset(-m_PositionOffset + GetPosition());

        // Overwrite the current contents, clearing this object's drawing.
        inRenderer->BitBltFrom(theRect, m_BufferedRenderer, 0, 0);

        m_HasSizeChanged = false;

        m_BufferedRenderer = new CBufferedRenderer(QSize(size.x, size.y));
    }

    else {
        if ((IsVisible() || m_WasVisible) && m_BufferedRenderer != NULL) {
            // If this moved since the last begin draw then take in that offset too.
            CRct theRect(GetSize());
            theRect.Offset(-m_PositionOffset + GetPosition());

            // Overwrite the current contents, clearing this object's drawing.
            inRenderer->BitBltFrom(theRect, m_BufferedRenderer, 0, 0);

            if (m_WasVisible)
                m_BufferedRenderer = nullptr;

            m_WasVisible = false;
        }
    }

    m_PositionOffset.x = 0;
    m_PositionOffset.y = 0;

#ifdef WIN32
    inRenderer->PopClippingRect();
#endif
}

//=============================================================================
/**
 * Overrides OnDraw to perform custom drawing logic.
 * This basically needs to draw all the time regardless of invalidation.
 * The dirty rect will be properly updated if this is dirty.
 * @param inRenderer the renderer to draw to.
 * @param inDirtyRect the resulting dirty rect from drawing.
 * @param inIgnoreValidation true if this should draw everything.
 */
void COverlayControl::OnDraw(CRenderer *inRenderer, CRct &inDirtyRect, bool inIgnoreValidation)
{
    CRct theClippingRect(-GetPosition(), GetParent()->GetSize());
    inRenderer->PushClippingRect(theClippingRect);

    if (IsVisible() && m_BufferedRenderer != nullptr) {
        // Copy the current contents of the background.
        m_BufferedRenderer->BitBltFrom(CRct(GetSize()), inRenderer, 0, 0);

        bool isInvalidated = IsInvalidated();

        if (isInvalidated || inIgnoreValidation) {
            // If this is invalidated then update the dirty rect appropriately
            CRct theBoundingBox = inRenderer->GetClippingRect();

            CRct theRect(GetSize());
            theRect.And(theBoundingBox);
            theRect.Offset(inRenderer->GetTranslation());
            inDirtyRect.Or(theRect);
        }

        // if ( m_Alpha != 255 )
        //{
        //	DrawAlpha( inRenderer );
        //}
        // else
        {
            // Always draw this component to the renderer, regardless of whether it's dirty or not.
            Draw(inRenderer);
        }

        Invalidate(false);
    }
    CControl::OnDraw(inRenderer, inDirtyRect, inIgnoreValidation);

    inRenderer->PopClippingRect();
}

//=============================================================================
/**
 * Handles drawing this control as being transparent.
 * @param inRenderer the renderer to draw to.
 */
void COverlayControl::DrawAlpha(CRenderer * /*inRenderer*/)
{
    /*	// Copy the contents of the current background, this will make it so only the areas
            // that are drawn to are actually blended
            BUFFEREDRENDERERTYPE theIntermediate( m_BufferedRenderer,
       m_BufferedRenderer->GetClippingRect() );
            Draw( &theIntermediate );

            // Do the transparent draw.
            theIntermediate.TransparentBltTo( inRenderer, m_Alpha );*/
}

//=============================================================================
/**
 * Logs the changes in position so that it can properly reset the background.
 * @param inPosition the new position.
 */
void COverlayControl::SetPosition(CPt inPosition)
{
    if (inPosition != GetPosition()) {
        // Offset the position, make it relative just in case multiple changes occur per cycle.
        m_PositionOffset += inPosition - GetPosition();

        CControl::SetPosition(inPosition);
    }
}

//=============================================================================
/**
 * Override of SetSize so that the renderer can be ditched.
 * @param inSize the new size of this control.
 */
void COverlayControl::SetSize(CPt inSize)
{
    if (inSize != GetSize()) {
        // Record how big this was so the previous drawing can be erased.
        if (!m_HasSizeChanged && m_BufferedRenderer != nullptr) {
            m_HasSizeChanged = true;
            m_PreviousSize = GetSize();
        }

        CControl::SetSize(inSize);
    }
}

//=============================================================================
/**
 * Set whether or not this control is visible.
 * Override to handle final clearing of this control when not visible.
 * @param inIsVisible true if this is to be visible.
 */
void COverlayControl::SetVisible(bool inIsVisible)
{
    if (!inIsVisible && IsVisible()) {
        m_WasVisible = true;
    }
    CControl::SetVisible(inIsVisible);
}

//=============================================================================
/**
 * Set the alpha blend of this control.
 * This defaults to 255 which is opaque, if it is anything other than 255 then
 * this control will be drawn with transparency.
 * @param inAlpha the new opacity, 255 = opaque, 0 = transparent.
 */
void COverlayControl::SetAlpha(short inAlpha)
{
    m_Alpha = inAlpha;
}

bool COverlayControl::IsChildInvalidated() const
{
    return true;
}
