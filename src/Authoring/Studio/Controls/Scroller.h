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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef INCLUDED_SCROLLER_H
#define INCLUDED_SCROLLER_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Control.h"
#include "ScrollerBar.h"
#include "FlowLayout.h"
#include "GenericFunctor.h"
#include "Multicaster.h"
#include "ITickTock.h"

//==============================================================================
//	Forwards
//==============================================================================
class CButtonControl;

GENERIC_FUNCTOR_2(CScrollListener, OnScroll, CScroller *, CPt);

class IScroller
{
protected:
    virtual ~IScroller() {}
public:
    virtual void SetVisiblePosition(CPt inVisiblePosition) = 0;
    virtual CPt GetVisiblePosition() = 0;
};

//=============================================================================
/**
 * Class for creating a scroller.
 */
class CScroller : public CControl, public IScroller
{
public:
    enum EScrollMode {
        NEVER,
        AS_NEEDED,
        ALWAYS,
    };

    CScroller(bool inCreateImmediately = true);
    virtual ~CScroller();

    void Initialize();

    void OnDraw(CRenderer *inRenderer, CRct &inDirtyRect,
                        bool inIgnoreValidation /* = false */) override;
    virtual void DrawBackground(CRenderer *) {}
    void Draw(CRenderer *inRenderer) override;

    void SetSize(CPt inSize) override;
    void SetLayout(CPt inSize, CPt inPosition) override;
    CPt GetMinimumSize() override;
    CPt GetMaximumSize() override;

    void AddChild(CControl *inControl, CControl *inInsertBefore = nullptr) override;

    CPt GetVisibleSize();
    CPt GetContaineeSize();

    CPt GetVisiblePosition() override;
    void SetVisiblePosition(CPt inVisiblePosition) override;
    CPt GetMaxVisiblePosition();

    CScrollerBar *GetHorizontalBar();
    CScrollerBar *GetVerticalBar();

    void SetVerticalScrollMode(EScrollMode inScrollMode);
    EScrollMode GetVerticalScrollMode();

    void SetHorizontalScrollMode(EScrollMode inScrollMode);
    EScrollMode GetHorizontalScrollMode();

    virtual void RecalcLayout();
    void OnChildSizeChanged(CControl *inChild) override;

    void AddScrollListener(CScrollListener *inScrollListener);
    void RemoveScrollListener(CScrollListener *inScrollListener);

    void SetAdditionalClippingRect(CRct inClippingRect);

    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseWheel(CPt inPoint, long inScrollAmount, Qt::KeyboardModifiers inFlags) override;
    void EnsureVisible(CRct inRect) override;

    virtual void OnTimer();
    void OnLoseFocus() override;
    void AdjustDelayTimeAccordingToOnTimerTime(unsigned long inTime);

protected:
    void OnSizeChanged(CPt inSize) override;
    virtual bool IsVerticalVisible();
    virtual bool IsHorizontalVisible();
    virtual bool IsVerticalScrolling();
    virtual bool IsHorizontalScrolling();

    virtual CScrollerBar *CreateVerticalBar();
    virtual CScrollerBar *CreateHorizontalBar();

    virtual void SetVisibleSize(CPt inSize);

    virtual CControl *GetControl();

    CScrollerBar *m_VerticalBar;
    CScrollerBar *m_HorizontalBar;

    CPt m_VisibleSize;
    CPt m_VisiblePosition;
    CPt m_MaxVisiblePosition;

    CPt m_ScrolledAmount;
    CPt m_ChildOffset;

    CRct m_AddtlClippingRect;
    CPt m_ScrollingDir;
    CPt m_MousePos;
    Qt::KeyboardModifiers m_MouseFlags;

    EScrollMode m_VerticalScrollMode;
    EScrollMode m_HorizontalScrollMode;

    bool m_ResizingChildren;

    CMulticaster<CScrollListener *> m_ScrollListeners;

    bool m_IsMouseDown;
    CPt m_PrevMousePoint;

    long m_OffsetAmmount;
    unsigned long m_DelayTime;
    std::shared_ptr<UICDM::ISignalConnection> m_CurrentTickTock;
};
#endif // INCLUDED_SCROLLER_H
