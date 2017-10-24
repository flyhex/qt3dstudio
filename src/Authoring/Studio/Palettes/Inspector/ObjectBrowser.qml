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
import Qt3DStudio 1.0
import "../controls"

Rectangle {
    id: root

    property alias selectedText: selectionText.text

    color: _backgroundColor
    border.color: _studioColor3

    ColumnLayout {
        anchors.fill: parent

        spacing: 10

        ListView {
            id: browserList

            Layout.margins: 10
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 80
            Layout.preferredHeight: count * 20
            Layout.preferredWidth: root.width

            ScrollBar.vertical: ScrollBar {}

            model: _objectBrowserView.model
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            currentIndex: _objectBrowserView.selection

            delegate: Item {
                id: delegateItem

                x: model.depth * 20
                width: parent.width
                height: model.parentExpanded ? typeIcon.height + 10 : 0

                visible: height > 0

                Behavior on height {
                    NumberAnimation {
                        duration: 100
                        easing.type: Easing.OutQuad
                    }
                }

                Row {
                    id: row

                    height: typeIcon.height
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

                    Rectangle {
                        height: typeIcon.height
                        width: typeIcon.width + name.width + 10

                        color: model.index === browserList.currentIndex ? _selectionColor
                                                                        : "transparent"

                        Row {
                            spacing: 10
                            Image {
                                id: typeIcon

                                source: model.icon
                            }

                            StyledLabel {
                                id: name
                                anchors.verticalCenter:  typeIcon.verticalCenter
                                color: model.textColor
                                text: model.name
                            }
                        }

                        MouseArea {
                            id: delegateArea

                            anchors.fill: parent
                            onClicked: browserList.currentIndex = model.index
                            onDoubleClicked: {
                                browserList.currentIndex = model.index;
                                _objectBrowserView.close();
                            }
                        }
                    }
                }
            }

            onCurrentIndexChanged: _objectBrowserView.selection = currentIndex

            Connections {
                target: _objectBrowserView
                onSelectionChanged: {
                    if (browserList.currentIndex !== _objectBrowserView.selection)
                        browserList.currentIndex = _objectBrowserView.selection;
                }
            }
        }

        StyledMenuSeparator {}

        GridLayout {
            columns: 2
            Layout.margins: 10

            StyledLabel {
                text: qsTr("Type")
            }

            StyledComboBox {
                id: pathCombo
                model: [qsTr("Absolute Reference"), qsTr("Path Reference")]

                onActivated: {
                    if (index === 0)
                        _objectBrowserView.pathType = ObjectBrowserView.Name;
                    else if (index === 1)
                        _objectBrowserView.pathType = ObjectBrowserView.Relative;
                }
            }

            StyledLabel {
                text: qsTr("Path")
            }

            StyledLabel {
                id: selectionText
                Layout.preferredWidth: _valueWidth
                text: pathCombo.currentIndex === 0 ? _objectBrowserView.name(browserList.currentIndex)
                                                   : _objectBrowserView.path(browserList.currentIndex)
            }
        }
    }
}
