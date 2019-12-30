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

import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../controls"

RowLayout {
    id: root

    property alias label: labelField.text
    property real desiredValue: Number(floatField.text)
    property real value: 0
    property int numberOfDecimal: 3
    property Item tabItem1: floatField

    signal editingFinished
    signal previewValueChanged
    signal triggerUndo
    signal triggerRedo

    onValueChanged: {
        // FloatTextField can set its text internally, thus breaking the binding, so
        // let's set the text value explicitly each time value changes
        floatField.text = Number(value).toFixed(numberOfDecimal);
    }

    StyledLabel {
        id: labelField
    }

    FloatTextField {
        id: floatField
        Layout.preferredWidth: _valueWidth
        decimalValue: numberOfDecimal
        onEditingFinished: root.editingFinished()
        onPreviewValueChanged: root.previewValueChanged()
        onTriggerUndo: root.triggerUndo()
        onTriggerRedo: root.triggerRedo()
    }
}
