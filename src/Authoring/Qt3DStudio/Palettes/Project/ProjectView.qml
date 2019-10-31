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
    color: _backgroundColor

    Connections {
       target: _parentView.projectModel
       onSelectFileChanged: {
           projectTree.currentIndex = idx; // auto select a newly created file
       }
    }

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
                    _parentView.showContextMenu(mouse.x, mouse.y, projectTree.currentIndex);
                }
            }

            ListView {
                id: projectTree

                anchors.fill: parent
                clip: true
                focus: true

                ScrollBar.vertical: ScrollBar {}
                keyNavigationEnabled: true

                model: _parentView.projectModel

                onCurrentIndexChanged: {
                    // Try to keep something selected always
                    if ((currentIndex < 0 || currentIndex >= count) && count > 0)
                        currentIndex = 0;
                    _parentView.setSelected(currentIndex);
                }

                delegate: Rectangle {
                    id: delegateItem
                    property bool dragging: false
                    property bool dragStarted: false
                    property point pressPoint
                    width: parent.width
                    height: 20
                    color: (index == projectTree.currentIndex || dragging) ? _selectionColor
                                                                           : "transparent"
                    function handlePress(mouse, tryDrag) {
                        projectTree.currentIndex = model.index;
                        _parentView.setSelected(model.index);

                        if (mouse.button === Qt.LeftButton && tryDrag && _isDraggable) {
                            pressPoint = Qt.point(mouse.x, mouse.y);
                            dragStarted = false;
                        }
                    }

                    function handlePositionChange(mouse, item) {
                        if (_isDraggable && !dragStarted
                                && (Math.abs(mouse.x - pressPoint.x) > 4
                                    || Math.abs(mouse.y - pressPoint.y) > 4)) {
                            dragStarted = true;
                            _parentView.startDrag(item, index);
                        }
                    }

                    function handleClick(mouse) {
                        _parentView.setSelected(projectTree.currentIndex)
                        if (mouse.button === Qt.RightButton) {
                            var rootPoint = mapToItem(root, mouse.x, mouse.y);
                            _parentView.showContextMenu(rootPoint.x, rootPoint.y,
                                                        projectTree.currentIndex);
                        }
                    }

                    function handleDoubleClick(mouse) {
                        if (mouse.button === Qt.LeftButton) {
                            if (_isExpandable) {
                                if (_expanded)
                                    projectTree.model.collapse(index);
                                else
                                    projectTree.model.expand(index);
                            } else {
                                _parentView.openFile(index);
                            }
                        }
                    }

                    MouseArea {
                        id: delegateMouseArea
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton | Qt.LeftButton

                        onPressed: delegateItem.handlePress(mouse, false)
                        onClicked: delegateItem.handleClick(mouse)
                        onDoubleClicked: delegateItem.handleDoubleClick(mouse)

                        Row {
                            x: _depth*28
                            anchors.verticalCenter: parent.verticalCenter

                            Image {
                                source: _resDir + (_expanded ? "arrow_down.png" : "arrow.png")
                                opacity: _isExpandable ? 1 : 0

                                MouseArea {
                                    visible: _isExpandable
                                    anchors.fill: parent
                                    acceptedButtons: Qt.LeftButton
                                    onPressed: delegateItem.handlePress(mouse, false)
                                    onClicked: {
                                        _parentView.setSelected(index);
                                        if (_expanded)
                                            projectTree.model.collapse(index)
                                        else
                                            projectTree.model.expand(index)
                                    }
                                }
                            }

                            Image {
                                id: fileIconImage
                                source: fileIcon
                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.RightButton | Qt.LeftButton
                                    onPressed: delegateItem.handlePress(mouse, true)
                                    onPositionChanged: delegateItem.handlePositionChange(
                                                           mouse, fileIconImage)
                                    onClicked: delegateItem.handleClick(mouse)
                                    onDoubleClicked: delegateItem.handleDoubleClick(mouse)
                                }
                            }

                            StyledLabel {
                                id: fileNameLabel
                                text: _fileId ? fileName + " <" + _fileId + ">" : fileName;
                                color: {
                                    _isReferenced ? _textColor
                                                  : _isProjectReferenced ? _projectReferencedColor
                                                                         : _disabledColor
                                }
                                leftPadding: 2

                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.RightButton | Qt.LeftButton
                                    onPressed: delegateItem.handlePress(mouse, true)
                                    onPositionChanged: delegateItem.handlePositionChange(
                                                           mouse, fileNameLabel)
                                    onClicked: delegateItem.handleClick(mouse)
                                    onDoubleClicked: delegateItem.handleDoubleClick(mouse)
                                }
                            }

                            Item {
                                // Spacer item
                                width: 4
                                height: 1
                            }

                            Image {
                                source: _extraIcon ? _resDir + _extraIcon : ""
                                visible: _extraIcon ? true : false

                                MouseArea {
                                    id: warningMouseArea
                                    anchors.fill: parent
                                    acceptedButtons: Qt.RightButton | Qt.LeftButton
                                    hoverEnabled: true
                                    onPressed: delegateItem.handlePress(mouse, false)
                                    onClicked: delegateItem.handleClick(mouse)
                                    onDoubleClicked: _parentView.editPresentationId(
                                                         index, _parentView.isQmlStream(index))
                                }
                                StyledTooltip {
                                    text: _parentView.isPresentation(index)
                                          ? qsTr("No presentation Id")
                                          : qsTr("No Qml stream Id")
                                    enabled: warningMouseArea.containsMouse
                                }
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
            Layout.fillWidth: true
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
                enabledImage: "Assets-Model.png";
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
