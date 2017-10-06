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
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.3
import Qt3DStudio 1.0
import "../controls"

Rectangle {
    id: root

    color: _backgroundColor

    Flickable {
        id: actionFlickable
        ScrollBar.vertical: ScrollBar {
            id: scrollBar
            visible: size < 1.0
        }

        anchors.fill: parent
        contentHeight: contentColumn.height

        property bool scrollToBottom: false

        onContentHeightChanged: {
            if (scrollToBottom) {
                scrollToBottom = false;
                if (contentHeight > height)
                    contentY = contentHeight - height;
            }
        }

        Column {
            id: contentColumn
            width: parent.width
            spacing: 4

            RowLayout {
                height: _controlBaseHeight + anchors.margins * 2
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 4
                anchors.rightMargin: 12

                Image {
                    id: headerImage
                    source: _actionView.itemIcon !== "" ? _resDir + _actionView.itemIcon : ""
                }

                StyledLabel {
                    Layout.fillWidth: true
                    text: _actionView.itemText
                    color: _actionView.itemColor
                }

                StyledToolButton {
                    enabled: actionsList.currentIndex !== -1
                    enabledImage: "Action-Trash-Normal.png"
                    disabledImage: "Action-Trash-Disabled.png"
                    toolTipText: qsTr("Delete selected action")

                    onClicked: _actionView.deleteAction(actionsList.currentIndex)
                }

                StyledToolButton {
                    enabledImage: "add.png"
                    toolTipText: qsTr("Add new action")

                    onClicked: _actionView.addAction()
                }
            }
            ListView {
                id: actionsList
                width: parent.width
                height: count * _controlBaseHeight
                clip: true

                boundsBehavior: Flickable.StopAtBounds
                model: _actionView.actionsModel

                delegate: Rectangle {
                    id: delegateItem

                    width: actionsList.width
                    height: _controlBaseHeight
                    color: model.index === actionsList.currentIndex ? _selectionColor
                                                                    : "transparent"

                    Row {
                        x: 10
                        y: 5
                        height: parent.height
                        width: parent.width - x
                        spacing: 4

                        Image {
                            id: visibilityIcon

                            source: model.visible ? _resDir + "Toggle-HideShow.png"
                                                  : _resDir + "Toggle-HideShow-disabled.png"

                            MouseArea {
                                anchors.fill: parent
                                onClicked: model.visible = !model.visible
                            }
                        }

                        StyledLabel {
                            text: model.description
                        }
                    }

                    MouseArea {
                        anchors.fill: parent

                        acceptedButtons: Qt.LeftButton | Qt.RightButton

                        onClicked: {
                            actionFlickable.scrollToBottom = false;
                            actionsList.currentIndex = model.index;
                            _actionView.setCurrentActionIndex(model.index);
                            if (mouse.button == Qt.LeftButton && mouse.x < visibilityIcon.width + 10)
                                model.visible = !model.visible;

                            if (mouse.button == Qt.RightButton) {
                                var updateMousePosition = mapToItem(actionsList, mouse.x, mouse.y)
                                _actionView.showContextMenu(updateMousePosition.x, updateMousePosition.y);
                            }
                        }
                        onDoubleClicked: {
                            actionFlickable.scrollToBottom = false;
                            if (mouse.button == Qt.LeftButton && mouse.x > visibilityIcon.width + 10) {
                                // Scroll down to bottom to show properties on double click
                                if (actionFlickable.contentHeight > actionFlickable.height) {
                                    actionFlickable.contentY = (actionFlickable.contentHeight
                                                                - actionFlickable.height)
                                }
                                // Since loading new property fields takes a moment, we want
                                // to keep the view scrolled to bottom
                                // when the content height changes the next time.
                                actionFlickable.scrollToBottom = true;
                            }
                        }
                    }
                }

                onCountChanged: {
                    if (currentIndex >= count)
                        currentIndex = count - 1;
                }

                onCurrentIndexChanged: _actionView.setCurrentActionIndex(currentIndex);
            }

            StyledMenuSeparator {
                leftPadding: 12
                rightPadding: 12
            }

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                height: childrenRect.height
                visible: actionsList.currentIndex !== -1
                spacing: 4

                RowLayout {
                    x: 12
                    StyledLabel {
                        text: qsTr("Trigger Object")
                    }
                    BrowserCombo {
                        value: _actionView.triggerObjectName
                        onShowBrowser: activeBrowser = _actionView.showTriggerObjectBrowser(
                                           mapToGlobal(width, 0));
                    }
                }

                RowLayout {
                    x: 12
                    StyledLabel {
                        text: qsTr("Event")
                    }
                    BrowserCombo {
                        value: _actionView.eventName
                        onShowBrowser: activeBrowser = _actionView.showEventBrowser(
                                           mapToGlobal(width, 0))
                    }
                }
            }

            StyledMenuSeparator {
                visible: actionsList.currentIndex !== -1
                leftPadding: 12
                rightPadding: 12
            }

            Column {
                visible: actionsList.currentIndex !== -1
                width: parent.width
                height: childrenRect.height
                spacing: 4

                RowLayout {
                    x: 12
                    StyledLabel {
                        text: qsTr("Target Object")
                    }

                    BrowserCombo {
                        value: _actionView.targetObjectName
                        onShowBrowser: activeBrowser = _actionView.showTargetObjectBrowser(
                                           mapToGlobal(width, 0))
                    }
                }

                RowLayout {
                    x: 12
                    StyledLabel {
                        text: qsTr("Handler")
                    }

                    BrowserCombo {
                        value: _actionView.handlerName
                        onShowBrowser: activeBrowser = _actionView.showHandlerBrowser(
                                           mapToGlobal(width, 0))
                    }
                }

                Component {
                    id: genericHandlerComponent

                    HandlerGenericText {
                        label: parent && parent.argument.name ? parent.argument.name : ""
                        value: parent && parent.argument.value ? parent.argument.value : ""

                        onEditingFinished: {
                            if (parent)
                                _actionView.setArgumentValue(parent.argument.handle, value)
                        }
                    }
                }

                Component {
                    id: floatHandlerComponent

                    HandlerGenericText {
                        label: parent && parent.argument.name ? parent.argument.name : ""
                        value: parent && parent.argument.value ? parent.argument.value : 0.0
                        validator: DoubleValidator {
                            decimals: 3
                            notation: DoubleValidator.StandardNotation
                        }

                        onEditingFinished: {
                            if (parent)
                                _actionView.setArgumentValue(parent.argument.handle, value)
                        }
                    }
                }

                Component {
                    id: signalHandlerComponent

                    HandlerGenericText {
                        label: parent && parent.argument.name ? parent.argument.name : ""
                        value: parent && parent.argument.value ? parent.argument.value : ""

                        onEditingFinished: {
                            if (parent)
                                _actionView.setArgumentValue(parent.argument.handle, value);
                        }
                    }
                }

                Component {
                    id: eventHandlerComponent

                    HandlerFireEvent {
                        label: parent && parent.argument.name ? parent.argument.name : ""
                        value: _actionView.firedEvent === "" ? qsTr("[Unknown Event]")
                                                             : _actionView.firedEvent

                        onShowBrowser: {
                            if (parent && parent.argument.handle) {
                                activeBrowser = _actionView.showEventBrowserForArgument(
                                            parent.argument.handle, mapToGlobal(width, 0))
                            }
                        }
                    }
                }

                Component {
                    id: slideHandlerComponent

                    HandlerGoToSlide {
                        slideModel: _actionView.slideNames()
                        defaultSlideIndex: parent && parent.argument.value ? _actionView.slideNameToIndex(parent.argument.value)
                                                                           : 0

                        onIndexChanged: {
                            if (parent && parent.argument.handle && currentSlide)
                                _actionView.setArgumentValue(parent.argument.handle, currentSlide)
                        }
                    }
                }

                Component {
                    id: checkboxHandlerComponent

                    HandlerGenericCheckbox {
                        label: parent && parent.argument.name ? parent.argument.name : ""
                        checked: parent && parent.argument.value ? parent.argument.value : false

                        onClicked: {
                            if (parent && parent.argument.handle)
                                _actionView.setArgumentValue(parent.argument.handle, !checked)
                        }
                    }
                }

                Component {
                    id: propertyHandlerComponent

                    HandlerProperty {
                        propertyModel: _actionView.propertyModel
                        defaultPropertyIndex: propertyModel ? propertyModel.defaultPropertyIndex : 0

                        onPropertySelected: {
                            if (parent && parent.argument.handle)
                                _actionView.setCurrentPropertyIndex(parent.argument.handle, index);
                        }
                    }
                }

                Repeater {
                    model: _actionView.handlerArguments.length

                    Loader {
                        x: 12

                        readonly property var argument:_actionView.handlerArguments[model.index]

                        onLoaded: {
                            // HandlerProperty does its own tab order handling
                            if (argument.type !== HandlerArgumentType.Property) {
                                // Dynamic actions use group 0.
                                // We assume there is always just one tabbable argument per action,
                                // and the rest are dependent types.
                                _tabOrderHandler.clear();
                                if (item.tabItem1 !== undefined) {
                                    _tabOrderHandler.addItem(0, item.tabItem1)
                                    if (item.tabItem2 !== undefined) {
                                            _tabOrderHandler.addItem(0, item.tabItem2)
                                        if (item.tabItem3 !== undefined)
                                            _tabOrderHandler.addItem(0, item.tabItem3)
                                    }
                                }
                            }
                        }

                        sourceComponent: {
                            const handlerType = argument.type;
                            switch (handlerType) {
                            case HandlerArgumentType.None:
                                switch (argument.completeType) {
                                case CompleteMetaDataType.Boolean:
                                    return checkboxHandlerComponent;
                                case CompleteMetaDataType.Float:
                                    return floatHandlerComponent;
                                default:
                                    return genericHandlerComponent;
                                }
                            case HandlerArgumentType.Event:
                                return eventHandlerComponent;
                            case HandlerArgumentType.Property:
                                return propertyHandlerComponent;
                            case HandlerArgumentType.Dependent:
                                return null; // no UI for Dependent type, they are the value for a property
                            case HandlerArgumentType.Signal:
                                return signalHandlerComponent;
                            case HandlerArgumentType.Slide:
                                return slideHandlerComponent
                            default: console.warn("KDAB_TODO implement handler for type: ", handlerType)
                            }
                            return null;
                        }
                    }
                }
            }

            StyledMenuSeparator {
                visible: actionsList.count > 0 && actionsList.currentIndex !== -1
                leftPadding: 12
                rightPadding: 12
            }
        }
    }
}
