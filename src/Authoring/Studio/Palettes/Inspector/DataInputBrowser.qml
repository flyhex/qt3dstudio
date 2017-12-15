/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

            model: _dataInputChooserView.model
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            currentIndex: _dataInputChooserView.selection

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
                            onClicked: {
                                if (_dataInputChooserView.selectable(model.index))
                                    browserList.currentIndex = model.index;
                            }
                            onDoubleClicked: {
                                if (_dataInputChooserView.selectable(model.index)) {
                                    browserList.currentIndex = model.index;
                                    _dataInputChooserView.close();
                                }
                            }
                        }
                    }
                }
            }

            onCurrentIndexChanged: _dataInputChooserView.selection = currentIndex

            Connections {
                target: _dataInputChooserView
                onSelectionChanged: {
                    if (browserList.currentIndex !== _dataInputChooserView.selection)
                        browserList.currentIndex = _dataInputChooserView.selection;
                }
            }
        }
    }
}
