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

#include "Qt3DSCommonPrecompile.h"
#include "MouseCursor.h"
#include <QtGui/qpixmap.h>

const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_ARROW = 0; // IDC_ARROW
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_WAIT = 1; // IDC_WAIT
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_RESIZE_LEFTRIGHT = 2; // IDC_SIZEWE
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_RESIZE_UPDOWN = 3; // IDC_SIZENS
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_GROUP_MOVE = 4; // IDC_GROUP_MOVE
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_GROUP_ROTATE = 5; // IDC_GROUP_ROTATE
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_GROUP_SCALE = 6; // IDC_GROUP_SCALE
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_ITEM_MOVE = 7; // IDC_ITEM_MOVE
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_ITEM_ROTATE = 8; // IDC_ITEM_ROTATE
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_ITEM_SCALE = 9; // IDC_ITEM_SCALE
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_EDIT_CAMERA_PAN =
    10; // IDC_EDIT_CAMERA_PAN
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_EDIT_CAMERA_ROTATE =
    11; // IDC_EDIT_CAMERA_ROTATE
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_EDIT_CAMERA_ZOOM =
    12; // IDC_EDIT_CAMERA_ZOOM
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_BLANK = 13; // blank cursor
const CMouseCursor::Qt3DSMouseCursor CMouseCursor::CURSOR_IBEAM = 14;

//=============================================================================
/**
 * Constructor
 */
CMouseCursor::CMouseCursor()
    : m_IsThemeCursor(false)
    , m_ThemeCursor(-1)
{
}

//=============================================================================
/**
 * Destructor
 */
CMouseCursor::~CMouseCursor()
{
    Destroy();
}

//=============================================================================
/**
 * @return A platform-specific handle to the cursor, or nullptr if no cursor has been loaded
 */
QCursor CMouseCursor::GetHandle()
{
    return m_Handle;
}

//=============================================================================
/**
 * If a cursor has been loaded, this function will change the current cursor
 * to the newly loaded one.
 */
void CMouseCursor::Show()
{
    qWarning() << Q_FUNC_INFO << "QCursor doesn't work this way - set a breakpoint and debug caller";
}

//=============================================================================
/**
* Sets the cursor position
* @param inXPos x position of the cursor (in pixels)
* @param inYPos y position of the cursor (in pixels)
*/
void CMouseCursor::SetCursorPos(long inXPos, long inYPos)
{
    QCursor::setPos(QPoint(inXPos, inYPos));
}

//=============================================================================
/**
 * Releases the cursor if one has been loaded.  Called by the destructor.
 * WINDOWS IMPLEMENTATION
 */
void CMouseCursor::Destroy()
{
}

//=============================================================================
/**
 * Loads the specified cursor resource.  All cursors are expected to be
 * resources that are loaded from ".cur" files.  Some standard cursors are
 * defined by the system and can be found in the MSDN (IDC_ARROW for example).
 * WINDOWS IMPLEMENTATION
 *
 * @param inCursor ID of the cursor to be loaded
 * @return true if the cursor was successfully loaded, otherwise false
 */
bool CMouseCursor::Load(Qt3DSMouseCursor inCursor)
{
    // Convert from our cursors to Windows specific cursors
    switch (inCursor) {
    case CURSOR_ARROW:
        m_Handle = Qt::ArrowCursor;
        break;

    case CURSOR_WAIT:
        m_Handle = Qt::WaitCursor;
        break;

    case CURSOR_RESIZE_LEFTRIGHT:
        m_Handle = Qt::SizeHorCursor;
        break;

    case CURSOR_RESIZE_UPDOWN:
        m_Handle = Qt::SizeVerCursor;
        break;

    case CURSOR_GROUP_MOVE:
        m_Handle = QCursor(QPixmap(":/cursors/group_move.png"), 0, 0);
        break;

    case CURSOR_GROUP_ROTATE:
        m_Handle = QCursor(QPixmap(":/cursors/group_rotate.png"), 0, 0);
        break;

    case CURSOR_GROUP_SCALE:
        m_Handle = QCursor(QPixmap(":/cursors/group_scale.png"), 0, 0);
        break;

    case CURSOR_ITEM_MOVE:
        m_Handle = QCursor(QPixmap(":/cursors/item_move.png"), 0, 0);
        break;

    case CURSOR_ITEM_ROTATE:
        m_Handle = QCursor(QPixmap(":/cursors/item_rotate.png"), 0, 0);
        break;

    case CURSOR_ITEM_SCALE:
        m_Handle = QCursor(QPixmap(":/cursors/item_scale.png"), 0, 0);
        break;

    case CURSOR_EDIT_CAMERA_PAN:
        m_Handle = QCursor(QPixmap(":/cursors/edit_camera_pan.png"), 10, 10);
        break;

    case CURSOR_EDIT_CAMERA_ROTATE:
        m_Handle = QCursor(QPixmap(":/cursors/edit_camera_rot.png"), 8, 10);
        break;

    case CURSOR_EDIT_CAMERA_ZOOM:
        m_Handle = QCursor(QPixmap(":/cursors/edit_camera_zoom.png"), 8, 8);
        break;

    case CURSOR_BLANK:
        m_Handle = Qt::BlankCursor;
        break;
    case CURSOR_IBEAM:
        m_Handle = Qt::IBeamCursor;
        break;
    default:
        m_Handle = QCursor();
        break;
    }
    return true;
}
