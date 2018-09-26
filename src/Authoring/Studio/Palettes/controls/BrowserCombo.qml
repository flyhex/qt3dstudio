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

import QtQuick 2.8
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

MouseArea {
    id: root

    property alias value: value.text
    property var activeBrowser
    property bool blockShow: false

    signal showBrowser

    Layout.minimumHeight: _controlBaseHeight
    Layout.preferredWidth: _valueWidth

    onPressed: {
        // Block showBrowser event on the mouse press that makes the browser lose focus
        if (activeBrowser && activeBrowser.visible) {
            activeBrowser = null;
            blockShow = true
        } else {
            blockShow = false
        }
    }

    onClicked: {
        if (!blockShow)
            root.showBrowser()
    }

    Rectangle {
        anchors.fill: parent

        color: _studioColor2

        StyledLabel {
            id: value
            anchors.fill: parent
            horizontalAlignment: Text.AlignLeft
            rightPadding: 6 + img.width
            leftPadding: 6
        }
        Image {
            id: img
            // Source image size is 16x16 pixels
            x: parent.width - 18
            y: parent.height / 2 - 8
            source: _resDir + "arrow_down.png"
            rotation: activeBrowser && activeBrowser.focused ? 180 : 0
        }
    }
}
