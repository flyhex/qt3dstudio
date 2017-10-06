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
#ifndef INCLUDED_SCROLLER_THUMB_H
#define INCLUDED_SCROLLER_THUMB_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Control.h"
#include "FlowLayout.h"

//==============================================================================
//	Forwards
//==============================================================================
class IScrollerBar;
class CRenderer;

//=============================================================================
/**
 * Class for creating the thumb portion of a scroller.
 */
class CScrollerThumb : public CControl
{
public:
    static const long MIN_LENGTH;

    CScrollerThumb(IScrollerBar *inScroller);
    virtual ~CScrollerThumb();

    virtual void Draw(CRenderer *inRenderer);

    virtual bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags);

    virtual CPt GetMinimumSize();

protected:
    IScrollerBar *m_ScrollerBar;
    bool m_MouseDown;
    CPt m_MouseDownPoint;
};

#endif // INCLUDED_SCROLLER_THUMB_H