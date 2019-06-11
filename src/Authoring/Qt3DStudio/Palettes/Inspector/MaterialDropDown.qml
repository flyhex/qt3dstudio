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
import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2

import Qt3DStudio 1.0
import "../controls"
import "../Action"

StyledComboBox {
    property int instance: parent.modelData.instance
    property int handle: parent.modelData.handle
    property var values: parent.modelData.values
    property var value: parent.modelData.value
    property bool blockIndexChange: false
    property var callback

    model: values

    implicitWidth: _valueWidth
    implicitHeight: _controlBaseHeight

    Component.onCompleted: {
        currentIndex = find(value)
    }
    onCurrentIndexChanged: {
        var newValue = textAt(currentIndex)
        if (!blockIndexChange && value !== newValue && currentIndex !== -1)
            callback(instance, handle, newValue)
    }
    onValueChanged: {
        var newNewIndex = find(value);
        if (!blockIndexChange || newNewIndex > 0)
            currentIndex = newNewIndex;
        blockIndexChange = false;
    }
    onValuesChanged : {
        // Changing the values list will reset the currentIndex to zero, so block setting
        // the actual material. We'll get the proper index right after.
        if (currentIndex > 0)
            blockIndexChange = true;
    }
}
