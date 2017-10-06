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
#ifndef INCLUDED_SCROLLER_BAR_H
#define INCLUDED_SCROLLER_BAR_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================

#include "Control.h"
#include "FlowLayout.h"

//==============================================================================
//	Forwards
//==============================================================================

class CScroller;
class CScrollerBar;
class CScrollerBackground;
class CRenderer;
class CScrollerButtonControl;

class IScroller;

class IScrollerBar
{
protected:
    virtual ~IScrollerBar() {}
public:
    enum EOrientation {
        VERTICAL,
        HORIZONTAL,
    };
    virtual EOrientation GetOrientation() = 0;
    virtual void RepositionThumb() = 0;
    virtual IScroller *GetScroller() = 0;
    virtual void SetBarPosition(long inPosition) = 0;
    virtual long GetBarPosition() = 0;
    virtual CControl *GetThumb() = 0;
};

//=============================================================================
/**
 * Class for creating a scrollerbar (contains a CScrollerThumb).
 */
class CScrollerBar : public CFlowLayout, public IScrollerBar
{
public:
    static const long DEFAULT_WIDTH;
    static const ::CColor DEFAULT_COLOR;
    static const ::CColor SCROLLER_TOP;

    CScrollerBar(CScroller *inScroller, bool inCreateImmediately = true);
    virtual ~CScrollerBar();

    DEFINE_OBJECT_COUNTER(CScrollerBar)

    void SetOrientation(EOrientation inOrientation);
    EOrientation GetOrientation() override;

    CPt GetMinimumSize() override;

    void SetScrollerThumbPosition(long inPosition);
    long GetScrollerThumbPosition();

    void SetSize(CPt inSize) override;

    void SetBarPosition(long inPosition) override;
    long GetBarPosition() override;

    CControl *GetThumb() override;
    virtual CControl *GetThumbBackground();
    void RepositionThumb() override;
    IScroller *GetScroller() override;

protected:
    virtual CControl *CreateThumb();
    void Initialize();
    Q3DStudio::CAutoMemPtr<CScrollerButtonControl> m_ButtonBackward;
    Q3DStudio::CAutoMemPtr<CScrollerButtonControl> m_ButtonForward;
    Q3DStudio::CAutoMemPtr<CControl> m_ScrollerThumb;
    Q3DStudio::CAutoMemPtr<CScrollerBackground> m_Background;

    EOrientation m_Orientation;
    CScroller *m_Scroller;
};

#endif // INCLUDED_SCROLLER_BAR_H
