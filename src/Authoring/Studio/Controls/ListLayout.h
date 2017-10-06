/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#ifndef INCLUDED_LIST_LAYOUT_H
#define INCLUDED_LIST_LAYOUT_H 1

#pragma once

#include "Control.h"

class CListLayout : public CControl
{
public:
    CListLayout(bool inAlignChildrenLength = false);
    virtual ~CListLayout();

    void SetSize(CPt inSize) override;

    void AddChild(CControl *inChild, CControl *inInsertBefore = nullptr) override;
    void RemoveChild(CControl *inChild) override;

    void OnChildSizeChanged(CControl *inControl) override;

    void RecalcLayout();

#ifdef _DEBUG
    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
#endif

protected:
    bool m_IsResizing;
    bool m_AlignChildrenLength; ///< True when all children in this list should align up
};
#endif // INCLUDED_LIST_LAYOUT_H
