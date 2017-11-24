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

#ifndef INCLUDED_TIME_CONTEXT_TIMEBAR_ROW_H
#define INCLUDED_TIME_CONTEXT_TIMEBAR_ROW_H 1

#pragma once

#include "BaseTimebarlessRow.h"

class CSlideRowUI;

class CSlideTimebarRow : public CBaseTimebarlessRow
{
public:
    CSlideTimebarRow(CSlideRowUI *inSlideRow);
    virtual ~CSlideTimebarRow();

    void CommitSelections() override;

    void SelectKeysInRect(CRct inRect, bool inModifierKeyDown) override;
    void SelectKeysByTime(long inTime, bool inSelected) override;
    void SelectAllKeys() override;

    void PopulateSnappingList(CSnapper *inSnappingList) override;
    ISnappingListProvider &GetSnappingListProvider() const override;

protected:
    CBaseStateRowUI *GetBaseStateRowUI() const override;

protected:
    CSlideRowUI *m_SlideRowUi;
};
#endif // INCLUDED_TIME_CONTEXT_TIMEBAR_ROW_H
