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
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../controls"

/* Use for: Position, Rotation, Scale, Pivot ... */

RowLayout {
    id: root

    property alias valueX: propertyXYZ.valueX
    property alias valueY: propertyXYZ.valueY
    property alias valueZ: propertyXYZ.valueZ
    property alias label: labelItem.text
    property alias tabItem1: propertyXYZ.tabItem1
    property alias tabItem2: propertyXYZ.tabItem2
    property alias tabItem3: propertyXYZ.tabItem3
    property alias numberOfDecimal: propertyXYZ.numberOfDecimal

    signal editingFinished
    signal previewValueChanged
    signal triggerUndo
    signal triggerRedo

    StyledLabel {
        id: labelItem
        Layout.alignment: Qt.AlignTop | Qt.AlignLeft
        text: qsTr("New Value")
    }

    HandlerPropertyBaseXYZ {
        id: propertyXYZ
        Layout.alignment: Qt.AlignRight

        onEditingFinished: root.editingFinished()
        onPreviewValueChanged: root.previewValueChanged()
        onTriggerUndo: root.triggerUndo()
        onTriggerRedo: root.triggerRedo()
    }
}
