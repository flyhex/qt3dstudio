/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#ifndef INCLUDED_PLAYHEAD_H
#define INCLUDED_PLAYHEAD_H 1

#pragma once

#include "OverlayControl.h"
#include "Snapper.h"

#include <QPixmap>

class CTimelineTimelineLayout;
class IDoc;

class CPlayhead : public COverlayControl
{
public:
    CPlayhead(CTimelineTimelineLayout *inTimeline, IDoc *inDoc);
    virtual ~CPlayhead();

    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void SetSize(CPt inSize) override;
    void Draw(CRenderer *inRenderer) override;
    void SetMinMaxPosition(long inMinimumPosition, long inMaximumPosition);
    long GetCurrentTime();
    bool HitTest(const CPt &inPoint) const override;
    void StepRightSmall();
    void StepRightLarge();
    void StepLeftSmall();
    void StepLeftLarge();

    long GetCenterOffset() const { return m_LinePos; }
    bool IsMouseDown() const { return m_IsMouseDown; }

    void UpdateTime(long inTime, bool inUpdateClient);

protected:
    QPixmap m_PlayheadImage;
    bool m_IsMouseDown;
    long m_InitialOffset;

    long m_MinimumPosition;
    long m_MaximumPosition;
    long m_LinePos;

    CTimelineTimelineLayout *m_Timeline;

    CSnapper m_Snapper;
    IDoc *m_Doc;
};
#endif // INCLUDED_PLAYHEAD_H
