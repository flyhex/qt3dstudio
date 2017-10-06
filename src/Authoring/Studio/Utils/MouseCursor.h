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

//=============================================================================
// Prefix
//=============================================================================
#ifndef INCLUDED_MOUSE_CURSOR_H
#define INCLUDED_MOUSE_CURSOR_H 1

#pragma once

#include <QCursor>


//=============================================================================
// Includes
//=============================================================================

//=============================================================================
/**
 * Cross-platform cursor class.
 */
class CMouseCursor
{
public:
    typedef long TUICMouseCursor;

    static const TUICMouseCursor CURSOR_ARROW;
    static const TUICMouseCursor CURSOR_WAIT;
    static const TUICMouseCursor CURSOR_RESIZE_LEFTRIGHT;
    static const TUICMouseCursor CURSOR_RESIZE_UPDOWN;
    static const TUICMouseCursor CURSOR_GROUP_MOVE;
    static const TUICMouseCursor CURSOR_GROUP_ROTATE;
    static const TUICMouseCursor CURSOR_GROUP_SCALE;
    static const TUICMouseCursor CURSOR_ITEM_MOVE;
    static const TUICMouseCursor CURSOR_ITEM_ROTATE;
    static const TUICMouseCursor CURSOR_ITEM_SCALE;
    static const TUICMouseCursor CURSOR_EDIT_CAMERA_PAN;
    static const TUICMouseCursor CURSOR_EDIT_CAMERA_ROTATE;
    static const TUICMouseCursor CURSOR_EDIT_CAMERA_ZOOM;
    static const TUICMouseCursor CURSOR_BLANK;
    static const TUICMouseCursor CURSOR_DROP_INVALID;
    static const TUICMouseCursor CURSOR_DROP_MOVE;
    static const TUICMouseCursor CURSOR_DROP_COPY;
    static const TUICMouseCursor CURSOR_IBEAM;

    CMouseCursor();
    virtual ~CMouseCursor();
    QCursor GetHandle();
    void Show();
    void Hide();
    void SetCursorPos(long inXPos, long inYPos);
    bool Load(TUICMouseCursor inCursor);

protected:
    void Destroy();

    QCursor m_Handle;

private:
    bool m_IsThemeCursor;
    short m_ThemeCursor;
};

#endif // INCLUDED_MOUSE_CURSOR_H
