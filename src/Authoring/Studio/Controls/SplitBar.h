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
#ifndef INCLUDED_SPLIT_BAR_H
#define INCLUDED_SPLIT_BAR_H 1

#pragma once

//=============================================================================
// Includes
//=============================================================================
#include "Control.h"

#include <QCursor>

//=============================================================================
// Forwards
//=============================================================================

//=============================================================================
/**
 * Abstract class for implementing a splitter control
 */
class CSplitterBase
{
public:
    enum ESplitDirection {
        SPLIT_VERTICAL,
        SPLIT_HORIZONTAL,
    };

    virtual void SetSplitLocation(long inPixels) = 0;
    virtual long GetSplitLocation() const = 0;
    virtual ESplitDirection GetSplitDirection() const = 0;
};

//=============================================================================
/**
 * Defines the bar used in a splitter control.  Handles drawing and cursors for
 * the bar.
 */
class CSplitBar : public CControl
{
public:
    static const long DEFAULT_WIDTH;

    CSplitBar(CSplitterBase *inSplitter, long inWidth = DEFAULT_WIDTH);
    virtual ~CSplitBar();

    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseRUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags) override;

    void Draw(CRenderer *inRenderer) override;

    virtual long GetWidth();
    virtual void SetWidth(long inWidth);

protected:
    CSplitterBase *m_Splitter;
    long m_Width;

    bool m_MouseDown;
    CPt m_MouseDownPoint;
};

#endif // INCLUDED_SPLIT_BAR_H
