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

#include "StateRowFactory.h"
#include "Bindings/ITimelineItemBinding.h"
#include "StateRow.h"

//=============================================================================
/**
 * Create the type specific StateRow for the Asset.
 * Different asset use different derivations of StateRow, and this will
 * return the proper state row for the asset.
 * @param inTimelineItem the timeline item to create the state row for.
 * @param inParentRow the parent row of the state row being created.
 * @param inSnappingListProvider For keyframe/timebar snapping
 * @return CStateRow the row that represents the state, or nullptr if it should not show up.
 */
CStateRow *CStateRowFactory::CreateStateRow(ITimelineItemBinding *inTimelineItem,
                                            CBaseStateRow *inParentRow,
                                            ISnappingListProvider *inSnappingListProvider)
{
    CStateRow *theRow = nullptr;
    if (inTimelineItem) {
        theRow = new CStateRow(inParentRow);

        if (theRow != nullptr) {
            theRow->Initialize(inTimelineItem, inSnappingListProvider);
        }
    }

    return theRow;
}
