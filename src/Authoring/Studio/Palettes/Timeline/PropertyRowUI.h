/****************************************************************************
**
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
#ifndef PROPERTYROWUI_H
#define PROPERTYROWUI_H

#include "AbstractTimelineRowUI.h"

class CPropertyRow;
class CPropertyColorControl;
class CBlankControl;
class CPropertyTreeControl;
class CPropertyToggleControl;
class CPropertyTimebarRow;
class ITimelineItemProperty;

class CPropertyRowUI : public CAbstractTimelineRowUI
{
    Q_OBJECT
public:
    CPropertyRowUI(CPropertyRow *baseStateRow, CAbstractTimelineRowUI *parentUiRow);
    ~CPropertyRowUI();

    CControl *GetColorControl() override;
    CControl *GetTreeControl() override;
    CControl *GetToggleControl() override;
    CControl *GetTimebarControl() override;

    void SetIndent(long inIndent) override;
    long GetIndent() const override;

    void SetSnappingListProvider(ISnappingListProvider *inProvider) override;
    ISnappingListProvider *GetSnappingListProvider() const override;

    void OnMouseOver();
    void OnMouseOut();
    void OnMouseDoubleClick();

    void SelectKeysInRect(CRct inRect, bool inModifierKeyDown,
                          bool inGlobalCommitSelectionFlag) override;
    void SetEnabled(bool inEnabled) override;

    void CommitSelections();

    // unused methods from CAbstractTimelineRowUI
    void SetFocus() override {}
    void SetNameReadOnly(bool) override {}
    void OnChildVisibilityChanged() override {}
    void PopulateSnappingList(CSnapper *) override {}
    void DoStartDrag(CControlWindowListener *) override {}
    void AcceptDropAfter(bool) override {}
    void AcceptDropBefore(bool) override {}
    void SetDropTarget(CDropTarget *) override {}

    void UpdateActionStatus() override {}

private Q_SLOTS:
    void Initialize() {}
    void handleDirtyChanged(bool dirty);
    void setTimeRatio(double inTimeRatio);
    void handleVisibleChanged(bool visible);
    void handleSelectAllKeys();
    void handleDeleteAllKeys();
    void handleRefreshRequested();
    void handleSelectKeysByTime(long inTime, bool inSelected);

protected:
    void SetDetailedView(bool inIsDetailedView);

    CPropertyTreeControl *m_TreeControl;
    CPropertyToggleControl *m_ToggleControl;
    CPropertyColorControl *m_PropertyColorControl;
    CPropertyTimebarRow *m_TimebarRow;
    CPropertyRow *m_propertyRow;

    bool m_DetailedView = false;
    bool m_Highlighted = false;
};

#endif // PROPERTYROWUI_H
