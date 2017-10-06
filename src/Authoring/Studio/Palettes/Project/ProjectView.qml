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

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: {
                    _projectView.showContextMenu(mouse.x, mouse.y, projectTree.currentIndex);
                }
            }

            ListView {
                id: projectTree

                anchors.fill: parent

                ScrollBar.vertical: ScrollBar {}

                model: _projectView.projectModel

                delegate: Rectangle {
                    id: delegateItem

                    width: parent.width
                    height: 20
                    color: index == projectTree.currentIndex ? _selectionColor : "transparent"

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

                                Drag.onDragStarted: _projectView.startDrag(index)
                            }
                        }
                    }

                    DropArea {
                        id: dropArea

                        anchors.fill: parent

                        onEntered: {
                            if (drag.hasUrls && projectTree.model.hasValidUrlsForDropping(drag.urls)) {
                                drag.accept(Qt.CopyAction)
                            } else {
                                drag.accepted = false;
                            }
                        }

                        onDropped: {
                            if (drop.hasUrls) {
                                projectTree.model.dropUrls(drop.urls, index)
                            }
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
            }
        }

        StyledMenuSeparator {}

        RowLayout {
            width: parent.width
            Layout.margins: 4
            Layout.rightMargin: 12

            Item {
                Layout.fillWidth: true
            }

            StyledToolButton {
                enabledImage: "Objects-Effect-Normal.png";
                onClicked: _projectView.effectAction()
                toolTipText: qsTr("Open Effect Library directory")
            }

            StyledToolButton {
                enabledImage: "Objects-Text-Normal.png";
                onClicked: _projectView.fontAction()
                toolTipText: qsTr("Open Font Library directory")
            }

            StyledToolButton {
                enabledImage: "Objects-Image-Normal.png";
                onClicked: _projectView.imageAction()
                toolTipText: qsTr("Open Maps Library directory")
            }

            StyledToolButton {
                enabledImage: "Objects-Material-Normal.png";
                onClicked: _projectView.materialAction()
                toolTipText: qsTr("Open Material Library directory")
            }

            StyledToolButton {
                enabledImage: "Objects-Model-Normal.png";
                onClicked: _projectView.modelAction()
                toolTipText: qsTr("Open Models Library directory")
            }

            StyledToolButton {
                enabledImage: "Objects-Behavior-Normal.png";
                onClicked: _projectView.behaviorAction()
                toolTipText: qsTr("Open Behavior Library directory")
            }
        }
    }
}
