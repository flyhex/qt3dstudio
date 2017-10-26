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

import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.3
import "../controls"

/* Use for: Tiling ... */

GridLayout {
    id: root

    property alias valueX: textFieldX.text
    property alias valueY: textFieldY.text
    property int numberOfDecimal: 3
    property Item tabItem1: textFieldX
    property Item tabItem2: textFieldY

    columns: 2
    rowSpacing: 1

    signal editingFinished
    signal previewValueChanged

    StyledLabel {
        Layout.preferredWidth: 10
        text: qsTr("X")
    }

    FloatTextField {
        id: textFieldX

        decimalValue: numberOfDecimal
        onEditingFinished: root.editingFinished()
        onPreviewValueChanged: root.previewValueChanged()
    }

    StyledLabel {
        Layout.preferredWidth: 10
        text: qsTr("Y")
    }

    FloatTextField {
        id: textFieldY

        decimalValue: numberOfDecimal
        onEditingFinished: root.editingFinished()
        onPreviewValueChanged: root.previewValueChanged()
    }
}

