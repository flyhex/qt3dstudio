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
#ifndef INCLUDED_TIMEBAR_TIP
#define INCLUDED_TIMEBAR_TIP 1
#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Control.h"
#include "Snapper.h"

#include <QCursor>
#include <QPixmap>
//==============================================================================
//	Forwards
//==============================================================================
class CTimebarControl;
class CRenderer;

//==============================================================================
/**
 * Class for the tips of timebar controls.  Allows the user to resize timebars
 * by grabbing the tips.
 */
class CTimebarTip : public CControl
{
public:
    CTimebarTip(CTimebarControl *inTimebarControl, bool inIsLeft, bool inHasHandle = false);
    virtual ~CTimebarTip();

    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void Draw(CRenderer *inRenderer) override;

    void ShowHandles(bool inShowHandles);

protected:
    void RefreshToolTip(CPt inPoint);

    CTimebarControl *m_Timebar;
    bool m_IsMouseDown;
    bool m_MaybeDragStart;
    CPt m_MouseDownLoc;
    bool m_IsLeft;
    bool m_HasHandle;
    QPixmap m_HandleImage;
    QPixmap m_HandleDisabledImage;
    CSnapper m_Snapper;
};

#endif // INCLUDED_TIMEBAR_TIP
