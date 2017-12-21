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
import Qt3DStudio 1.0
import "../controls"

Item {
    id: root

    property real splitterPos: 0

    Row {
        id: delegateRow

        anchors.fill: parent
        spacing: 2

        Rectangle {
            id: itemDescription

            width: root.width - buttons.width - delegateRow.spacing
            height: root.height

            color: model.selected ? _selectionColor :
                                    model.isProperty ? _backgroundColor : _studioColor2
            border.color: _backgroundColor

            MouseArea {
                id: delegateArea

                anchors.fill: parent
                onClicked: _timelineView.select(model.index, mouse.modifiers)
            }

            Row {
                id: row

                x: model.depth * 20
                anchors.verticalCenter: parent.verticalCenter
                height: root.height
                width: splitterPos - x
                spacing: 5

                Image {
                    source: {
                        if (!model.hasChildren)
                            return "";
                        model.expanded ? _resDir + "arrow_down.png"
                                       : _resDir + "arrow.png";
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: model.expanded = !model.expanded
                    }
                }

                Item {
                    height: root.height
                    width: typeIcon.width + name.width + 10

                    Row {
                        spacing: 10
                        Image {
                            id: typeIcon

                            source: model.icon
                        }

                        StyledLabel {
                            id: name
                            anchors.verticalCenter: typeIcon.verticalCenter
                            color: model.textColor
                            text: model.name
                        }
                    }
                }
            }
        }

        Loader {
            id: buttons

            active: !model.isProperty

            sourceComponent: buttonsComponent
        }

        Component {
            id: buttonsComponent


            Row {
                id: buttonsRow

                Layout.alignment: Qt.AlignRight

                StyledToggleButton {
                    width: 20
                    height: 20
                    checked: model.shy

                    backgroundColor: itemDescription.color
                    downColor: backgroundColor
                    enabledImage: "Toggle-Empty.png"
                    checkedImage: "Toggle-Shy.png"

                    onClicked: model.shy = checked
                }

                StyledToggleButton {
                    width: 20
                    height: 20
                    checked: model.rowVisible

                    backgroundColor: itemDescription.color
                    downColor: backgroundColor
                    enabledImage: "Toggle-Empty.png"
                    checkedImage: "filter-toggle-eye-up.png"

                    onClicked: model.rowVisible = checked
                }

                StyledToggleButton {
                    width: 20
                    height: 20

                    checked: model.locked

                    backgroundColor: itemDescription.color
                    downColor: backgroundColor
                    checkedImage: "Toggle-Lock.png"
                    enabledImage: "Toggle-Empty.png"

                    onClicked: model.locked = checked
                }
            }
        }
    }
}
