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

#ifndef CABSTRACTTIMELINEROWUI_H
#define CABSTRACTTIMELINEROWUI_H

#include <QObject>

class CTimelineRow;
class ITimelineItemBinding;
class ITimelineControl;
class ISnappingListProvider;

class CControl;
class CControlWindowListener;
class CDropTarget;
class CRct;
class CSnapper;

class CAbstractTimelineRowUI : public QObject
{
    Q_OBJECT
public:
    CAbstractTimelineRowUI(CTimelineRow *timelineRow, CAbstractTimelineRowUI *parentUiRow);
    virtual ~CAbstractTimelineRowUI();

    void SetParentRow(CAbstractTimelineRowUI *parentUiRow);

    CTimelineRow *GetTimelineRow() const;

    virtual CControl *GetColorControl() = 0;
    virtual CControl *GetTreeControl() = 0;
    virtual CControl *GetToggleControl() = 0;
    virtual CControl *GetTimebarControl() = 0;

    virtual void SetTimelineControl(ITimelineControl *inTimelineControl);
    virtual ITimelineControl *GetTopControl() const;

    virtual void SetSnappingListProvider(ISnappingListProvider *inProvider) = 0;
    virtual ISnappingListProvider *GetSnappingListProvider() const = 0;

    virtual void SetIndent(long indent);
    virtual long GetIndent() const;

    virtual void SetEnabled(bool inEnabled) = 0;
    virtual void SetFocus() = 0;
    virtual void SetNameReadOnly(bool inReadOnly) = 0;

    virtual void OnChildVisibilityChanged() = 0;
    virtual void SelectKeysInRect(CRct inRect, bool inModifierKeyDown,
                                  bool inGlobalCommitSelectionFlag) = 0;
    virtual void PopulateSnappingList(CSnapper *inSnappingList) = 0;

    virtual void DoStartDrag(CControlWindowListener *inWndListener) = 0;
    virtual void AcceptDropAfter(bool inAccept) = 0;
    virtual void AcceptDropBefore(bool inAccept) = 0;
    virtual void SetDropTarget(CDropTarget *inDropTarget) = 0;

    virtual void UpdateActionStatus() = 0;

public Q_SLOTS:
    virtual void Initialize() = 0;

protected:
    CTimelineRow *m_timelineRow = nullptr;
    CAbstractTimelineRowUI *m_parentRowUI = nullptr;
    ITimelineControl *m_TimelineControl = nullptr;

    long m_Indent;
};

#endif // CABSTRACTTIMELINEROWUI_H
