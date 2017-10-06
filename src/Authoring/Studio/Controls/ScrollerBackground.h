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

#ifndef INCLUDED_SCROLLER_BACKGROUND_H
#define INCLUDED_SCROLLER_BACKGROUND_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "BlankControl.h"
#include "StudioPreferences.h"
#include "ScrollerBar.h"
#include "ScrollController.h"

//==============================================================================
//	Forwards
//==============================================================================
class CRenderer;

//=============================================================================
/**
 * Extends the blank control to draw items specific to the scroller control.
 */
class CScrollerBackground : public CBlankControl, public CScrollController
{
protected:
    CPt m_MousePos;

public:
    CScrollerBackground(CColor inColor = CStudioPreferences::GetScrollBGColor());
    virtual ~CScrollerBackground();
    virtual void Draw(CRenderer *inRenderer);
    virtual bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags);
    virtual void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags);

protected:
    virtual long DetermineScrollAmount();
};

#endif // INCLUDED_SCROLLER_BACKGROUND_H