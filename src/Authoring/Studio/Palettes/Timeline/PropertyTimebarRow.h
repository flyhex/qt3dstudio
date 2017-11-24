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

#ifndef INCLUDED_PROPERTY_TIMEBAR_ROW_H
#define INCLUDED_PROPERTY_TIMEBAR_ROW_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Control.h"
#include "PropertyTimebarGraph.h"
#include "StateRow.h"

//==============================================================================
//	forwards
//==============================================================================
class CPropertyRowUI;
class CPropertyTimelineKeyframe;
class ISnappingListProvider;

class CPropertyTimebarRow : public CControl
{
    typedef std::vector<CPropertyTimelineKeyframe *> TTimelineKeyframeList;

public:
    CPropertyTimebarRow(CPropertyRowUI *inPropertyRowUI);
    virtual ~CPropertyTimebarRow();

    void Draw(CRenderer *inRenderer) override;
    void OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;

    void SetHighlighted(bool inIsHighlighted);
    void RefreshKeyframes();

    void SetSize(CPt inSize) override;

    CPropertyRowUI *GetPropertyRowUI();

    void SetTimeRatio(double inTimeRatio);

    void SetDetailedView(bool inDetailedView);
    void OnKeySelected(long inTime, bool inSelected);
    void DeselectAllKeyframes();
    void SetDirty(bool inDirtyFlag);
    void CommitSelections();
    void SelectKeysInRect(CRct inRect, bool inModifierKeyDown);
    void SelectAllKeys();
    bool HasSelectedKeys();
    void Invalidate(bool inInvalidate = true) override;
    void SetVisible(bool inIsVisible) override;

    bool HasKeyframe(long inTime) const;
    void SelectKeysByTime(long inTime, bool inSelected);

    void SetSnappingListProvider(ISnappingListProvider *inProvider);
    ISnappingListProvider &GetSnappingListProvider() const;

protected:
    void DrawColor(CRenderer *inRenderer);

protected:
    TTimelineKeyframeList m_Keyframes; ///< Properties Keyframe List (STL)
    CPropertyRowUI *m_PropertyRowUI;
    CPropertyTimebarGraph m_DetailedView;
    ::CColor m_BackgroundColor;
    double m_TimeRatio;

    bool m_DirtyFlag;
    bool m_Refreshing;
    ISnappingListProvider *m_SnappingListProvider;
};
#endif // INCLUDED_STATE_TIMEBAR_ROW_H
