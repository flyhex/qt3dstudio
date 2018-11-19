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
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

TextField {
    id: styledTextFieldId
    property bool ignoreHotkeys: true
    property bool textChanged: false

    signal editingFinished

    selectByMouse: true
    text: ""
    Layout.preferredWidth: _valueWidth
    Layout.preferredHeight: _controlBaseHeight

    topPadding: 0
    bottomPadding: 0
    rightPadding: 6
    leftPadding: 6

    activeFocusOnPress: false

    horizontalAlignment: TextInput.AlignRight
    verticalAlignment: TextInput.AlignVCenter

    selectionColor: _selectionColor
    selectedTextColor: _textColor
    font.pixelSize: _fontSize
    color: _textColor
    background: Rectangle {
        color: styledTextFieldId.enabled ? _studioColor2 : "transparent"
        border.width: styledTextFieldId.activeFocus ? 1 : 0
        border.color: styledTextFieldId.activeFocus ? _selectionColor : _disabledColor
    }

    cursorVisible: false
    onActiveFocusChanged: {
        if (!activeFocus && textChanged) {
            styledTextFieldId.editingFinished();
            textChanged = false;
        }

        if (focusReason === Qt.OtherFocusReason) {
            select(0, 0);
            cursorVisible = false;
        } else if (activeFocus) {
            selectAll();
        }
    }

    MouseArea {
        id: mouseArea
        property int clickedPos: 0

        acceptedButtons: Qt.LeftButton
        preventStealing: true
        anchors.fill: parent
        onPressed: {
            if (parent.activeFocus) {
                clickedPos = parent.positionAt(mouse.x, mouse.y);
                parent.cursorPosition = clickedPos;
            } else {
                parent.forceActiveFocus();
            }
        }

        onClicked: {
            if (!parent.cursorVisible) {
                parent.cursorVisible = true;
                parent.selectAll();
            }
        }

        onDoubleClicked: {
            parent.selectAll();
            parent.cursorVisible = true;
        }
    }

    onTextChanged: textChanged = true;

    Keys.onPressed: {
        if (textChanged && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)) {
            event.accepted = true
            styledTextFieldId.editingFinished();
            textChanged = false;
        }
    }
}
