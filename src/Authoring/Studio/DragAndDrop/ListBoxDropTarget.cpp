/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#include "ListBoxDropTarget.h"
#include "ListBoxDropSource.h"
#include "IDragable.h"

//===============================================================================
/**
 * 	Constructor
 */
CListBoxDropTarget::CListBoxDropTarget()
    : m_Item(nullptr)
{
}

//===============================================================================
/**
 * 	This get called on every DragWithin.
 *	Note: the source will validate the target instead of the otherway around.
 *	This is because the DropSource knows how to get information from itself without
 *	creating an asset. This is mainly for DropSources that have a lazy creation idiom.
 *  like files.
 *	@param the DropSource in question.
 *	@return true if the DropSource likes the DropTarget.
 */
bool CListBoxDropTarget::Accept(CDropSource &inSource)
{
    return inSource.ValidateTarget(this);
}

//===============================================================================
/**
 *	This is where is actually happens.
 *	Note: At this point everything should be verified, and setup in the dropsource.
 *	The only thing left to do is to get the Assets and move/copy or connect them.
 *  @param inSource the Object in question.
 *	@return true if the drop was successful .
 */
bool CListBoxDropTarget::Drop(CDropSource &inSource)
{
    if (m_Item) {
        inSource;
        /*
        CSlideDropSource*		theSlideDropSource = static_cast< CSlideDropSource* >(
        &inSource );
        CSlideControl*			theSlideControl = theSlideDropSource->GetSlide( );
        CTimeContext*			theTimeContext = theSlideControl->GetTimeContext( );

        // Reorder this slide to the position held by this drop target.
        CCmdRearrangeTimeContext*	theCommand = new CCmdRearrangeTimeContext( theTimeContext,
        m_SlideInsert->GetInsertIndex( ) );
        if ( theCommand )
        {
                theTimeContext->GetAsset( )->ExecuteCommand( theCommand, false );
        }
        */
    }

    // we are always successful
    return true;
}

//===============================================================================
/**
 * 	 This will get the objec ttype from the Asset.
 *	 Note: The asset can change all of the time, so i always ask the asset for its type.
 *	@return the Studio object type.
 */
long CListBoxDropTarget::GetObjectType()
{
    return EUIC_FLAVOR_LISTBOX;
}

//===============================================================================
/**
 *	Set the SlideInsertionControl that is the drop target
 */
void CListBoxDropTarget::SetItem(CListBoxItem *inItem)
{
    m_Item = inItem;
}