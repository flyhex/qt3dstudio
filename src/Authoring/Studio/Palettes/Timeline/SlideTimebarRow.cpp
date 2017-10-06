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

#include "SlideTimebarRow.h"
#include "SlideRow.h"

CSlideTimebarRow::CSlideTimebarRow(CSlideRow *inSlideRow)
    : m_SlideRow(inSlideRow)
{
}

CSlideTimebarRow::~CSlideTimebarRow()
{
}

void CSlideTimebarRow::CommitSelections()
{
}

void CSlideTimebarRow::SelectKeysInRect(CRct inRect, bool inModifierKeyDown)
{
    Q_UNUSED(inRect);
    Q_UNUSED(inModifierKeyDown);
}

void CSlideTimebarRow::SelectKeysByTime(long inTime, bool inSelected)
{
    Q_UNUSED(inTime);
    Q_UNUSED(inSelected);
}

void CSlideTimebarRow::SelectAllKeys()
{
}

void CSlideTimebarRow::PopulateSnappingList(CSnapper *inSnapper)
{
    Q_UNUSED(inSnapper);
}

CBaseStateRow *CSlideTimebarRow::GetBaseStateRow() const
{
    return m_SlideRow;
}

// This is not applicable to a SlideTimebarRow!!
ISnappingListProvider &CSlideTimebarRow::GetSnappingListProvider() const
{
    throw;
}