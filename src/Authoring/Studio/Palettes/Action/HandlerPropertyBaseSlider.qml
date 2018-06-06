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
import "../controls"

/*
* Use for: Opacity,  Edge Tesselation Value, Inner Tesselation Value ...
* For the latter two set sliderMax to 64
*/

Row {
    id: root

    property alias value: slider.value
    property alias sliderMin: slider.from
    property alias sliderMax: slider.to
    property bool intSlider: false
    property int decimalSlider: 3
    property Item tabItem1: textField

    signal editingFinished
    signal sliderMoved

    spacing: 5
    width: _valueWidth

    Keys.onPressed: {
        if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
            event.accepted = true
            var delta = 1.0;
            if (intSlider) {
                if (event.key === Qt.Key_Down)
                    delta = -delta;
                slider.value = Number(slider.value + delta).toFixed(0);
            } else {
                if (event.modifiers === Qt.ControlModifier)
                    delta = 0.1;
                else if (event.modifiers === Qt.ShiftModifier)
                    delta = 10.0;
                if (event.key === Qt.Key_Down)
                    delta = -delta;
                slider.value = Number(slider.value + delta).toFixed(doubleValidator.decimals);
            }
            if (!rateLimiter.running)
                rateLimiter.start();
            textField.setTextFieldValue()
        }
    }

    Slider {
        id: slider

        leftPadding: 0

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: _valueWidth / 2 - 5
            implicitHeight: 6
            height: implicitHeight
            radius: 2
            color: _studioColor2
        }
        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * slider.availableWidth
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 6
            implicitHeight: 12
            color: _studioColor3
            radius: 2
        }

        from: 0
        to: 100
        stepSize: root.intSlider ? 1 : sliderStepFromRange(slider.to, slider.from, 100)
        snapMode: root.intSlider ? Slider.SnapAlways : Slider.NoSnap

        function sliderStepFromRange(top, bottom, steps) {
            return ((top - bottom) / steps);
        }

        onMoved: {
            if (!rateLimiter.running) {
                rateLimiter.start();
            }
            textField.setTextFieldValue()
        }

        onPressedChanged: {
            if (!pressed) {
                if (rateLimiter.running)
                    rateLimiter.stop();
                textField.setTextFieldValue()
                root.editingFinished();
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton

            onWheel: {
                var delta = (wheel.angleDelta.x != 0) ? wheel.angleDelta.x
                                                      : wheel.angleDelta.y;

                if (delta > 0) {
                    slider.increase();
                } else {
                    slider.decrease();
                }
                if (!rateLimiter.running) {
                    rateLimiter.start();
                }
                textField.setTextFieldValue()
            }
        }
    }

    Timer {
        id: rateLimiter
        interval: 10
        onTriggered: {
            root.sliderMoved();
        }
    }

    DoubleValidator {
        id: doubleValidator

        decimals: decimalSlider
        bottom: slider.from
        top: slider.to
        locale: "C"
    }

    IntValidator {
        id: intValidator

        bottom: slider.from
        top: slider.to
    }

    StyledTextField {
        id: textField

        height: _controlBaseHeight
        width: _valueWidth / 2
        text: intSlider ? slider.value.toFixed(0) : slider.value.toFixed(decimalSlider)

        validator: intSlider ? intValidator : doubleValidator

        onTextEdited: {
            if (!intSlider && text.search(",")) {
                text = text.replace(",",".")
            }
            if (intSlider) {
                // handle limiting integer values when entered value is less than
                // minimum value since IntValidator doesn't handle this
                if (text.length >= sliderMin.toString().length && text < sliderMin)
                    text = text.substring(0, text.length - 1)
            }
        }

        onEditingFinished: {
            if (text > sliderMax)
                text = sliderMax
            else if (text < sliderMin)
                text = sliderMin
            slider.value = text
            root.editingFinished()
        }

        function setTextFieldValue() {
            text = intSlider ? slider.value.toFixed(0) : slider.value.toFixed(decimalSlider)
        }
        onActiveFocusChanged: {
            if (!activeFocus)
                setTextFieldValue()
        }
    }
}
