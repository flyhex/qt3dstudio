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
import QtQuick.Layouts 1.3
import "../controls"

RowLayout {
    id: root

    property alias label: labelField.text
    property alias value: textArea.text
    property Item tabItem1: textArea

    signal editingFinished()

    StyledLabel {
        id: labelField
        text: qsTr("Argument")
    }

    Flickable {
        Layout.preferredWidth: _valueWidth
        Layout.preferredHeight: textArea.height

        ScrollBar.vertical: ScrollBar {}

        TextArea.flickable: TextArea {
            id: textArea

            horizontalAlignment: TextInput.AlignLeft
            verticalAlignment: TextInput.AlignVCenter
            implicitWidth: _valueWidth
            font.pixelSize: _fontSize
            color: _textColor

            topPadding: 6
            bottomPadding: 6
            rightPadding: 6

            wrapMode: TextArea.Wrap
            background: Rectangle {
                color: textArea.enabled ? _studioColor2 : "transparent"
                border.width: textArea.activeFocus ? 1 : 0
                border.color: textArea.activeFocus ? _selectionColor : _disabledColor
            }

            onEditingFinished: root.editingFinished()
        }

        MouseArea {
            id: mouseArea

            anchors.fill: parent
            onPressed: parent.forceActiveFocus()
        }
    }
}
