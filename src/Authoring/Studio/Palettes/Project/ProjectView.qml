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
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.3
import "../controls"

Rectangle {
    id: root

    color: _backgroundColor

    ColumnLayout {
        anchors.fill: parent
        spacing: 4
        width: parent.width

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: {
                    _parentView.showContextMenu(mouse.x, mouse.y, projectTree.currentIndex);
                }
            }

            ListView {
                id: projectTree

                anchors.fill: parent
                clip: true

                ScrollBar.vertical: ScrollBar {}

                model: _parentView.projectModel

                onCurrentIndexChanged: {
                    // Try to keep something selected always
                    if ((currentIndex < 0 || currentIndex >= count) && count > 0)
                        currentIndex = 0;
                }

                delegate: Rectangle {
                    id: delegateItem
                    property bool dragging: false
                    width: parent.width
                    height: 20
                    color: (index == projectTree.currentIndex || dragging) ? _selectionColor
                                                                           : "transparent"

                    Row {
                        x: _depth*28
                        anchors.verticalCenter: delegateItem.verticalCenter

                        Image {
                            source: _resDir + (_expanded ? "arrow_down.png" : "arrow.png")
                            opacity: _isExpandable ? 1 : 0

                            MouseArea {
                                visible: _isExpandable
                                anchors.fill: parent
                                onClicked: {
                                    if (_expanded)
                                       projectTree.model.collapse(index)
                                    else
                                        projectTree.model.expand(index)
                                    delegateMouseArea.clickPending = false
                                }
                            }
                        }

                        Image {
                            source: fileIcon

                            Item {
                                id: dragItemIcon

                                visible: _isDraggable
                                anchors.fill: parent

                                Drag.active: dragAreaIcon.drag.active
                                Drag.hotSpot.x: width / 2
                                Drag.hotSpot.y: height / 2
                                Drag.dragType: Drag.Automatic
                                Drag.supportedActions: Qt.CopyAction

                                MouseArea {
                                    id: dragAreaIcon
                                    anchors.fill: parent
                                    drag.target: dragItemIcon
                                }

                                Drag.onDragStarted: _parentView.startDrag(dragAreaIcon, index)
                            }
                        }

                        StyledLabel {
                            text: fileName
                            color: _isReferenced ? _textColor : _disabledColor
                            leftPadding: 2

                            Item {
                                id: dragItem

                                visible: _isDraggable
                                anchors.fill: parent

                                Drag.active: dragArea.drag.active
                                Drag.hotSpot.x: width / 2
                                Drag.hotSpot.y: height / 2
                                Drag.dragType: Drag.Automatic
                                Drag.supportedActions: Qt.CopyAction

                                MouseArea {
                                    id: dragArea
                                    anchors.fill: parent
                                    drag.target: dragItem
                                }

                                Drag.onDragStarted: _parentView.startDrag(dragArea, index)
                            }
                        }
                    }

                    DropArea {
                        anchors.fill: parent

                        onEntered: {
                            if (drag.hasUrls
                                    && projectTree.model.hasValidUrlsForDropping(drag.urls)) {
                                dragging = true;
                                drag.accept(Qt.CopyAction)
                            } else {
                                drag.accepted = false;
                            }
                        }

                        onExited: {
                            dragging = false;
                        }

                        onDropped: {
                            if (drop.hasUrls)
                                projectTree.model.importUrls(drop.urls, index, false);
                            dragging = false;
                        }
                    }

                    MouseArea {
                        id: delegateMouseArea
                        property bool clickPending: false
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton|Qt.LeftButton
                        propagateComposedEvents: true
                        onPressed: {
                            projectTree.currentIndex = model.index;

                            // Presses must be ignored by this handler in order for dragging to work
                            mouse.accepted = false;

                            // Since ignoring presses means we don't get doubleClicked events,
                            // detect doubleclick using custom timer.
                            if (clickPending) {
                                if (_isExpandable) {
                                    if (_expanded)
                                        projectTree.model.collapse(index);
                                    else
                                        projectTree.model.expand(index);
                                }
                                clickPending = false;
                            } else {
                                clickPending = true;
                                doubleClickTimer.restart();
                            }
                        }
                        Timer {
                            id: doubleClickTimer
                            repeat: false
                            triggeredOnStart: false
                            interval: 500
                            onTriggered: parent.clickPending = false;
                        }
                    }
                }
                DropArea {
                    // Leftover listview area. Dropping here is equivalent to dropping to root
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: parent.height - parent.contentHeight
                    onEntered: {
                        if (drag.hasUrls && projectTree.model.hasValidUrlsForDropping(drag.urls))
                            drag.accept(Qt.CopyAction)
                        else
                            drag.accepted = false;
                    }
                    onDropped: {
                        if (drop.hasUrls)
                            projectTree.model.importUrls(drop.urls, 0, false)
                    }
                }
            }
        }

        StyledMenuSeparator {
            leftPadding: 12
            rightPadding: 12
        }

        RowLayout {
            width: parent.width
            Layout.margins: 4
            Layout.rightMargin: 12
            Layout.leftMargin: 12

            StyledToolButton {
                enabledImage: "Asset-import-Normal.png";
                onClicked: _parentView.assetImportAction(projectTree.currentIndex);
                toolTipText: qsTr("Import Assets");
            }

            Item {
                Layout.fillWidth: true
            }

            StyledToolButton {
                enabledImage: "Objects-Effect-Normal.png";
                onClicked: _parentView.effectAction(projectTree.currentIndex)
                toolTipText: qsTr("Open Effect Library")
            }

            StyledToolButton {
                enabledImage: "Objects-Text-Normal.png";
                onClicked: _parentView.fontAction(projectTree.currentIndex)
                toolTipText: qsTr("Open Font Library")
            }

            StyledToolButton {
                enabledImage: "Objects-Image-Normal.png";
                onClicked: _parentView.imageAction(projectTree.currentIndex)
                toolTipText: qsTr("Open Map Library")
            }

            StyledToolButton {
                enabledImage: "Objects-Material-Normal.png";
                onClicked: _parentView.materialAction(projectTree.currentIndex)
                toolTipText: qsTr("Open Material Library")
            }

            StyledToolButton {
                enabledImage: "Objects-Model-Normal.png";
                onClicked: _parentView.modelAction(projectTree.currentIndex)
                toolTipText: qsTr("Open Model Library")
            }

            StyledToolButton {
                enabledImage: "Objects-Behavior-Normal.png";
                onClicked: _parentView.behaviorAction(projectTree.currentIndex)
                toolTipText: qsTr("Open Behavior Library")
            }
        }
    }
}
