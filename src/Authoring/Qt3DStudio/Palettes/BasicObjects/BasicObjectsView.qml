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
import "../controls"

Rectangle {

    color: _backgroundColor

    ListView {
        anchors {
            fill: parent
            leftMargin: 8
        }
        boundsBehavior: Flickable.StopAtBounds

        model: _basicObjectsModel
        spacing: 2

        ScrollBar.vertical: ScrollBar {}

        delegate: Item {
            height: contentRow.height
            width: contentRow.width
            Item {
                id: dragItem
                anchors.fill: parent

                MouseArea {
                    id: dragArea
                    property bool dragStarted: false
                    property point pressPoint

                    anchors.fill: parent
                    onPressed: {
                        pressPoint = Qt.point(mouse.x, mouse.y);
                        dragStarted = false;
                    }
                    onPositionChanged: {
                        if (!dragStarted && (Math.abs(mouse.x - pressPoint.x) > 4
                                             || Math.abs(mouse.y - pressPoint.y) > 4)) {
                            dragStarted = true;
                            _basicObjectsView.startDrag(dragItem, index);
                        }
                    }
                }
            }
            Row {
                id: contentRow
                spacing: 4
                Image {
                    id: assetIcon
                    width: 24
                    height: 24
                    fillMode: Image.Pad
                    source: model.icon
                }
                StyledLabel {
                    y: (assetIcon.height - height) / 2
                    text: model.name
                }
            }
        }
    }
}
