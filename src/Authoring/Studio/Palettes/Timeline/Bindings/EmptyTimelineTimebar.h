/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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
#pragma once

#include "ITimelineTimebar.h"

//=============================================================================
/**
 * The current timeline UI design is such that even when no timebar shows up ( with the exception of
 * the top row, ie the time context )
 * there is a timebar control to store the keyframes for the animated properties.
 * Hence, instead of return nullptr for GetTimebar for ITimelineItem, this class will ensure the UI
 * classes still work.
 */
class CEmptyTimelineTimebar : public ITimelineTimebar
{
public:
    CEmptyTimelineTimebar();
    virtual ~CEmptyTimelineTimebar();

    // ITimelineTimebar
    long GetStartTime() const override;
    long GetEndTime() const override;
    long GetDuration() const override;
    bool ShowHandleBars() const override;
    void OnBeginDrag() override;
    void OffsetTime(long inDiff) override;
    void ChangeTime(long inTime, bool inSetStart) override;
    void CommitTimeChange() override;
    void RollbackTimeChange() override;
    ::CColor GetTimebarColor() override;
    void SetTimebarColor(const ::CColor &inColor) override;
    Q3DStudio::CString GetTimebarComment() override;
    void SetTimebarComment(const Q3DStudio::CString &inComment) override;
    void SetTimebarTime(ITimeChangeCallback *inCallback = nullptr) override;
};
