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
//	Prefix
//==============================================================================
#ifndef INCLUDED_STATE_ROW_H
#define INCLUDED_STATE_ROW_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "BaseStateRow.h"

//==============================================================================
//	Forwards
//==============================================================================
class CButtonControl;
class CButtonDownListener;
class CColorControl;
class CStateTreeControl;
class CToggleControl;
class CStateTimebarlessRow;
class CPropertyRow;
class CCmdBatch;
class CSnapper;
class CResImage;

class CStateRow : public CBaseStateRow
{
    Q_OBJECT
public:
    CStateRow(CBaseStateRow *inParentRow);
    virtual ~CStateRow();

    using CBaseStateRow::Initialize;
    virtual void Initialize(ITimelineItemBinding *inTimelineItemBinding);

    void Expand(bool inExpandAll = false, bool inExpandUp = false) override;
    void Collapse(bool inCollapseAll = false) override;
    virtual void OnTimeChange();

    long GetLatestEndTime() override;
    bool CalculateActiveStartTime() override;
    bool CalculateActiveEndTime() override;
    bool HasVisibleChildren() override;

Q_SIGNALS:
    void timeChanged();
    void layoutRecalcRequested();

protected:
    bool PerformFilter(const CFilter &inFilter) override;
    void LoadProperties() override;
};
#endif // INCLUDED_STATE_ROW_H
