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
    border.color: _studioColor3

    ColumnLayout {
        anchors.fill: parent

        ListView {
            id: eventsList

            Layout.margins: 10
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 80
            Layout.preferredHeight: count * 20
            Layout.preferredWidth: root.width

            ScrollBar.vertical: ScrollBar {}

            boundsBehavior: Flickable.StopAtBounds
            clip: true
            currentIndex: _eventsBrowserView.selection

            model: _eventsBrowserView.model

            delegate: Item {
                id: delegateItem

                readonly property bool isCategory: model.isCategory

                x: isCategory ? 0 : 50
                width: parent.width
                height: model.parentExpanded ? 30 : 0
                visible: height > 0

                Behavior on height {
                    NumberAnimation {
                        duration: 100
                        easing.type: Easing.OutQuad
                    }
                }

                Row {
                    id: row

                    height: categoryIcon.height
                    spacing: 5

                    Image {
                        source: {
                            if (!delegateItem.isCategory)
                                return "";
                            model.expanded ? _resDir + "arrow_down.png"
                                           : _resDir + "arrow.png";
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: model.expanded = !model.expanded
                        }
                    }

                    Rectangle {
                        height: name.height
                        width: categoryIcon.width + name.width + 10

                        color: model.index === eventsList.currentIndex ? _selectionColor
                                                                       : "transparent"

                        Row {
                            id: textRow

                            spacing: 10
                            Image {
                                id: categoryIcon

                                source: model.icon
                            }

                            StyledLabel {
                                id: name
                                anchors.verticalCenter:  textRow.verticalCenter
                                text: model.name
                            }
                        }

                        MouseArea {
                            id: delegateArea

                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if (!delegateItem.isCategory)
                                    eventsList.currentIndex = model.index;
                            }
                            onEntered: itemDescription.text = model.description
                            onExited: itemDescription.text = ""
                            onDoubleClicked: {
                                if (!delegateItem.isCategory) {
                                    eventsList.currentIndex = model.index;
                                    _eventsBrowserView.close();
                                }
                            }
                        }
                    }
                }

            }
            onCurrentIndexChanged: _eventsBrowserView.selection = currentIndex

            Connections {
                target: _eventsBrowserView
                onSelectionChanged: {
                    if (eventsList.currentIndex !== _eventsBrowserView.selection)
                        eventsList.currentIndex = _eventsBrowserView.selection;
                }
            }
        }

        StyledMenuSeparator {
            bottomPadding: 0
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: _controlBaseHeight + 4
            Rectangle {
                anchors.fill: parent
                anchors.margins: 2

                color: _backgroundColor

                StyledLabel {
                    id: itemDescription
                    leftPadding: 6
                    anchors.fill: parent
                }
            }
        }
    }
}
