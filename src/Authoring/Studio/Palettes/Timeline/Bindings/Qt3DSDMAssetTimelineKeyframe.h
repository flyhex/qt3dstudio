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
#ifndef QT3DSDM_ASSET_KEYFRAME_H
#define QT3DSDM_ASSET_KEYFRAME_H 1

#pragma once

#include "IKeyframe.h"

// Data model specific
#include "Qt3DSDMHandles.h"

class Qt3DSDMTimelineItemBinding;

//==============================================================================
/**
 *	Represents a keyframe displayed for a Asset( e.g. material ), for all keyframes (of the
 *animated properties) at time t.
 */
//==============================================================================
class Qt3DSDMAssetTimelineKeyframe : public IKeyframe
{
protected:
    Qt3DSDMTimelineItemBinding *m_OwningBinding;
    long m_Time;
    bool m_Selected;

public:
    Qt3DSDMAssetTimelineKeyframe(Qt3DSDMTimelineItemBinding *inOwningBinding, long inTime);
    virtual ~Qt3DSDMAssetTimelineKeyframe();

    // IKeyframe
    bool IsSelected() const override;
    long GetTime() const override;
    void SetTime(const long inNewTime) override;
    void SetDynamic(bool inIsDynamic) override;
    void setUI(Keyframe *kfUI) override;
    bool IsDynamic() const override;

    void SetSelected(bool inSelected);
    void UpdateTime(const long inTime) { m_Time = inTime; }

private:
    Keyframe *m_ui;
};

#endif // QT3DSDM_ASSET_KEYFRAME_H
