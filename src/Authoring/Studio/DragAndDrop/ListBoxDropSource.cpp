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
#include "ListBoxDropSource.h"
#include "DropTarget.h"
#include "ListBoxItem.h"
#include "IDragable.h"

//===============================================================================
/**
 *	Constructor
 */
CListBoxDropSource::CListBoxDropSource(long inFlavor, IDragable *inDragable)
    : CDropSource(inFlavor, 0)
{
    m_Item = reinterpret_cast<CListBoxItem *>(inDragable);
}

//===============================================================================
/**
 * 	Validate that the drop target is QT3DS_FLAVOR_LISTBOX type.
 *	@param inTarget	Drop target for validation
 *	@return true if inTarget is QT3DS_FLAVOR_LISTBOX; false if otherwise
 */
bool CListBoxDropSource::ValidateTarget(CDropTarget *inTarget)
{
    bool theValidTarget = (inTarget->GetObjectType() == QT3DS_FLAVOR_LISTBOX);
    SetHasValidTarget(theValidTarget);

    return theValidTarget;
}

//===============================================================================
/**
 *	@return true
 */
bool CListBoxDropSource::CanMove()
{
    return true;
}

//===============================================================================
/**
 *	@return true
 */
bool CListBoxDropSource::CanCopy()
{
    return true;
}
