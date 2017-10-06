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
#ifndef INCLUDED_STATE_TIMEBARLESS_ROW_H
#define INCLUDED_STATE_TIMEBARLESS_ROW_H 1

//==============================================================================
//	Includes
//==============================================================================
#include "BaseTimebarlessRow.h"
#include "DispatchListeners.h"

//==============================================================================
//	Forwards
//==============================================================================
class CStateRow;
class CSnapper;
class CAssetTimelineKeyframe;
class ITimelineItemBinding;

class CStateTimebarlessRow : public CBaseTimebarlessRow
{
    typedef std::vector<CAssetTimelineKeyframe *> TTimelineAssetKeyframeList;

public:
    CStateTimebarlessRow(CStateRow *inStateRow);
    virtual ~CStateTimebarlessRow();

    void OnDraw(CRenderer *inRenderer, CRct &inDirtyRect, bool inIgnoreValidation = false) override;
    void Draw(CRenderer *inRenderer) override;

    bool OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;

    void OnKeySelected(long inTime, bool inState, bool inClearPreviouslySelectedKeys);

    void SetTimeRatio(double inTimeRatio) override;

    virtual void RefreshKeyframes();
    void Invalidate(bool inInvalidate = true) override;
    void CommitSelections() override;
    void SelectKeysInRect(CRct inRect, bool inModifierKeyDown) override;
    void SelectAllKeys() override;
    void SelectKeysByTime(long inTime, bool inSelected) override;
    bool HasSelectedKeys();
    CStateRow *GetStateRow();
    void PopulateSnappingList(CSnapper *inSnapper) override;

protected:
    CBaseStateRow *GetBaseStateRow() const override;
    bool PropertiesHaveKeyframe(long inTime);

protected:
    CStateRow *m_StateRow;
    bool m_Selected;
    TTimelineAssetKeyframeList m_Keyframes; ///<Master Keyframe list ( STL )
    bool m_Refreshing;
};
#endif // INCLUDED_STATE_TIMEBARLESS_ROW_H
