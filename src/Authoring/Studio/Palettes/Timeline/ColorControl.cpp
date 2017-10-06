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

#include "CColor.h"
#include "ColorControl.h"
#include "CoreUtils.h"
#include "BaseStateRow.h"
#include "Renderer.h"
#include "StudioPreferences.h"
#include "HotKeys.h"
#include "Dispatch.h"
#include "ResourceCache.h"
#include "Bindings/ITimelineItem.h"

//==============================================================================
/**
 *	The control (of each row) to the left of the tree view, where it indicates if there are
 *actions associated with this row.
 */
CColorControl::CColorControl(CBaseStateRow *inRow)
    : m_Shaded(true)
    , m_Selected(false)
    , m_HasAction(false)
    , m_HasMasterAction(false)
    , m_ChildHasAction(false)
    , m_ChildHasMasterAction(false)
    , m_ComponentHasAction(false)
    , m_ComponentHasMasterAction(false)
{
    m_ParentRow = inRow;
    m_BackgroundColor = m_ParentRow->GetTimebarBackgroundColor(m_ParentRow->GetObjectType());

    UpdateIconStatus();
}

CColorControl::~CColorControl()
{
}

void CColorControl::SetShaded(bool inIsShaded)
{
    m_Shaded = inIsShaded;
}

//==============================================================================
/**
 *	Draw
 *
 *  draws this object
 *
 *	@param	inRenderer	 a renderer object
 */
void CColorControl::Draw(CRenderer *inRenderer)
{
    CRct theRect(GetSize());

    inRenderer->FillSolidRect(theRect, m_BackgroundColor);
    //
    inRenderer->PushPen(CStudioPreferences::GetPropertyFloorColor());
    // bottom
    inRenderer->MoveTo(CPt(1, theRect.size.y - 1));
    inRenderer->LineTo(CPt(theRect.size.x - 1, theRect.size.y - 1));
    // left
    inRenderer->MoveTo(CPt(0, 1));
    inRenderer->LineTo(CPt(0, theRect.size.y - 1));
    // right
    inRenderer->MoveTo(CPt(theRect.size.x - 1, 1));
    inRenderer->LineTo(CPt(theRect.size.x - 1, theRect.size.y - 1));
    //
    inRenderer->PopPen();

    CPt thePos(0, 0);
    if (m_HasMasterAction) {
        if (m_ActionImages[IMAGETYPE_MASTERACTION].isNull())
            m_ActionImages[IMAGETYPE_MASTERACTION] =
                CResourceCache::GetInstance()->GetBitmap("Action-MasterAction.png");

        inRenderer->DrawBitmap(thePos, m_ActionImages[IMAGETYPE_MASTERACTION]);
    } else if (m_HasAction) {
        if (m_ActionImages[IMAGETYPE_ACTION].isNull())
            m_ActionImages[IMAGETYPE_ACTION] =
                CResourceCache::GetInstance()->GetBitmap("Action-Action.png");

        inRenderer->DrawBitmap(thePos, m_ActionImages[IMAGETYPE_ACTION]);
    }
    if (m_ChildHasMasterAction) {
        if (m_ActionImages[IMAGETYPE_CHILDMASTERACTION].isNull())
            m_ActionImages[IMAGETYPE_CHILDMASTERACTION] =
                CResourceCache::GetInstance()->GetBitmap("Action-ChildMasterAction.png");

        inRenderer->DrawBitmap(thePos, m_ActionImages[IMAGETYPE_CHILDMASTERACTION]);
    } else if (m_ChildHasAction) {
        if (m_ActionImages[IMAGETYPE_CHILDACTION].isNull())
            m_ActionImages[IMAGETYPE_CHILDACTION] =
                CResourceCache::GetInstance()->GetBitmap("Action-ChildAction.png");

        inRenderer->DrawBitmap(thePos, m_ActionImages[IMAGETYPE_CHILDACTION]);
    }
    if (m_ComponentHasMasterAction) {
        if (m_ActionImages[IMAGETYPE_COMPONENTMASTERACTION].isNull())
            m_ActionImages[IMAGETYPE_COMPONENTMASTERACTION] =
                CResourceCache::GetInstance()->GetBitmap("Action-ComponentMasterAction.png");

        inRenderer->DrawBitmap(thePos, m_ActionImages[IMAGETYPE_COMPONENTMASTERACTION]);
    } else if (m_ComponentHasAction) {
        if (m_ActionImages[IMAGETYPE_COMPONENTACTION].isNull())
            m_ActionImages[IMAGETYPE_COMPONENTACTION] =
                CResourceCache::GetInstance()->GetBitmap("Action-ComponentAction.png");

        inRenderer->DrawBitmap(thePos, m_ActionImages[IMAGETYPE_COMPONENTACTION]);
    }

    // the old code with selection

    // inRenderer->DrawGradientBitmap( theRect, m_ParentRow->GetAsset( )->GetTimebarColor( ), false
    // );
    // if ( m_Selected )
    //{
    //	long theXSize = theRect.size.x / 2;
    //	long theYSize = theRect.size.y / 2;
    //	inRenderer->FillSolidRect( CRct( CPt( theXSize / 2, theYSize / 2 ), CPt( theXSize, theYSize)
    //), CalculateSelectedColor( m_ParentRow->GetAsset( )->GetTimebarColor( ) ) );
    //}

    // if this is selected then do something special
    // if ( m_Shaded )
    //{
    //	// This is used for the left line of the color control
    //	CColor theNonGradiantHighlight = m_ParentRow->GetAsset( )->GetTimebarColor( );
    //	float theLuminance = theNonGradiantHighlight.GetLuminance( );
    //	theLuminance = theLuminance * (float)1.15;
    //	if ( theLuminance > 1.0 )
    //	{
    //		theLuminance = (float)1.0;
    //	}
    //
    //	// Highlight on left edge
    //	theNonGradiantHighlight.SetLuminance( theLuminance );
    //	inRenderer->PushPen( theNonGradiantHighlight );
    //	inRenderer->MoveTo( CPt( 1, 0 ) );
    //	inRenderer->LineTo( CPt( 1, theRect.size.y - 1 ) );
    //	inRenderer->PopPen( );

    //	// Dark line on far left edge
    //	inRenderer->PushPen( CStudioPreferences::GetRowTopColor( ) );
    //	inRenderer->MoveTo( CPt( 0, 0 ) );
    //	inRenderer->LineTo( CPt( 0, theRect.size.y - 1 ) );
    //	inRenderer->PopPen( );
    //
    //	inRenderer->PushPen( CStudioPreferences::GetRowTopColor( ) );
    //	inRenderer->MoveTo( CPt( 0,theRect.size.y - 1 ) );
    //	inRenderer->LineTo( CPt( theRect.size.x, theRect.size.y - 1 ) );
    //	inRenderer->MoveTo( CPt( theRect.size.x - 1, 0 ) );
    //	inRenderer->LineTo( CPt( theRect.size.x - 1, theRect.size.y - 1 ) );
    //	inRenderer->PopPen( );
    //}

    // inRenderer->Draw3dRect( CRct( theRect.position, CPt( theRect.size.x, theRect.size.y + 1 ) ),
    // CColor( 0, 255, 0 ), CColor( 0, 255, 0 ) );
}

//==============================================================================
/**
 *	CalculateNonGradiantHighlight
 *
 *  calculates the highlight for this color
 *
 *	@param	inColor	 color to multiply
 */
CColor CColorControl::CalculateNonGradiantHighlight(CColor inColor)
{
    double theNonGradiantHighlightR = inColor.GetRed() * 1.15;
    double theNonGradiantHighlightG = inColor.GetGreen() * 1.15;
    double theNonGradiantHighlightB = inColor.GetBlue() * 1.15;
    if (theNonGradiantHighlightR > 255) {
        theNonGradiantHighlightR = 255;
    }
    if (theNonGradiantHighlightG > 255) {
        theNonGradiantHighlightG = 255;
    }
    if (theNonGradiantHighlightB > 255) {
        theNonGradiantHighlightB = 255;
    }
    return CColor(::dtol(theNonGradiantHighlightR), ::dtol(theNonGradiantHighlightG),
                  ::dtol(theNonGradiantHighlightB));
}

//==============================================================================
/**
 *	CalculateNonGradiantHighlight
 *
 *  calculates the highlight for this color
 *
 *	@param	inColor	 color to multiply
 */
CColor CColorControl::CalculateSelectedColor(CColor inColor)
{
    double theNonGradiantHighlightR = inColor.GetRed() * 0.65;
    double theNonGradiantHighlightG = inColor.GetGreen() * 0.65;
    double theNonGradiantHighlightB = inColor.GetBlue() * 0.65;
    if (theNonGradiantHighlightR > 255) {
        theNonGradiantHighlightR = 255;
    }
    if (theNonGradiantHighlightG > 255) {
        theNonGradiantHighlightG = 255;
    }
    if (theNonGradiantHighlightB > 255) {
        theNonGradiantHighlightB = 255;
    }
    return CColor(::dtol(theNonGradiantHighlightR), ::dtol(theNonGradiantHighlightG),
                  ::dtol(theNonGradiantHighlightB));
}

//==============================================================================
/**
 *	Handles the OnMouseDownEvent
 */
bool CColorControl::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    Q_UNUSED(inPoint);
    Q_UNUSED(inFlags);
    return true;
}

//==============================================================================
/**
 * Called when this row becomes selected
 */
void CColorControl::OnSelect()
{
    m_Selected = true;
    this->Invalidate();
}

//==============================================================================
/**
 * Called when this row becomes deselected
 */
void CColorControl::OnDeselect()
{
    m_Selected = false;
    this->Invalidate();
}

//==============================================================================
/**
 *	Updates the icon used for display based on the following logic:
 * 1. Self has actions
 * 2. Descendents have actions
 * Results in 4 states.
 */
void CColorControl::UpdateIconStatus()
{
    ITimelineItem *theTimelineItem = m_ParentRow->GetTimelineItem();

    if (!theTimelineItem)
        return;

    // Master 'supersede' non-master
    m_HasMasterAction = theTimelineItem->HasAction(true);
    if (!m_HasMasterAction)
        m_HasAction = theTimelineItem->HasAction(false);

    // no descendent info if current row is expanded
    if (!m_ParentRow->IsExpanded()) {
        m_ChildHasMasterAction = theTimelineItem->ChildrenHasAction(true);
        if (!m_ChildHasMasterAction)
            m_ChildHasAction = theTimelineItem->ChildrenHasAction(false);
    } else {
        m_ChildHasMasterAction = false;
        m_ChildHasAction = false;
    }

    m_ComponentHasMasterAction = theTimelineItem->ComponentHasAction(true);
    if (!m_ComponentHasMasterAction)
        m_ComponentHasAction = theTimelineItem->ComponentHasAction(false);

    this->Invalidate();
}
