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

TextField {
    id: styledTextFieldId

    property alias backgroundColor: textBackground.color
    property bool ignoreHotkeys: true

    Layout.preferredWidth: _valueWidth
    Layout.preferredHeight: _controlBaseHeight
    horizontalAlignment: TextInput.AlignRight
    verticalAlignment: TextInput.AlignVCenter

    topPadding: 0
    bottomPadding: 0
    rightPadding: 6

    selectByMouse: true
    selectionColor: _selectionColor
    selectedTextColor: _textColor

    font.pixelSize: _fontSize
    color: _textColor
    background: Rectangle {
        id: textBackground
        color: styledTextFieldId.enabled ? _studioColor2 : "transparent"
        border.width: styledTextFieldId.activeFocus ? 1 : 0
        border.color: styledTextFieldId.activeFocus ? _selectionColor : _disabledColor
    }

    onActiveFocusChanged: {
        if (activeFocus)
            selectAll();
    }
}
