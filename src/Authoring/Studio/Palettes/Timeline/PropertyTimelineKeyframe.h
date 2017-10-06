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

#ifndef INCLUDED_PROPERTY_TIMELINE_KEYFRAME
#define INCLUDED_PROPERTY_TIMELINE_KEYFRAME 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Control.h"
#include "TimelineKeyframe.h"
#include "Snapper.h"

#include <QPixmap>

//==============================================================================
//	Forwards
//==============================================================================
class CRenderer;
class CPropertyTimebarRow;
class ITimelineItemProperty;

class CPropertyTimelineKeyframe : public CControl, public CTimelineKeyframe
{

public:
    CPropertyTimelineKeyframe(CPropertyTimebarRow *inParentRow, double inTimeRatio);
    ~CPropertyTimelineKeyframe();
    void Draw(CRenderer *inRenderer) override;
    QPixmap GetImage();

    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void Select(bool inState);
    void SetTimeRatio(double inTimeRatio);
    bool OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool IsSelected();
    void SetRectOverHandled(bool inState);
    bool GetRectOverHandled();
    void SetPreviousSelectState(bool inState);
    bool GetPreviousSelectState();

protected:
    void RefreshToolTip(CPt inPoint);
    ITimelineItemProperty *GetProperty() const;

protected:
    bool m_Selected;
    bool m_RectOverHandled; ///< Indicates if the mouse rect over has been handled.
    bool m_PreviousSelectState; ///< Stores the previous select state for the keyframe.
    CPropertyTimebarRow *m_ParentRow;
    bool m_IsMouseDown;
    CPt m_MouseDownLoc; ///< Location of the mouse after an OnMouseDownEvent, in client coordinates
    bool m_IsDragging; ///< Indicates whether or not the keyframe is currently being dragged,
                       ///determined by the pixel buffer
    double m_TimeRatio;
    CSnapper m_Snapper;
    QPixmap m_Icon;
    QPixmap m_DisabledIcon;
    QPixmap m_SelectedIcon;
    QPixmap m_DynamicIcon;
    QPixmap m_DynamicSelectedIcon;
};

#endif // INCLUDED_PROPERTY_TIMELINE_KEYFRAME
