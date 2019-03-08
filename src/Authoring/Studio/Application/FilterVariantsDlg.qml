/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

Rectangle {
    id: root
    width: 400
    height: 280
    color: _backgroundColor
    border.color: _studioColor3

    // hider for the border segment between the icon and the dialog
    Rectangle {
        color: _backgroundColor
        x: 1
        width: _view.actionSize() - 2 // -1px from each side
        height: 1
    }

    Item {
        height: 25
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10

        Text {
            text: qsTr("Select filtering")
            color: _studioColor4
            font.pointSize: 10
            anchors.verticalCenter: parent.verticalCenter
        }

        Rectangle { // clear button
            width: 60
            height: 25
            color: _backgroundColor
            border.color: _studioColor4
            anchors.right: parent.right

            Text {
                anchors.centerIn: parent
                text: qsTr("Clear")
                font.pointSize: 10
                color: _studioColor4
            }

            MouseArea {
                anchors.fill: parent
                onClicked: _model.clearAll();
            }
        }
    }

    Flickable {
        anchors.top: parent.top
        anchors.topMargin: 35
        anchors.fill: parent
        contentWidth: variantsColumn.width
        contentHeight: variantsColumn.height
        flickableDirection: Flickable.VerticalFlick
        clip: true

        ScrollBar.vertical: ScrollBar {}

        Column {
            id: variantsColumn
            spacing: 10
            padding: 10

            Repeater { // groups
                id: tagsRepeater
                model: _model
                property int maxGroupLabelWidth;

                onItemAdded: {
                    // make all group labels have equal width as the widest one
                    if (index == 0)
                        maxGroupLabelWidth = 20; // min group label width

                    if (item.groupLabelWidth > maxGroupLabelWidth) {
                        maxGroupLabelWidth = item.groupLabelWidth;

                        if (maxGroupLabelWidth > 150) // max group label width
                            maxGroupLabelWidth = 150;
                    }
                }

                Row { // a row of a group and its tags
                    spacing: 5

                    readonly property var tagsModel: model.tags
                    readonly property var groupModel: model
                    readonly property int groupLabelWidth: tLabel.implicitWidth + 10

                    Rectangle { // group button
                        width: tagsRepeater.maxGroupLabelWidth;
                        height: 25
                        color: _backgroundColor
                        border.color: model.color

                        Text {
                            id: tLabel
                            text: model.group
                            color: model.color
                            elide: Text.ElideRight
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            anchors.fill: parent;
                            onClicked: _model.toggleGroupState(model.group);
                        }
                    }

                    Flow { // group tags
                        width: root.width - tagsRepeater.maxGroupLabelWidth - 15
                        spacing: 5

                        Repeater {
                            model: tagsModel

                            Loader {
                                readonly property var tagsModel: model
                                readonly property var grpModel: groupModel
                                sourceComponent: tagComponent
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: tagComponent

        Rectangle {
            property bool toggled: tagsModel ? tagsModel.selected : false
            property string grpColor: grpModel ? grpModel.color : ""

            width: Math.max(tLabel.width + 10, 60)
            height: 25
            color: toggled ? grpColor : _backgroundColor
            border.color: toggled ? _studioColor4 : grpColor;

            Text {
                id: tLabel
                anchors.centerIn: parent
                text: tagsModel ? tagsModel.tag : ""
                color: toggled ? _textColor : _studioColor4
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    toggled = !toggled;
                    _model.setTagState(grpModel.group, tagsModel.tag, toggled);
                }
            }
        }
    }
}
