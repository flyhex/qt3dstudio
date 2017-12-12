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
    property bool ignoreHotkeys: true

    signal previewValueChanged

    selectByMouse: true
    text: "0.000"
    Layout.preferredWidth: _valueWidth / 2
    Layout.preferredHeight: _controlBaseHeight

    topPadding: 0
    bottomPadding: 0
    rightPadding: 6

    onTextEdited: {
        if (text.search(","))
            text = text.replace(",",".")
    }

    activeFocusOnPress: false

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

    Timer {
        id: rateLimiter
        interval: 10
        onTriggered: {
            floatTextFieldId.previewValueChanged();
        }
    }

    cursorVisible: false
    onActiveFocusChanged: {
        if (focusReason === Qt.OtherFocusReason) {
            select(0, 0);
            cursorVisible = false;
        } else if (activeFocus) {
            selectAll();
        }
    }

    Item {
        id: focusEater
        // Used to eat keyboard focus after drag-modifying the text is finished
    }

    MouseArea {
        id: mouseArea
        property int clickedPos: 0
        property int pressedX: 0
        property bool draggingActive: false

        preventStealing: true
        anchors.fill: parent
        onPressed: {
            pressedX = mouse.x;
            draggingActive = false;
            if (parent.activeFocus) {
                clickedPos = parent.positionAt(mouse.x, mouse.y);
                parent.cursorPosition = clickedPos;
            } else {
                parent.forceActiveFocus();
            }
        }
        onClicked: {
            if (!draggingActive && !parent.cursorVisible) {
                parent.cursorVisible = true;
                parent.selectAll();
            }
        }
        onReleased: {
            if (draggingActive) {
                _mouseHelper.endUnboundedDrag();
                rateLimiter.stop();
                floatTextFieldId.onEditingFinished();
                focusEater.forceActiveFocus();
            }
        }

        onCanceled: {
            if (draggingActive) {
                _mouseHelper.endUnboundedDrag();
                rateLimiter.stop();
                floatTextFieldId.onEditingFinished();
                focusEater.forceActiveFocus();
            }
        }

        onDoubleClicked: {
            parent.selectAll();
            parent.cursorVisible = true;
        }

        onPositionChanged: {
            if (parent.cursorVisible) {
                parent.cursorPosition = parent.positionAt(mouse.x, mouse.y);
                parent.select(clickedPos, parent.cursorPosition);
            } else {
                if (!draggingActive) {
                    var startDelta = (pressedX - mouse.x) / 2.0;
                    if (startDelta > 4.0 || startDelta < -4.0) {
                        _mouseHelper.startUnboundedDrag();
                        draggingActive = true;
                    }
                }
                if (draggingActive) {
                    var delta = _mouseHelper.delta().x;
                    if (delta !== 0) {
                        floatTextFieldId.text =
                                Number(parseFloat(floatTextFieldId.text)
                                       + delta).toFixed(validator.decimals);
                        if (!rateLimiter.running)
                            rateLimiter.start();
                    }
                }
            }
        }
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
            event.accepted = true
            var delta = 1.0;
            if (event.modifiers === Qt.ControlModifier)
                delta = 0.1;
            else if (event.modifiers === Qt.ShiftModifier)
                delta = 10.0;
            if (event.key === Qt.Key_Down)
                delta = -delta;
            floatTextFieldId.text = Number(parseFloat(floatTextFieldId.text)
                                           + delta).toFixed(validator.decimals);
            if (!rateLimiter.running)
                rateLimiter.start();
        }
    }
}
