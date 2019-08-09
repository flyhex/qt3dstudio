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

RowLayout {
    id: root

    property alias valueX: propertyXYZW.valueX
    property alias valueY: propertyXYZW.valueY
    property alias valueZ: propertyXYZW.valueZ
    property alias valueW: propertyXYZW.valueW
    property alias label: labelItem.text
    property alias tabItem1: propertyXYZW.tabItem1
    property alias tabItem2: propertyXYZW.tabItem2
    property alias tabItem3: propertyXYZW.tabItem3
    property alias tabItem4: propertyXYZW.tabItem4
    property alias numberOfDecimal: propertyXYZW.numberOfDecimal

    signal editingFinished
    signal previewValueChanged

    StyledLabel {
        id: labelItem
        Layout.alignment: Qt.AlignTop | Qt.AlignLeft
        text: qsTr("New Value")
    }

    HandlerPropertyBaseXYZW {
        id: propertyXYZW
        Layout.alignment: Qt.AlignRight

        onEditingFinished: root.editingFinished()
        onPreviewValueChanged: root.previewValueChanged()
    }
}
