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

#ifndef INCLUDED_SPLIT_AGENT_H
#define INCLUDED_SPLIT_AGENT_H 1

#pragma once

#include "SplitBar.h"

class CSplitter : public CControl, public CSplitterBase
{
public:
    CSplitter();
    virtual ~CSplitter();

    virtual void SetSize(CPt inSize);
    virtual void SetLayout(CPt inSize, CPt inPosition);
    virtual CPt GetPreferredSize();
    virtual CPt GetMinimumSize();
    virtual CPt GetMaximumSize();

    // CSplitterBase
    virtual void SetSplitLocation(long inPixels);
    virtual long GetSplitLocation() const;
    virtual ESplitDirection GetSplitDirection() const;

    virtual void SetSplitLimits(long inSplitMinMargin, long inSplitMaxMargin);
    virtual void SetSplitDirection(CSplitterBase::ESplitDirection inSplitDirection);

    virtual void AddChild(CControl *inControl, CControl *inInsertBefore = nullptr);

    virtual void OnChildSizeChanged(CControl *inChild);

    virtual void SetSplitBarWidth(const long inWidth);

protected:
    void RecalcLayout();

    ESplitDirection m_SplitDirection;
    long m_SplitLocation;
    long m_SplitMinMargin;
    long m_SplitMaxMargin;
    CSplitBar *m_SplitBar;
    bool m_InRecalcLayoutFlag;
};

#endif // INCLUDED_SPLIT_AGENT_H