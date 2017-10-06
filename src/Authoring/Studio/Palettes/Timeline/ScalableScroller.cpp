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

#include "stdafx.h"

#include "ScalableScroller.h"
#include "ScalableScrollerBar.h"

CScalableScroller::CScalableScroller()
    : CScroller(false)
    , m_ScalableBar(nullptr)
    , m_ScalingListener(nullptr)
{
    Initialize();
}

CScalableScroller::~CScalableScroller()
{
}

CScrollerBar *CScalableScroller::CreateHorizontalBar()
{
    if (m_ScalableBar == nullptr) {
        m_ScalableBar = new CScalableBar(this);
        m_ScalableBar->SetOrientation(CScrollerBar::HORIZONTAL);
        m_ScalableBar->SetScalingListener(m_ScalingListener);
    }
    return m_ScalableBar;
}

void CScalableScroller::SetScalingListener(CScalingListener *inListener)
{
    m_ScalingListener = inListener;

    if (m_ScalableBar != nullptr) {
        m_ScalableBar->SetScalingListener(inListener);
    }
}

//====================================================================
/**
 * Protected function for seting the display size of the scroller.
 * Overridden because the scaleable scroller thumb at the bottom of
 * the timeline was getting messed up.  Yes, it's a hack, but I couldn't
 * fix it any other way.
 */
void CScalableScroller::SetVisibleSize(CPt inSize)
{
    m_VisibleSize = inSize;

    // Don't call the base class because it messes things up
}
