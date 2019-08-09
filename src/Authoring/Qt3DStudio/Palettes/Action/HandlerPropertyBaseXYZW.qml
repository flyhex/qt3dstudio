/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../controls"

/* Use for: Float4 */

ColumnLayout {
    id: root

    property alias valueX: textFieldX.text
    property alias valueY: textFieldY.text
    property alias valueZ: textFieldZ.text
    property alias valueW: textFieldW.text
    property int numberOfDecimal: 3
    property Item tabItem1: textFieldX
    property Item tabItem2: textFieldY
    property Item tabItem3: textFieldZ
    property Item tabItem4: textFieldW

    signal editingFinished
    signal previewValueChanged
    transformOrigin: Item.Center
    spacing: 0

    RowLayout {
        transformOrigin: Item.Center
        spacing: 0

        StyledLabel {
            Layout.preferredWidth: 10
            text: qsTr("X")
            color: _xAxisColor
        }

        FloatTextField {
            id: textFieldX
            Layout.preferredWidth: (_valueWidth - 50) / 3
            decimalValue: numberOfDecimal
            onEditingFinished: root.editingFinished()
            onPreviewValueChanged: root.previewValueChanged()
        }

        Item { width: 10 }

        StyledLabel {
            Layout.preferredWidth: 10
            text: qsTr("Y")
            color: _yAxisColor
        }

        FloatTextField {
            id: textFieldY
            Layout.preferredWidth: (_valueWidth - 50) / 3
            decimalValue: numberOfDecimal
            onEditingFinished: root.editingFinished()
            onPreviewValueChanged: root.previewValueChanged()
        }

        Item { width: 10 }

        StyledLabel {
            Layout.preferredWidth: 10
            text: qsTr("Z")
            color: _zAxisColor
        }

        FloatTextField {
            id: textFieldZ
            Layout.preferredWidth: (_valueWidth - 50) / 3
            decimalValue: numberOfDecimal
            onEditingFinished: root.editingFinished()
            onPreviewValueChanged: root.previewValueChanged()
        }
    }

    Item { height: 4 }

    RowLayout {
        transformOrigin: Item.Center
        spacing: 0
        Layout.leftMargin: -3

        StyledLabel {
            Layout.preferredWidth: 13
            text: qsTr("W")
            color: _wAxisColor
        }

        FloatTextField {
            id: textFieldW
            Layout.preferredWidth: (_valueWidth - 50) / 3
            decimalValue: numberOfDecimal
            onEditingFinished: root.editingFinished()
            onPreviewValueChanged: root.previewValueChanged()
        }
    }
}
