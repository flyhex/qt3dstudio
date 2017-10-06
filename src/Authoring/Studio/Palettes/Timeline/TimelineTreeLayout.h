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

//=============================================================================
// Prefix
//=============================================================================
#ifndef INCLUDED_TIMELINE_TREE_LAYOUT_H
#define INCLUDED_TIMELINE_TREE_LAYOUT_H 1

#pragma once

//=============================================================================
// Includes
//=============================================================================
#include "Control.h"
#include "TimelineFilter.h"
#include "Scroller.h"
#include <vector>

//=============================================================================
// Forwards
//=============================================================================
class CFlowLayout;
class CScroller;
class CFilterToolbar;
class CTimelineRow;
class CToggleToolbar;
class CTimeToolbar;
class CTimelineControl;
class IDoc;
class CRenderer;
class CToggleBlankControl;
class CColorBlankControl;
class CTreeBlankControl;

//=============================================================================
/**
 * Class for tree control on the timeline palette.
 */
class CTimelineTreeLayout : public CControl, public CScrollListener
{
    typedef std::vector<CTimelineRow *> TTimelineRowList;

public:
    CTimelineTreeLayout(CTimelineControl *inTimelineControl, IDoc *inDoc);
    virtual ~CTimelineTreeLayout();

    // CControl
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void SetSize(CPt inSize) override;

    virtual void AddRow(CTimelineRow *inRow);

    /// Returns a filter object so that filter preferences can be set.  You must call Filter() in
    /// order to apply the filters once you make your changes.
    CFilter *GetFilter() { return &m_Filter; }
    void Filter();

    void OnScroll(CScroller *inScroller, CPt inScrollAmount) override;
    void SetScrollPositionY(CScroller *inSource, long inPositionY, bool inAbsolute = true);

    void ClearRows();

    void SetTime(long inTime);
    long GetTime();
    void ResetFilter();
    CRct GetVisibleArea();
    void RecalcLayout();

protected:
    CFilter m_Filter;
    CFilterToolbar
        *m_FilterToolbar; ///< Control at the top of the timeline containing filter buttons.
    CTimeToolbar *m_TimeToolbar; ///< Control at the top containing the time display
    CToggleToolbar
        *m_ToggleToolbar; ///< Control at the top containing a header for the toggle column.

    CScroller *m_ColorScroller;
    CFlowLayout *m_ColorList;
    CColorBlankControl *m_ColorBlankControl;

    CScroller *m_ToggleScroller;
    CFlowLayout *m_ToggleList;
    CToggleBlankControl *m_ToggleBlankControl;

    CScroller *m_TreeScroller;
    CFlowLayout *m_TreeList;
    CTreeBlankControl *m_TreeBlankControl;

    TTimelineRowList m_Rows;
    CTimelineControl *m_TimelineControl; ///< Parent control of this control

    bool
        m_IsScrolling; ///< Flag to not process onScroll that was triggered from a previous onScroll
};
#endif // INCLUDED_TIMELINE_TREE_LAYOUT_H
