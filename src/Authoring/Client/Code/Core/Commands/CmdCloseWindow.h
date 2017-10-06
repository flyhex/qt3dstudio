/****************************************************************************
**
** Copyright (C) 2002 Anark Corporation.
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
// Prefix
//==============================================================================
#ifndef INCLUDED_CMD_CLOSE_WINDOW_H
#define INCLUDED_CMD_CLOSE_WINDOW_H 1

#pragma once

//==============================================================================
// Includes
//==============================================================================
#include "Cmd.h"
#include "DialogControl.h"

//==============================================================================
// Forwards
//==============================================================================

//==============================================================================
// CCmd to close a modal window
//==============================================================================
class CCmdCloseWindow : public CCmd
{
public:
    CCmdCloseWindow(CDialogControl *inDialog)
        : m_Dialog(inDialog)
    {
        SetUndoable(false);
    }

    unsigned long Do() override
    {
        if (m_Dialog)
            m_Dialog->EndWindow();
        return 0;
    }

    unsigned long Undo() override { return 0; }

protected:
    CDialogControl *m_Dialog;
};

#endif // INCLUDED_CMD_CLOSE_WINDOW_H
