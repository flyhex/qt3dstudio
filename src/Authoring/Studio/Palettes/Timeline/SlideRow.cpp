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

#include "SlideRow.h"
#include "ColorControl.h"
#include "Bindings/ITimelineItemBinding.h"

CSlideRow::CSlideRow(CTimelineRow *parent)
    : CBaseStateRow(parent)
{
}

CSlideRow::~CSlideRow()
{
}

//=============================================================================
/**
 * Expand this node of the tree control.
 * This will display all children the fit the filter.
 */
void CSlideRow::Expand(bool inExpandAll /*= false*/, bool inExpandUp)
{
    if (!m_Loaded) {
        m_Loaded = true;
        LoadChildren();
    }

    CBaseStateRow::Expand(inExpandAll, inExpandUp);
}

//=============================================================================
/**
 * This do not 'contribute' to its child's active start time
 */
bool CSlideRow::CalculateActiveStartTime()
{
    return false;
}
//=============================================================================
/**
 * This do not 'contribute' to its child's active end time
 */
bool CSlideRow::CalculateActiveEndTime()
{
    return false;
}

bool CSlideRow::PerformFilter(const CFilter &inFilter)
{
    Q_UNUSED(inFilter);
    return true;
}
