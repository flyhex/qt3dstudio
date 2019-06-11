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
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3

RowLayout {
    id: root

    property alias color: rect.color
    property color selectedColor: "black"
    property bool listenToColorChanges: false

    signal colorSelected()
    signal previewColorSelected()

    Connections {
        target: _parentView
        onDialogCurrentColorChanged: {
            if (root.listenToColorChanges) {
                root.selectedColor = newColor;
                root.previewColorSelected();
            }
        }
    }

    Rectangle {
        id: rect

        width: _valueWidth / 4
        height: _controlBaseHeight

        border {
            width: 1
            color: _studioColor2
        }

        MouseArea {
            id: mouseArea

            anchors.fill: parent
            onClicked: {
                root.listenToColorChanges = true;
                _inspectorModel.suspendMaterialRename(true);
                root.selectedColor = _parentView.showColorDialog(rect.color, instance, handle);
                root.listenToColorChanges = false;
                _inspectorModel.suspendMaterialRename(false);
                root.colorSelected();
            }
        }

        Image {
            id: img
            // Source image size is 16x16 pixels
            x: parent.width - 18
            y: parent.height / 2 - 8
            source: _resDir + "arrow_down.png"
        }
    }

    Item {
        Layout.fillWidth: true
    }
}
