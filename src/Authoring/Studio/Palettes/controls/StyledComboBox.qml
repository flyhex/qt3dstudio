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
import QtQuick.Window 2.2

ComboBox {
    id: control

    property bool ignoreHotkeys: true

    Layout.preferredHeight: _controlBaseHeight
    Layout.preferredWidth: _valueWidth
    topPadding: 0
    bottomPadding: 0
    delegate: ItemDelegate {
        id: itemDelegate

        property bool hasSeparator: itemDelegate.text.endsWith("|separator")

        width: parent.width
        height: hasSeparator ? _controlBaseHeight + 6 : _controlBaseHeight
        padding: 0
        spacing: 0
        text: {
            control.textRole ? (Array.isArray(control.model) ? modelData[control.textRole]
                                                             : model[control.textRole])
                             : modelData
        }
        highlighted: control.highlightedIndex === index
        hoverEnabled: control.hoverEnabled
        contentItem: ColumnLayout {
            anchors.fill: itemDelegate
            spacing: 0
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: _studioColor3
                visible: itemDelegate.hasSeparator
            }
            StyledLabel {
                Layout.fillWidth: true
                rightPadding: control.indicator.width + 6
                leftPadding: 6
                text: {
                    hasSeparator ? itemDelegate.text.replace("|separator", "")
                                 : itemDelegate.text
                }
                visible: itemDelegate.text
                horizontalAlignment: Text.AlignLeft
            }
        }
        background: Rectangle {
            anchors.fill: itemDelegate
            color: hovered ? _selectionColor : _studioColor2
        }
    }

    indicator: Image {
        x: control.width - width - 2
        y: control.topPadding + (control.availableHeight - height) / 2
        source: _resDir + "arrow_down.png"
    }

    contentItem: StyledTextField {
        text: {
            var newText = control.editable ? control.editText : control.displayText;
            var hasSeparator = newText.endsWith("|separator");
            hasSeparator ? newText.replace("|separator", "") : newText;
        }

        enabled: control.editable
        autoScroll: control.editable
        readOnly: control.popup.visible
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        opacity: 1
        leftPadding: 6
        horizontalAlignment: Text.AlignLeft
    }

    background: Rectangle {
        color: control.enabled ? _studioColor2 : "transparent"
        border.width: 0
    }

    popup: Popup {
        y: control.height
        width: control.width
        height: Math.min(contentItem.implicitHeight,
                         control.Window.height - topMargin - bottomMargin)
        topMargin: 6
        bottomMargin: 6
        padding: 0

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            highlightRangeMode: ListView.ApplyRange
            highlightMoveDuration: 0
            ScrollIndicator.vertical: ScrollIndicator {
                id: scrollIndicator
                contentItem: Rectangle {
                    id: indicator

                    implicitWidth: 2
                    implicitHeight: 2

                    color: _studioColor3
                    visible: scrollIndicator.size < 1.0
                    opacity: 0.75
                }
            }
            Rectangle {
                z: 10
                anchors.fill: parent
                color: "transparent"
                border.color: _studioColor3
            }
        }

        background: Rectangle {
            color: _studioColor2
        }
    }
}
