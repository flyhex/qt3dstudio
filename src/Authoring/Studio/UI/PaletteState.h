/****************************************************************************
**
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

#ifndef INCLUDED_PALETTE_STATE_H
#define INCLUDED_PALETTE_STATE_H 1

#pragma once

#include "Pt.h"

class CPaletteState
{
public:
    CPaletteState(Q3DStudio::CString inWindowName);
    virtual ~CPaletteState();

    bool IsMaximized() const { return m_Maximized; }
    void SetMaximized(bool inMaximized) { m_Maximized = inMaximized; }

    CPt GetPosition() const;
    void SetPosition(CPt inPosition);

    CPt GetSize() const;
    void SetSize(CPt inSize);

    bool IsVisible() const;
    void SetVisible(bool inIsVisible);

    void SaveState() const;
    void LoadState();

    void RestoreDefaults();

protected:
    CPt GetDisplaySize() const;

    Q3DStudio::CString m_WindowName;
    bool m_Maximized;
    CPt m_Position;
    CPt m_Size;
    bool m_IsVisible;
};

#endif // INCLUDED_PALETTE_STATE_H
