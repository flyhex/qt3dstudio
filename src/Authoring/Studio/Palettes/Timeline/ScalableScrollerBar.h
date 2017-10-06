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

#ifndef INCLUDED_SCALABLE_SCROLLER_BAR_H
#define INCLUDED_SCALABLE_SCROLLER_BAR_H 1

#pragma once

#include "ScrollerThumb.h"
#include "ScrollerBar.h"

#include <QCursor>

class CScalableBar;
class CScalableScroller;
class CScalableThumb;

class CScalingListener
{
public:
    virtual void OnScalingLeft(long inLength, long inTotalLength, long inOffset) = 0;
    virtual void OnScalingRight(long inLength, long inTotalLength, long inOffset) = 0;
    virtual void OnScalingReset() = 0;
};

class CScalableThumbTab : public CControl
{
public:
    CScalableThumbTab(CScalableThumb *inThumb, bool inIsRightTab, CScalableBar *inBar);
    virtual ~CScalableThumbTab();

    void Draw(CRenderer *inRenderer) override;

    void OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;

protected:
    CScalableThumb *m_Thumb;
    CScalableBar *m_Bar;
    bool m_IsMouseDown;
    bool m_IsRightTab;
    CPt m_MouseDownLoc;
};

class CScalableThumb : public CScrollerThumb
{
public:
    CScalableThumb(CScalableBar *inScrollerBar);
    virtual ~CScalableThumb();

    void SetSize(CPt inSize) override;
    bool OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags) override;

protected:
    CScalableBar *m_ScrollerBar;
    CScalableThumbTab m_LeftTab;
    CScalableThumbTab m_RightTab;
};

class CScalableBar : public CScrollerBar
{
public:
    CScalableBar(CScalableScroller *inScroller);
    virtual ~CScalableBar();

    void SetEnabled(bool inIsEnabled) override;

    void SetScalingListener(CScalingListener *inListener);

    void OnScalingRight(long inAmount);
    void OnScalingLeft(long inAmount);
    void OnScalingReset();

protected:
    CControl *CreateThumb() override;

    CScalableScroller *m_ScalableScroller;

    CScalingListener *m_Listener;
    CScalableThumb *m_Thumb;
};

#endif // INCLUDED_SCALABLE_SCROLLER_BAR_H
