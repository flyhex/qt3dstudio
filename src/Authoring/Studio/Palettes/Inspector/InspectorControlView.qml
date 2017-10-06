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
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2

import Qt3DStudio 1.0
import "../controls"
import "../Action"

Rectangle {
    id: root
    color: _backgroundColor

    Connections {
       target: _inspectorModel
       onModelAboutToBeReset: {
           _tabOrderHandler.clear();
       }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 4
        spacing: 8

        RowLayout {
            height: _controlBaseHeight + anchors.margins * 2
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 4

            Image {
                id: headerImage
                source: _inspectorView.titleIcon !== "" ? _resDir + _inspectorView.titleIcon : ""
            }

            StyledLabel {
                text: _inspectorView.titleText
                color: _inspectorView.titleColor()
            }
        }

        ListView {
            id: groupElements

            Layout.fillWidth: true
            Layout.fillHeight: true

            spacing: 4

            clip: true

            ScrollBar.vertical: ScrollBar {
                visible: size < 1.0
            }

            model: _inspectorModel
            delegate: Rectangle {
                id: delegateItem

                property int indexOfThisDelegate: index

                width: parent.width
                height: items.height
                color: "transparent";

                readonly property var values: model.values

                Column {
                    id: items

                    x: 10
                    width: parent.width - x
                    spacing: 4

                    StyledLabel {
                        text: model.title
                    }

                    Column {
                        spacing: 4

                        Repeater {
                            model: delegateItem.values

                            onItemAdded: {
                                if (index === 0)
                                    _tabOrderHandler.clearGroup(indexOfThisDelegate);
                                if (item.loadedItem.tabItem1 !== undefined) {
                                    _tabOrderHandler.addItem(indexOfThisDelegate,
                                                             item.loadedItem.tabItem1)
                                    if (item.loadedItem.tabItem2 !== undefined) {
                                            _tabOrderHandler.addItem(
                                                indexOfThisDelegate,
                                                item.loadedItem.tabItem2)
                                        if (item.loadedItem.tabItem3 !== undefined) {
                                            _tabOrderHandler.addItem(
                                                        indexOfThisDelegate,
                                                        item.loadedItem.tabItem3)
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                id: groupDelegateItem
                                spacing: 0

                                property alias loadedItem: loader.item

                                function showContextMenu(coords) {
                                    _inspectorView.showContextMenu(
                                                coords.x, coords.y,
                                                model.modelData.handle,
                                                model.modelData.instance);
                                    // Refresh text; title color is wrong after this
                                    propertyRow.color = _inspectorView.titleColor(
                                                modelData.instance, modelData.handle);
                                }

                                Item {
                                    Layout.alignment: Qt.AlignTop
                                    width: animatedPropertyButton.sourceSize.width
                                    height: _controlBaseHeight
                                    visible: model.modelData.animatable
                                    Image {
                                        id: animatedPropertyButton

                                        property bool animated: model.modelData.animated

                                        anchors.fill: parent
                                        fillMode: Image.Pad

                                        source: {
                                            _resDir + (animated
                                                       ? "Inspector-AnimateToggle-Active.png"
                                                       : "Inspector-AnimateToggle-Normal.png")
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            acceptedButtons: Qt.RightButton | Qt.LeftButton
                                            onClicked:  {
                                                if (mouse.button === Qt.LeftButton) {
                                                    _inspectorModel.setPropertyAnimated(
                                                                model.modelData.instance,
                                                                model.modelData.handle,
                                                                !model.modelData.animated)
                                                } else {
                                                    const coords = mapToItem(root, mouse.x, mouse.y);
                                                    groupDelegateItem.showContextMenu(coords);
                                                 }
                                            }
                                        }
                                    }
                                }


                                Item {
                                    // Spacer item
                                    width: model.modelData.animatable
                                           ? 4 : animatedPropertyButton.width + 4
                                    height: loadedItem.height + 4 // Add little space between items
                                }

                                StyledLabel {
                                    id: propertyRow

                                    readonly property var modelData: model.modelData
                                    text: model.modelData.title
                                    color: _inspectorView.titleColor(modelData.instance,
                                                                     modelData.handle)

                                    Layout.alignment: Qt.AlignTop

                                    MouseArea {
                                        id: mouse
                                        anchors.fill: parent
                                        acceptedButtons: Qt.RightButton
                                        hoverEnabled: true
                                        onClicked:  {
                                            const coords = mapToItem(root, mouse.x, mouse.y);
                                            groupDelegateItem.showContextMenu(coords);
                                        }
                                    }

                                    StyledTooltip {
                                        text: modelData.toolTip
                                        visible: mouse.containsMouse
                                    }
                                }

                                Loader {
                                    id: loader
                                    readonly property var modelData: propertyRow.modelData
                                    sourceComponent: {
                                        const dataType = modelData.dataType;
                                        console.warn("KDAB_TODO: DEBUG for type", dataType,
                                                     "property", modelData.propertyType,
                                                     "text ", model.modelData.title);
                                        switch (dataType) {
                                        case DataModelDataType.Long:
                                            if (modelData.propertyType ===
                                                    AdditionalMetaDataType.ShadowMapResolution) {
                                                return shadowResolutionComponent;
                                            }
                                            if (modelData.propertyType === AdditionalMetaDataType.Range) {
                                                return intSliderComponent;
                                            }
                                            console.warn("KDAB_TODO: implement handler for type \"Long\", property:",
                                                         modelData.propertyType);
                                            return null;
                                        case DataModelDataType.Long4:
                                            if (modelData.propertyType === AdditionalMetaDataType.Image) {
                                                return imageChooser;
                                            }
                                            console.warn("KDAB_TODO: implement handler for type \"long4\" property:",
                                                         modelData.propertyType);
                                            return null;
                                        case DataModelDataType.ObjectRef:
                                            if (modelData.propertyType === AdditionalMetaDataType.ObjectRef)
                                                return objectReference;
                                            console.warn("KDAB_TODO: implement handler for type: \"objectref\" property:",
                                                         modelData.propertyType);
                                            return null;
                                        case DataModelDataType.StringOrInt:
                                            //TODO: Maybe do some further check if the right combo is used
                                            if (modelData.propertyType === AdditionalMetaDataType.StringList)
                                                return slideSelectionDropDown;
                                            console.warn("KDAB_TODO: (String) implement handler for type \"stringOrInt\" property:",
                                                         modelData.propertyType);
                                            return null;
                                        case DataModelDataType.String:
                                            if (modelData.propertyType === AdditionalMetaDataType.Import)
                                                return fileChooser;
                                            if (modelData.propertyType === AdditionalMetaDataType.StringList)
                                                return comboDropDown;
                                            if (modelData.propertyType === AdditionalMetaDataType.Renderable)
                                                return renderableDropDown;
                                            if (modelData.propertyType === AdditionalMetaDataType.Mesh)
                                                return meshChooser;
                                            if (modelData.propertyType === AdditionalMetaDataType.MultiLine)
                                                return multiLine;
                                            if (modelData.propertyType === AdditionalMetaDataType.Font)
                                                return comboDropDown;
                                            if (modelData.propertyType === AdditionalMetaDataType.Texture)
                                                return textureChooser;
                                            console.warn("KDAB_TODO: (String) implement handler for type \"string\" property:",
                                                         modelData.propertyType);
                                            return null;
                                        case DataModelDataType.Bool:
                                            return checkBox;
                                        case DataModelDataType.Float:
                                            if (modelData.propertyType === AdditionalMetaDataType.None)
                                                return valueComponent;
                                            if (modelData.propertyType === AdditionalMetaDataType.Range)
                                                return sliderComponent;
                                            if (modelData.propertyType === AdditionalMetaDataType.FontSize)
                                                return fontSizeComponent;
                                            console.warn("KDAB_TODO: implement handler for type\"float\" property:",
                                                         modelData.propertyType);
                                            return null;
                                        case DataModelDataType.Float3:
                                            if (modelData.propertyType === AdditionalMetaDataType.Color)
                                                return colorBox;
                                            if (modelData.propertyType === AdditionalMetaDataType.Rotation)
                                                return xyzPropertyComponent;
                                            if (modelData.propertyType === AdditionalMetaDataType.None)
                                                return xyzPropertyComponent;
                                            console.warn("KDAB_TODO: implement handler for type:\"float3\" property:",
                                                         modelData.propertyType, "text ",
                                                         model.modelData.title);
                                            return null;
                                        case DataModelDataType.StringRef:
                                            if (modelData.propertyType === AdditionalMetaDataType.None)
                                                return materialDropDown;
                                            console.warn("KDAB_TODO: implement handler for type:\"StringRef\" text ",
                                                         model.modelData.title);
                                            return null;
                                        default:
                                            console.warn("KDAB_TODO: implement handler for type",
                                                         dataType, "property",
                                                         modelData.propertyType, "text ",
                                                         model.modelData.title);
                                        }
                                        return null;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: multiLine

        ScrollView {
            id: scrollView
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant value: parent.modelData.value
            property Item tabItem1: textArea

            width: _valueWidth
            height: _controlBaseHeight * 3
            clip: true

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            TextArea {
                id: textArea

                text: scrollView.value
                horizontalAlignment: TextInput.AlignLeft
                verticalAlignment: TextInput.AlignTop
                font.pixelSize: _fontSize
                color: _textColor
                wrapMode: TextEdit.WordWrap
                selectionColor: _selectionColor
                selectedTextColor: _textColor

                topPadding: 6
                bottomPadding: 6
                rightPadding: 6

                background: Rectangle {
                    color: textArea.enabled ? _studioColor2 : "transparent"
                    border.width: textArea.activeFocus ? 1 : 0
                    border.color: textArea.activeFocus ? _selectionColor : _disabledColor
                }

                MouseArea {
                    id: mouseAreaX
                    anchors.fill: parent
                    property int clickedPos
                    preventStealing: true

                    onPressed: {
                        textArea.forceActiveFocus()
                        clickedPos = textArea.positionAt(mouse.x, mouse.y)
                        textArea.cursorPosition = clickedPos
                    }
                    onDoubleClicked: textArea.selectAll()
                    onPositionChanged: {
                        textArea.cursorPosition = textArea.positionAt(mouse.x, mouse.y)
                        textArea.select(clickedPos, textArea.cursorPosition)
                    }
                }

                onTextChanged: {
                    _inspectorModel.setPropertyValue(scrollView.instance, scrollView.handle,
                                                     text, false)
                }

                onEditingFinished: {
                    _inspectorModel.setPropertyValue(scrollView.instance, scrollView.handle,
                                                     text, true)
                }
            }
        }
    }

    Component {
        id: meshChooser
        HandlerFilesChooser {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant values: parent.modelData.values
            value: parent.modelData.value
            onShowBrowser: {
                activeBrowser = _inspectorView.showMeshChooser(handle, instance,
                                                               mapToGlobal(width, 0))
            }
        }
    }

    Component {
        id: imageChooser
        HandlerFilesChooser {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant values: parent.modelData.values
            value: parent.modelData.value
            onShowBrowser: {
                activeBrowser = _inspectorView.showImageChooser(handle, instance,
                                                                mapToGlobal(width, 0))
            }
        }
    }

    Component {
        id: fileChooser
        HandlerFilesChooser {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant values: parent.modelData.values
            value: parent.modelData.value === "" ? qsTr("[None]") : parent.modelData.value
            onShowBrowser: {
                activeBrowser = _inspectorView.showFilesChooser(handle, instance,
                                                                mapToGlobal(width, 0))
            }
        }
    }

    Component {
        id: textureChooser
        HandlerFilesChooser {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant values: parent.modelData.values
            value: parent.modelData.value
            onShowBrowser: {
                activeBrowser = _inspectorView.showTextureChooser(handle, instance,
                                                                  mapToGlobal(width, 0))
            }
        }
    }

    Component {
        id: xyzPropertyComponent

        RowLayout {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant values: parent.modelData.values
            property alias tabItem1: xyzHandler.tabItem1
            property alias tabItem2: xyzHandler.tabItem2
            property alias tabItem3: xyzHandler.tabItem3
            spacing: 0
            Item {
                width: _valueWidth - xyzHandler.width
            }
            HandlerPropertyBaseXYZ {
                id: xyzHandler
                valueX: Number(values[0]).toFixed(3)
                valueY: Number(values[1]).toFixed(3)
                valueZ: Number(values[2]).toFixed(3)
                onEditingFinished: {
                    _inspectorModel.setPropertyValue(parent.instance, parent.handle,
                                                     Qt.vector3d(valueX, valueY, valueZ))
                }
            }
        }
    }

    Component {
        id: valueComponent

        RowLayout {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property real value: Number(parent.modelData.value).toFixed(3)
            property Item tabItem1: floatField
            spacing: 0
            Item {
                width: _valueWidth - floatField.width
            }
            FloatTextField {
                id: floatField
                text: parent.value
                implicitHeight: _controlBaseHeight
                implicitWidth: _valueWidth / 2

                onWheelEventFinished: {
                    _inspectorModel.setPropertyValue(parent.instance, parent.handle, Number(text));
                }

                onEditingFinished: {
                    _inspectorModel.setPropertyValue(parent.instance, parent.handle, Number(text));
                }
            }
        }
    }

    Component {
        id: sliderComponent

        HandlerPropertyBaseSlider {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant values: parent.modelData.values
            property real oldValue: 0.0

            value: parent.modelData.value
            sliderMin: values[0]
            sliderMax: values[1]

            onEditingStarted: {
                oldValue = value
            }
            onEditingFinished: {
                // make sure the undo step is created, therefore resetting it to the old value
                var val = value
                _inspectorModel.setPropertyValue(instance, handle, oldValue, false)
                _inspectorModel.setPropertyValue(instance, handle, val, true)
            }
            onSliderMoved: {
                _inspectorModel.setPropertyValue(instance, handle, value, false)
            }
        }
    }

    Component  {
        id: comboDropDown

        StyledComboBox {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property var values: parent.modelData.values
            property var value: parent.modelData.value

            model: values

            implicitWidth: _valueWidth
            implicitHeight: _controlBaseHeight

            Component.onCompleted: {
                currentIndex = find(value)
            }
            onCurrentIndexChanged: {
                var newValue = textAt(currentIndex)
                if (value !== newValue && currentIndex !== -1)
                    _inspectorModel.setPropertyValue(instance, handle, newValue)
            }
            onValueChanged: {
                currentIndex = find(value)
            }
        }
    }

    Component  {
        id: slideSelectionDropDown

        StyledComboBox {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property var values: parent.modelData.values
            property var value: parent.modelData.value

            model: values

            implicitWidth: _valueWidth
            implicitHeight: _controlBaseHeight

            Component.onCompleted: {
                var newIndex = find(value)
                if (newIndex === -1)
                    newIndex = find(value + "|separator")
                currentIndex = newIndex
            }
            onCurrentIndexChanged: {
                var newValue = textAt(currentIndex).replace("|separator", "")
                if (value !== newValue && currentIndex !== -1) {
                    _inspectorModel.setSlideSelection(instance, handle,
                                                      currentIndex, values)
                }
            }
            onValueChanged: {
                var newIndex = find(value)
                if (newIndex === -1)
                    newIndex = find(value + "|separator")
                currentIndex = newIndex
            }
        }
    }

    Component  {
        id: materialDropDown

        StyledComboBox {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property var values: parent.modelData.values
            property var value: parent.modelData.value

            model: values

            implicitWidth: _valueWidth
            implicitHeight: _controlBaseHeight

            Component.onCompleted: {
                currentIndex = find(value)
            }
            onCurrentIndexChanged: {
                var newValue = textAt(currentIndex)
                if (value !== newValue && currentIndex !== -1)
                    _inspectorModel.setMaterialTypeValue(instance, handle, newValue)
            }
            onValueChanged: {
                currentIndex = find(value)
            }
        }
    }

    Component  {
        id: renderableDropDown

        StyledComboBox {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property var values: parent.modelData.values
            property var value: parent.modelData.value
            model: values

            implicitWidth: _valueWidth
            implicitHeight: _controlBaseHeight

            Component.onCompleted: {
                currentIndex = find(value)
            }

            onCurrentIndexChanged: {
                var newValue = textAt(currentIndex)
                if (value !== newValue && currentIndex !== -1)
                    _inspectorModel.setRenderableValue(instance, handle, newValue)
            }
            onValueChanged: {
                currentIndex = find(value)
            }
        }
    }

    Component {
        id: checkBox

        Image {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property bool checked: parent.modelData.value

            source: (_resDir + (checked ? "checkbox-checked.png" : "checkbox-unchecked.png"))

            MouseArea {
                anchors.fill: parent
                onClicked: _inspectorModel.setPropertyValue(instance, handle, !checked)
            }
        }
    }

    Component {
        id: colorBox

        HandlerGenericBaseColor {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle

            color: parent.modelData.value
            onColorSelected: _inspectorModel.setPropertyValue(instance, handle, selectedColor)
        }
    }

    Component {
        id: objectReference

        HandlerGenericChooser {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant values: parent.modelData.values
            value: parent.modelData.value
            onShowBrowser: {
                activeBrowser = _inspectorView.showObjectReference(handle, instance,
                                                   mapToGlobal(width, 0))
            }
        }
    }

    Component {
        id: intSliderComponent

        HandlerPropertyBaseSlider {
            intSlider: true;
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant values: parent.modelData.values
            intValue: parent.modelData.value
            sliderMin: values[0]
            sliderMax: values[1]

            onEditingFinished: {
                _inspectorModel.setPropertyValue(instance, handle, intValue)
            }
        }
    }

    Component {
        id: fontSizeComponent

        StyledComboBox {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property real value: parent.modelData.value

            model: ["8", "9", "10", "11", "12", "14", "16", "18", "20", "22", "24", "26", "28", "36", "48", "72", "96", "120"]

            implicitWidth: _valueWidth
            implicitHeight: _controlBaseHeight

            Component.onCompleted: {
                currentIndex = find(value)
            }

            onCurrentIndexChanged: {
                var newvalue = parseInt(textAt(currentIndex))
                _inspectorModel.setPropertyValue(instance, handle, newvalue)
            }

            onValueChanged: {
                currentIndex = find(value)
            }
        }
    }

    Component {
        id: shadowResolutionComponent

        StyledComboBox {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property var value: parent.modelData.value
            property int newValue

            model: ["8", "9", "10", "11"]
            implicitWidth: _valueWidth
            implicitHeight: _controlBaseHeight

            Component.onCompleted: {
                currentIndex = find(value)
            }

            onCurrentIndexChanged: {
                newValue = parseInt(textAt(currentIndex))
                if (value !== newValue && currentIndex !== -1)
                    _inspectorModel.setPropertyValue(instance, handle, newValue)
            }

            onValueChanged: {
                currentIndex = find(value)
            }
        }
    }
}
