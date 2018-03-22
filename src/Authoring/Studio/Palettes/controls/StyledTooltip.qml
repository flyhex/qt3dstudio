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

import QtQuick 2.6
import QtQuick.Controls 2.1

ToolTip {
    id: control
    delay: 500
    contentItem: StyledLabel {
        text: control.text
    }

    // Handle tooltip visibility based on the trigger event given to the 'enabled' property and
    // the 'Tooltips' view menu setting. Has to be done this way, as even though the eventFilter
    // set for MainFrm catches the tooltip events for QML, it doesn't prevent showing them because
    // we were/are controlling the visibility in code.
    onEnabledChanged: {
        if (enabled && _parentView.toolTipsEnabled())
            visible = true;
        else
            visible = false;
    }

    background: Rectangle {
        border.color: _studioColor3
        color: _studioColor2
        radius: 2
    }
}
