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

#include "AbstractTimelineRowUI.h"
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
    Q_OBJECT
public:
    CPropertyRow(ITimelineItemProperty *inProperty, CTimelineRow *parent);
    virtual ~CPropertyRow();

    void SetTimeRatio(double inTimePerPixel) override;
    void Filter(const CFilter &inFilter, bool inFilterChildren = true) override;

    void Select(Qt::KeyboardModifiers inKeyState, bool inCheckKeySelection = true) override {}
    void Select(bool inIsShiftKeyPressed = false);
    bool IsSelected() const override;

    void DeleteAllKeys();
    bool IsViewable() const override;

    using CTimelineRow::GetTimebarBackgroundColor;
    virtual ::CColor GetTimebarBackgroundColor();
    using CTimelineRow::GetTimebarHighlightBackgroundColor;
    virtual ::CColor GetTimebarHighlightBackgroundColor();

    void Refresh();
    ITimelineItemProperty *GetProperty() const { return m_Property; }

    void Expand(bool inExpandAll = false, bool inExpandUp = false) override {}
    void Collapse(bool inCollapseAll = false) override {}
    bool CalculateActiveStartTime() override { return true; }
    bool CalculateActiveEndTime() override { return true; }

    // not implemented methods
    void Initialize(ITimelineItemBinding *) override {}
    void LoadChildren() override {}
    bool HasVisibleChildren() override {return false;}

Q_SIGNALS:
    void visibleChanged(bool visible);
    void deleteAllKeys();
    void selectAllKeys();
    void refreshRequested();


protected:
    CFilter m_Filter;
    ITimelineItemProperty *m_Property;
};
#endif // INCLUDED_PROPERTY_CONTROL_H
