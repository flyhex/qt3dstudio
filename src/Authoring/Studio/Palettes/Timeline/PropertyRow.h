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

#ifndef INCLUDED_PROPERTY_ROW_H
#define INCLUDED_PROPERTY_ROW_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================

#include "TimelineRow.h"
#include "StateRow.h"
#include "Rct.h"
#include <vector>

//==============================================================================
//	Forwards
//==============================================================================
class CPropertyColorControl;
class CBlankControl;
class CPropertyTreeControl;
class CPropertyToggleControl;
class CPropertyTimebarRow;
class ITimelineItemProperty;

class CPropertyRow : public CTimelineRow
{
public:
    CPropertyRow(ITimelineItemProperty *inProperty);
    virtual ~CPropertyRow();
    CControl *GetColorControl() override;
    CControl *GetTreeControl() override;
    CControl *GetToggleControl() override;
    CControl *GetTimebarControl() override;

    void SetTimeRatio(double inTimePerPixel) override;
    void SetIndent(long inIndent) override;
    void Filter(const CFilter &inFilter, bool inFilterChildren = true) override;
    void OnMouseOver();
    void OnMouseOut();
    void OnMouseDoubleClick();
    void Select(bool inIsShiftKeyPressed = false);
    void SetDetailedView(bool inIsInDetailedView);
    void CommitSelections();
    void SelectKeysInRect(CRct inRect, bool inModifierKeyDown);
    void SelectAllKeys();
    void DeleteAllKeys();
    bool IsViewable() const override;
    void SetEnabled(bool inEnabled);

    ISnappingListProvider *GetSnappingListProvider() const override;
    void SetSnappingListProvider(ISnappingListProvider *inProvider) override;
    using CTimelineRow::GetTimebarBackgroundColor;
    virtual ::CColor GetTimebarBackgroundColor();
    using CTimelineRow::GetTimebarHighlightBackgroundColor;
    virtual ::CColor GetTimebarHighlightBackgroundColor();

    void Refresh();
    ITimelineItemProperty *GetProperty() const { return m_Property; }

    CPropertyTimebarRow *GetTimebar() { return m_TimebarRow; }

protected:
    CPropertyTreeControl *m_TreeControl;
    CPropertyToggleControl *m_ToggleControl;
    CPropertyColorControl *m_PropertyColorControl;
    CPropertyTimebarRow *m_TimebarRow;
    bool m_Highlighted;
    bool m_DetailedView;
    double m_TimeRatio;
    CFilter m_Filter;
    ITimelineItemProperty *m_Property;
};
#endif // INCLUDED_PROPERTY_CONTROL_H
