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

/* Use for: Position, Rotation, Scale, Pivot ... */

TextField {
    id: floatTextFieldId
    property alias decimalValue: validator.decimals

    signal wheelEventFinished

    selectByMouse: true
    text: "0.000"
    Layout.preferredWidth: _valueWidth / 2
    Layout.preferredHeight: _controlBaseHeight

    topPadding: 0
    bottomPadding: 0
    rightPadding: 6

    horizontalAlignment: TextInput.AlignRight
    verticalAlignment: TextInput.AlignVCenter
    validator: DoubleValidator {
        id: validator
        decimals: 3
        locale: "C"
    }

    selectionColor: _selectionColor
    selectedTextColor: _textColor
    font.pixelSize: _fontSize
    color: _textColor
    background: Rectangle {
        color: floatTextFieldId.enabled ? _studioColor2 : "transparent"
        border.width: floatTextFieldId.activeFocus ? 1 : 0
        border.color: floatTextFieldId.activeFocus ? _selectionColor : _disabledColor
    }

    MouseArea {
        property int _clickedPos
        id: mouseAreaBaseId

        anchors.fill: parent
        onPressed: {
            parent.forceActiveFocus()
            _clickedPos = parent.positionAt(mouse.x, mouse.y)
            parent.cursorPosition = _clickedPos
        }
        onDoubleClicked: parent.selectAll()
        onPositionChanged: {
            parent.cursorPosition = parent.positionAt(mouse.x, mouse.y)
            parent.select(_clickedPos, parent.cursorPosition)
        }

        onWheel: {
            if (!floatTextFieldId.activeFocus) {
                wheel.accepted = false
                return
            }
            if (wheel.angleDelta.y > 0) {
                floatTextFieldId.text++
            } else {
                floatTextFieldId.text--
            }
            wheel.accepted=true
            floatTextFieldId.wheelEventFinished();
        }
    }
}
