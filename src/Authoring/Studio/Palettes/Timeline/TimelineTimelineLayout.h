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
#ifndef INCLUDED_TIMELINE_TIMELINE_LAYOUT_H
#define INCLUDED_TIMELINE_TIMELINE_LAYOUT_H 1

#pragma once

//=============================================================================
// Includes
//=============================================================================
#include "Control.h"
#include <vector>
#include "ScalableScrollerBar.h"
#include "Scroller.h"
#include "Pt.h"
#include "Snapper.h"
#include "AreaBoundingRect.h"
#include "Playhead.h"
#include "Qt3DSDMHandles.h"

#include "TimelineRow.h"

//=============================================================================
// Forwards
//=============================================================================
class CScalableScroller;
class CFlowLayout;
class CTimelineControl;
class CSnapper;
class IDoc;
class CHotKeys;
class CTimeMeasure;
class CPlayhead;

//=============================================================================
/**
 * Right-hand pane of the Timeline containing timebars, keyframes, etc.
 */
class CTimelineTimelineLayout : public QObject,
                                public CControl,
                                public CScalingListener,
                                public CScrollListener,
                                public ISnappingListProvider
{
    Q_OBJECT

    typedef std::vector<CTimelineRow *> TTimelineRowList;
    typedef std::map<qt3dsdm::Qt3DSDMSlideHandle, double> TSlideRatioMap;

public:
    static const long END_BUFFER_SIZE = 20;
    static const double SCALING_PERCENTAGE_INC;
    static const double SCALING_PERCENTAGE_DEC;
    static const double MAX_ZOOM_OUT;

    CTimelineTimelineLayout(CTimelineControl *inView, IDoc *inDoc);
    virtual ~CTimelineTimelineLayout();

    void SetSize(CPt inSize) override;

    void AddRow(CTimelineRow *inRow);

    void OnScalingRight(long inLength, long inTotalLength, long inOffset) override;
    void OnScalingLeft(long inLength, long inTotalLength, long inOffset) override;
    void OnScalingReset() override;

    long ScrollLayout(long inAmount);

    void Filter();

    void OnScroll(CScroller *inScroller, CPt inScrollAmount) override;
    void SetScrollPositionY(CScroller *inSource, long inPositionY, bool inAbsolute = true);

    void ClearRows();

    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseWheel(CPt inPoint, long inAmount, Qt::KeyboardModifiers inFlags) override;

    CScalableScroller *GetScroller();
    CPlayhead *GetPlayhead();
    CTimeMeasure *GetTimeMeasure();
    void RecalcTime(bool inUpdateClient, long inFlags);
    void SetTime(long inTime, bool inIsSecondary = false);
    void OnTimeMeasureMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags);
    long GetViewTimeOffset();
    void RecalcLayout();
    void RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler, QWidget *actionParent);
    void SetTimeRatio(double inTimeRatio);
    void OnTimelineLayoutChanged();

    void DeleteTimelineRatio(qt3dsdm::Qt3DSDMSlideHandle inSlide);
    void ClearAllTimeRatios();
    double GetTimelineRatio(qt3dsdm::Qt3DSDMSlideHandle inSlide);

    // ISnappingListProvider
    void PopulateSnappingList(CSnapper *inSnappingList) override;

protected:
    void SetTimelineRatio(qt3dsdm::Qt3DSDMSlideHandle inSlide, double inRatio);

    long GetMaximumTimebarTime();
    void OnScalingZoomIn();
    void OnScalingZoomOut();
    void CenterToPlayhead();

    bool m_CommitKeyframeSelection; ///< flag for saving previous keyframe selection when the mouse
                                    ///is released.
    CTimelineControl *m_TimelineControl;
    CTimeMeasure *m_TimeMeasure;
    CScalableScroller *m_Scroller;
    CFlowLayout *m_TimebarList;
    double m_TimeRatio = DEFAULT_TIME_RATIO;

    TTimelineRowList m_Rows;
    CPlayhead m_Playhead;
    CSnapper m_Snapper;
    bool m_IsLayoutChanged; ///< flag to keep track of a need for a delayed RecalcLayout

    bool m_IsMouseDown;
    CPt m_DragBeginPoint;
    CAreaBoundingRect *m_BoundingRect;

    TSlideRatioMap m_TimelineRatio; ///< stores the time zooming ratios for each slide
};
#endif // INCLUDED_TIMELINE_TIMELINE_LAYOUT_H
