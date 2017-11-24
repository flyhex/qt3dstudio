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
#ifndef BASESTATEROWUI_H
#define BASESTATEROWUI_H

#include "AbstractTimelineRowUI.h"
#include "ListLayout.h"

#include <QObject>

class CBaseTimelineTreeControl;
class CColorControl;
class CBlankToggleControl;
class CBaseTimebarlessRow;
class CBaseStateRow;
class CPropertyRow;
class CFilter;
class CSnapper;

class CBaseStateRowUI : public CAbstractTimelineRowUI
{
    Q_OBJECT
public:
    static const long DEFAULT_TOGGLE_LENGTH;

    CBaseStateRowUI(CBaseStateRow *baseStateRow, CAbstractTimelineRowUI *parentUiRow);
    ~CBaseStateRowUI();

    CControl *GetColorControl() override;
    CControl *GetTreeControl() override;
    CControl *GetToggleControl() override;
    CControl *GetTimebarControl() override;

    CBaseTimebarlessRow *GetTimebar() const;

    void SetFocus() override;
    void SetNameReadOnly(bool inReadOnly) override;

    void OnChildVisibilityChanged() override;
    void SelectKeysInRect(CRct inRect, bool inModifierKeyDown,
                          bool inGlobalCommitSelectionFlag) override;
    void PopulateSnappingList(CSnapper *inSnappingList) override;

    void DoStartDrag(CControlWindowListener *inWndListener) override;
    void AcceptDropAfter(bool inAccept) override;
    void AcceptDropBefore(bool inAccept) override;
    void SetDropTarget(CDropTarget *inDropTarget) override;

    void SetEnabled(bool inEnabled) override;
    void UpdateActionStatus() override;

    virtual void OnMouseOver();
    virtual void OnMouseOut();
    virtual void OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual void OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags);

private Q_SLOTS:
    void Initialize() override;
    void setTimeRatio(double inTimeRatio);
    void handleDirtyChange(bool dirty);
    void handleRowAdded(CBaseStateRow *row);
    void handlePropertyRowAdded(CPropertyRow *row);
    void handleChildrenLoaded();
    void handleExpanded(bool expanded);
    void handleVisibleChanged(bool visible);
    void handleHasChildrenChanged(bool hasChildren);
    void handleRowAboutToBeRemoved(CTimelineRow *row);
    void handleSelectAllKeys();
    void handleSelectedChanged(bool selected);
    void handleAddRowToUILists(CTimelineRow *inRow, CTimelineRow *inNextRow, CFilter &inFilter);
    void handleRefreshRowMetaDataRequest();

protected:
    void connectBackend();

    virtual CBlankToggleControl *CreateToggleControl();
    virtual CBaseTimebarlessRow *CreateTimebarRow() = 0;

    bool initialized() const;

    CListLayout m_ColorList;
    CListLayout m_TreeList;
    CListLayout m_ToggleList;
    CListLayout m_TimebarList;

    CBaseTimelineTreeControl *m_TreeControl;
    CColorControl *m_ColorControl;
    CBlankToggleControl *m_ToggleControl;
    CBaseTimebarlessRow *m_TimebarControl;
    CBaseStateRow *m_baseStateRow;

    bool m_Highlighted;
};

#endif // BASESTATEROWUI_H
