/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef ROWTYPES_H
#define ROWTYPES_H

#include <qglobal.h>

enum class TimelineControlType : int {
    None                = 0x00,
    KeyFrame            = 0x01,
    Duration            = 0x02,
    DurationStartHandle = 0x04,
    DurationEndHandle   = 0x08,
    BezierKeyframe      = 0x10,
    BezierInHandle      = 0x20,
    BezierOutHandle     = 0x40,
    IsBezierControl     = BezierKeyframe | BezierInHandle | BezierOutHandle
};

inline int operator &(TimelineControlType lhs, TimelineControlType rhs)
{
    return int(lhs) & int(rhs);
}

enum class TreeControlType {
    None,
    Arrow,
    Shy,
    Hide,
    Lock
};

#endif // ROWTYPES_H
