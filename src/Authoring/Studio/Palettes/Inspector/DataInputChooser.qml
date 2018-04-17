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
import "../controls"

Rectangle {
    id: root

    color: _backgroundColor

    border.color: _studioColor3

    StyledLabel {
        id: title
        color: _dataInputColor
        text: qsTr("Select Controlling Data Input")
        leftPadding: 8
        height: 20
    }

    StyledMenuSeparator {
        id: separator
        anchors.top: title.bottom
        leftPadding: 8
        rightPadding: 8
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 30
        spacing: 10

        ListView {
            id: listView
            Layout.leftMargin: 8
            Layout.fillHeight: true
            Layout.fillWidth: true

            boundsBehavior: Flickable.StopAtBounds
            spacing: 4
            clip: true

            ScrollBar.vertical: ScrollBar {}

            model: _dataInputSelectModel

            delegate:  Row {
                height: 20
                Image {
                    // do not show item icon for fixed items
                    visible: index >= _dataInputSelectModel.fixedItemCount
                    source: index === _dataInputSelectView.selected
                                      ? _dataInputSelectModel.getActiveIconPath()
                                      : _dataInputSelectModel.getInactiveIconPath();
                }
                StyledLabel {
                    leftPadding: 5
                    text: model.display
                    width: listView.width / 2;
                    color: (index >= _dataInputSelectModel.fixedItemCount)
                           && (index === _dataInputSelectView.selected)
                           ? _dataInputColor : _textColor;

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onClicked: _dataInputSelectView.setSelection(index)
                    }
                }
                StyledLabel {
                    leftPadding: 5
                    visible: index >= _dataInputSelectModel.fixedItemCount
                    text:  "(" + model.datatype + ")"
                    color: (index >= _dataInputSelectModel.fixedItemCount)
                           && (index === _dataInputSelectView.selected)
                           ? _dataInputColor : _textColor;

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onClicked: _dataInputSelectView.setSelection(index)
                    }
                }
            }
        }
    }
}

