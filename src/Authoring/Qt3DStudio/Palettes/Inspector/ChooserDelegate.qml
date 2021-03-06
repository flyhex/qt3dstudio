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
    id: item

    signal clicked(string filePath)
    signal doubleClicked(string filePath)

    width: parent.width
    height: 20
    color: isCurrentFile ? _selectionColor : "transparent"

    Row {
        x: depth * 28 - (item.width <= _valueWidth ? 14 : 0)
        anchors.verticalCenter: item.verticalCenter

        Image {
            source: _resDir + (expanded ? "arrow_down.png" : "arrow.png")
            opacity: isExpandable ? 1 : 0

            MouseArea {
                visible: listView.model && isExpandable
                anchors.fill: parent
                onClicked: {
                    if (expanded)
                        listView.model.collapse(index)
                    else
                        listView.model.expand(index)
                }
            }
        }

        Image {
            source: listView.model ? fileIcon : ""
            width: 16
            height: 16
        }

        StyledLabel {
            text: listView.model ? fileName : ""
            color: _textColor
            leftPadding: 5

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: {
                    if (isSelectable) {
                        listView.model.setCurrentFile(filePath);
                        item.clicked(filePath);
                    }
                }
                onDoubleClicked: {
                    if (isSelectable) {
                        listView.model.setCurrentFile(filePath);
                        item.doubleClicked(filePath);
                    } else if (isExpandable) {
                        if (expanded)
                            listView.model.collapse(index);
                        else
                            listView.model.expand(index);
                    }
                }
            }
        }
    }
}
