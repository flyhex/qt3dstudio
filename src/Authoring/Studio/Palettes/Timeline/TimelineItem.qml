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

Item {
    id: root

    property var timeInfo
    property bool selected: false
    property alias color: itemRect.color
    property string selectionColor: "black"
    property string borderColor: "black"

    x: timeInfo.lifeStart
    width: timeInfo.lifeEnd - timeInfo.lifeStart

    Row {
        Rectangle {
            width: timeInfo.startPosition - timeInfo.lifeStart
            height: root.height
            border.color: borderColor
            border.width: 1
            color: "gray" // TODO: use a real hashed rectangle
        }

        Rectangle {
            id: itemRect

            width: timeInfo.endPosition - timeInfo.startPosition
            height: root.height
            border.color: borderColor
            border.width: 1
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.lighter(root.color, 1.1)
                }

                GradientStop {
                    position: 0.5
                    color: root.color
                }

                GradientStop {
                    position: 1.0
                    color: Qt.darker(root.color, 1.1)
                }
            }
        }

        Rectangle {
            width: timeInfo.lifeEnd - timeInfo.endPosition
            height: root.height
            border.color: borderColor
            border.width: 1
            color: "gray" // TODO: use a real hashed rectangle
        }
    }

    Rectangle {
        id: selectionRect

        anchors {
            fill: parent
            topMargin: 3
            bottomMargin: 7
            leftMargin: 2
            rightMargin: 2
        }
        visible: root.selected
        color: selectionColor
    }
}
