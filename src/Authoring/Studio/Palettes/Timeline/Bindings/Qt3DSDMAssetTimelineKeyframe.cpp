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

#include "stdafx.h"
#include "Qt3DSDMAssetTimelineKeyframe.h"
#include "Qt3DSDMTimelineItemBinding.h"

using namespace qt3dsdm;

CUICDMAssetTimelineKeyframe::CUICDMAssetTimelineKeyframe(CUICDMTimelineItemBinding *inOwningBinding,
                                                         long inTime)
    : m_OwningBinding(inOwningBinding)
    , m_Time(inTime)
    , m_Selected(false)
{
}

CUICDMAssetTimelineKeyframe::~CUICDMAssetTimelineKeyframe()
{
}

bool CUICDMAssetTimelineKeyframe::IsSelected() const
{
    return m_Selected;
}

long CUICDMAssetTimelineKeyframe::GetTime() const
{
    return m_Time;
}

void CUICDMAssetTimelineKeyframe::SetTime(const long inNewTime)
{
    Q_UNUSED(inNewTime);
    // note: this is not used. because setting time is currently only done through offsetting by
    // moving keyframes OR using the edit time dialog.
    ASSERT(0);
}

void CUICDMAssetTimelineKeyframe::SetDynamic(bool inIsDynamic)
{
    m_OwningBinding->SetDynamicKeyframes(m_Time, inIsDynamic);
}

bool CUICDMAssetTimelineKeyframe::IsDynamic() const
{
    // return true if any of its property keyframes is dynamic
    return m_OwningBinding->HasDynamicKeyframes(m_Time);
}

void CUICDMAssetTimelineKeyframe::SetSelected(bool inSelected)
{
    m_Selected = inSelected;
}