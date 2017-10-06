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

#ifndef INCLUDED_BASE_TIMEBARLESS_ROW_H
#define INCLUDED_BASE_TIMEBARLESS_ROW_H 1

#pragma once

#include "Control.h"
#include "CColor.h"

class CSnapper;
class CBaseStateRow;
class ISnappingListProvider;

class CBaseTimebarlessRow : public CControl
{
public:
    CBaseTimebarlessRow();
    virtual ~CBaseTimebarlessRow();

    void Draw(CRenderer *inRenderer) override;

    virtual void SetBackgroundColor(::CColor inColor);
    virtual void SetTimeRatio(double inTimeRatio);

    virtual void RefreshRowMetaData();

    virtual void OnSelect();
    virtual void OnDeselect();

    void OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;

    virtual void SetDirty(bool inIsDirty);
    virtual void UpdateTime(long inStartTime, long inEndTime);

    virtual void CommitSelections() = 0;
    virtual void SelectKeysInRect(CRct inRect, bool inModifierKeyDown) = 0;
    virtual void SelectAllKeys() = 0;
    virtual void SelectKeysByTime(long inTime, bool inSelected) = 0;
    virtual void PopulateSnappingList(CSnapper *inSnappingList) = 0;
    virtual ISnappingListProvider &GetSnappingListProvider() const = 0;

protected:
    virtual CBaseStateRow *GetBaseStateRow() const = 0;

    ::CColor m_BackgroundColor;
    bool m_Selected;
    bool m_DirtyFlag;
    double m_TimeRatio;
};
#endif // INCLUDED_BASE_TIMEBARLESS_ROW_H
