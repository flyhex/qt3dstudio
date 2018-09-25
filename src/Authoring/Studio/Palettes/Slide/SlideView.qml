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
import "../controls"

Rectangle {

    id: root

    readonly property bool masterSlide: _parentView.showMasterSlide

    function handleMouseClicks(mouse) {
        if (mouse.button === Qt.RightButton) {
            const coords = slideList.mapToItem(root, mouse.x, mouse.y);
            _parentView.showContextMenu(coords.x, coords.y, -1);
        } else {
            root.focus = true;
            //Unselect All element when we click outside slider item in listView.
            //It worked as it in old version.
            _parentView.deselectAll();
            mouse.accepted = false
        }
    }

    Connections {
        target: _parentView
        onDockAreaChanged: diIndicator.reAnchor();
    }

    color: _backgroundColor

    Column {
        anchors {
            top: parent.top
            topMargin: 5
            horizontalCenter: parent.horizontalCenter
        }

        spacing: 5
        width: parent.width

        MouseArea {
            id: masterMouseArea

            width: parent.width
            height: childrenRect.height

            propagateComposedEvents: true
            acceptedButtons: Qt.AllButtons
            onClicked: root.handleMouseClicks(mouse)

            Column {
                id: masterButtonColumn
                spacing: -4
                anchors.horizontalCenter: parent.horizontalCenter
                Button {
                    id: masterEditButton
                    anchors.horizontalCenter: parent.horizontalCenter

                    onClicked: _parentView.showMasterSlide = !_parentView.showMasterSlide

                    background: Rectangle {
                        color: "transparent"
                    }
                    contentItem: Image {
                        source: _parentView.showMasterSlide ? _resDir + "Slide-Normal.png"
                                                            : _resDir + "Slide-Master-Active.png"
                    }
                }

                StyledLabel {
                    id: masterEditLabel
                    text: _parentView.showMasterSlide ? qsTr("Leave Master") : qsTr("Edit Master")
                    font.pixelSize: _fontSize
                    color: _masterColor
                    verticalAlignment: Text.AlignVCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
        StyledMenuSeparator {
            id: separator
            leftPadding: 12
            rightPadding: 12
        }

        ListView {
            id: slideList

            ScrollBar.vertical: ScrollBar {}

            width: root.width
            property int listItemHeight: root.height - masterButtonColumn.height
                                         - separator.height - separator2.height
                                         - parent.spacing * 2 - 14 - slideControlButton.height
                                         - slideControlButton.spacing * 2
            // DockWidgetArea is enum; value 0x2 denotes right edge
            property int area: _parentView.dockArea
            height: listItemHeight > 0 ? listItemHeight : 0

            anchors.horizontalCenter: parent.horizontalCenter
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            model: _parentView.currentModel
            spacing: 10

            Rectangle {
                id: diIndicator
                height: slideList.listItemHeight
                width: dataInputImage2.height

                function reAnchor() {
                    // reset anchors before setting new value
                    anchors.right = undefined
                    anchors.left = undefined
                    // default position for indicator is right edge
                    // except when slide panel is attached to window right side
                    if (parent.area === 2)
                        anchors.left = parent.left
                    else
                        anchors.right = parent.right
                }

                color: _parentView.controlled ? _dataInputColor : "transparent"
                Row {
                    rotation: 90
                    anchors.centerIn: parent
                    spacing: 5
                    Image {
                        id: dataInputImage2
                        fillMode: Image.Pad
                        visible: _parentView.controlled
                        source: _resDir + "Objects-DataInput-White.png"

                    }
                    StyledLabel {
                        text: _parentView.currController
                        anchors.margins: 16
                        color: "#ffffff"
                    }
                }
            }

            MouseArea {
                // mouse handling for the area not covered by the delegates
                propagateComposedEvents: true
                anchors.fill: parent
                acceptedButtons: Qt.AllButtons
                onClicked: {
                    if (slideList.indexAt(mouse.x, mouse.y) === -1)
                        root.handleMouseClicks(mouse);
                    else
                        mouse.accepted = false;
                }
                onPressed: {
                    if (slideList.indexAt(mouse.x, mouse.y) !== -1)
                        mouse.accepted = false;
                }
            }

            delegate: MouseArea {
                id: delegateArea

                property int dragIndex
                property bool held : false

                anchors.horizontalCenter: parent.horizontalCenter
                height: delegateItem.height
                width: parent.width

                acceptedButtons: Qt.RightButton | Qt.LeftButton
                drag.target: held ? delegateItem : null
                drag.axis: Drag.YAxis

                onPressed: {
                    dragIndex = model.index;
                    _parentView.startSlideRearrange(model.index);
                    if (mouse.x > delegateItem.x && mouse.x < delegateItem.x + delegateItem.width)
                        held = true;
                }

                onReleased: {
                    held = false;
                    _parentView.finishSlideRearrange(true);
                }

                onCanceled: {
                    held = false;
                    _parentView.finishSlideRearrange(false);
                }

                onClicked: {
                    _parentView.deselectAll();
                    if (mouse.button === Qt.LeftButton) {
                        root.focus = true;
                        model.selected = true;
                    }
                    if (mouse.button === Qt.RightButton) {
                        const coords = mapToItem(root, mouse.x, mouse.y);
                        _parentView.showContextMenu(coords.x, coords.y, model.index);
                    }
                }

                Item {
                    id: delegateItem

                    anchors.centerIn: parent
                    height: column.implicitHeight
                    width: 100

                    Drag.keys: "application/x-slide"
                    Drag.active: delegateArea.held
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2
                    Drag.source: delegateArea

                    Column {
                        id: column
                        spacing: 2
                        anchors.fill: parent
                        Image {
                            id: slideImage

                            source: {
                                if (masterSlide)
                                    return _resDir + "Slide-Master-Active.png"
                                return model.selected ? _resDir + "Slide-Active.png"
                                                      : _resDir + "Slide-Normal.png";
                            }
                        }

                        Item {
                            anchors.horizontalCenter: slideImage.horizontalCenter

                            height: childrenRect.height
                            width: childrenRect.width
                            Row {
                                StyledLabel {
                                    visible: !masterSlide
                                    text: model.index + 1 + ": "
                                }

                                TextInput {
                                    id: slideName

                                    property bool ignoreHotkeys: true

                                    readOnly: masterSlide
                                    selectByMouse: !readOnly
                                    color: _textColor
                                    text: model.name
                                    font.pixelSize: _fontSize

                                    onFocusChanged: {
                                        if (focus && !readOnly)
                                            selectAll();
                                    }

                                    onEditingFinished: {
                                        model.name = text;
                                        slideName.focus = false;
                                    }

                                    Keys.onEscapePressed: {
                                        slideName.undo();
                                        slideName.focus = false;
                                    }
                                }
                            }
                        }
                    }
                }

                DropArea {
                    anchors.fill: parent
                    keys: "application/x-slide"
                    onEntered: {
                        var oldIndex = drag.source.dragIndex
                        var newIndex = model.index
                        _parentView.moveSlide(oldIndex, newIndex)
                        drag.source.dragIndex = newIndex
                    }
                }

                states: State {
                    when: held

                    ParentChange {
                        target: delegateItem
                        parent: slideList
                    }

                    PropertyChanges {
                        target: delegateItem
                        anchors.centerIn: null
                    }
                }
            }
        }

        StyledMenuSeparator {
            id: separator2
            leftPadding: 12
            rightPadding: 12
        }
        // RowLayout for possible addition and positioning of label
        // showing the controller name
        RowLayout {
            Layout.rightMargin: 12
            Layout.leftMargin: 12
            anchors.left: parent.left
            Button {
                id: slideControlButton
                width: dataInputImage.sourceSize.width
                height: dataInputImage.sourceSize.height
                Layout.leftMargin: 12
                property bool controlled: _parentView.controlled
                property string currentController: _parentView.currController
                property string toolTip: _parentView.toolTip
                background: Rectangle {
                    color: controlButtonArea.containsMouse ? _studioColor1 : _backgroundColor
                }
                MouseArea {
                    id: controlButtonArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton
                    onClicked:  {
                        _parentView.showControllerDialog(mapToGlobal(x + width, y + height));
                    }
                }
                Image {
                    id: dataInputImage
                    anchors.fill: parent
                    fillMode: Image.Pad
                    property bool controlled: parent.controlled
                    source: {
                        _resDir + (controlled
                                   ? "Objects-DataInput-Active.png"
                                   : "Objects-DataInput-Inactive.png")
                    }
                }
                StyledTooltip {
                    id: tooltip
                    enabled: controlButtonArea.containsMouse
                    text: parent.toolTip
                }
            }
            StyledLabel {
                id: dataInputName
                text: _parentView.currController
                color: _parentView.controlled ? _dataInputColor : "transparent"
            }
        }
    }
}
