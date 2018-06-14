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

    MouseArea {
        anchors.fill: controlArea
        onPressed: {
            mouse.accepted = false
            focusEater.forceActiveFocus();
        }
    }

    ColumnLayout {
        id: controlArea
        anchors.fill: parent
        anchors.topMargin: 4
        spacing: 8

        Item {
            id: focusEater
            // Used to eat keyboard focus when user clicks outside any property control
        }

        RowLayout {
            height: _controlBaseHeight + 8
            Layout.leftMargin: 4

            Image {
                id: headerImage
                source: _parentView.titleIcon !== "" ? _resDir + _parentView.titleIcon : ""
            }

            StyledLabel {
                text: _parentView.titleText
                color: _parentView.titleColor()
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

            MouseArea {
                anchors.fill: parent
                z: -10
                onPressed: {
                    mouse.accepted = false
                    focusEater.forceActiveFocus();
                }
            }

            model: _inspectorModel
            delegate: Rectangle {
                id: delegateItem

                property int indexOfThisDelegate: index

                width: parent.width
                height: items.height
                color: "transparent";
                ListView.delayRemove: true

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
                                    _parentView.showContextMenu(
                                                coords.x, coords.y,
                                                model.modelData.handle,
                                                model.modelData.instance);
                                    // Refresh text; title color is wrong after this
                                    propertyRow.color = _parentView.titleColor(
                                                modelData.instance, modelData.handle);
                                }

                                Item {
                                    Layout.alignment: Qt.AlignTop
                                    width: 16
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
                                    width: (animatedPropertyButton.visible
                                           ? 4 : animatedPropertyButton.width + 4)
                                    height: loadedItem.height + 4 // Add little space between items
                                }

                                StyledLabel {
                                    id: propertyRow

                                    readonly property var modelData: model.modelData
                                    text: model.modelData.title
                                    // Color picked from DataInput icon
                                    color: model.modelData.controlled?
                                               _dataInputColor
                                             : _parentView.titleColor(modelData.instance,
                                                                      modelData.handle)

                                    Layout.alignment: Qt.AlignTop

                                    MouseArea {
                                        id: mouse
                                        anchors.fill: parent
                                        acceptedButtons: Qt.RightButton
                                        hoverEnabled: true
                                        onClicked: {
                                            const coords = mapToItem(root, mouse.x, mouse.y);
                                            groupDelegateItem.showContextMenu(coords);
                                        }
                                    }

                                    StyledTooltip {
                                        id: valueToolTip
                                        text: modelData.toolTip
                                        enabled: mouse.containsMouse
                                    }
                                }
                                Item {
                                    Layout.alignment: Qt.AlignTop
                                    width: 16
                                    height: _controlBaseHeight
                                    visible: model.modelData.controllable

                                    MouseArea {
                                        id: mousearea
                                        anchors.fill: parent
                                        acceptedButtons: Qt.RightButton | Qt.LeftButton
                                        hoverEnabled: true
                                        onClicked: {
                                            if (mouse.button === Qt.LeftButton) {
                                                _parentView.showDataInputChooser(
                                                            model.modelData.handle,
                                                            model.modelData.instance,
                                                            mapToGlobal(mouse.x, mouse.y));
                                            } else {
                                                groupDelegateItem.showContextMenu(
                                                            mapToItem(root, mouse.x, mouse.y));
                                            }
                                        }
                                    }
                                    StyledTooltip {
                                        id: ctrlToolTip
                                        text: modelData.toolTip
                                        enabled: mousearea.containsMouse
                                    }

                                    Image {
                                        id: controlledPropertyButton

                                        property bool controlled: model.modelData.controlled

                                        anchors.fill: parent
                                        fillMode: Image.Pad

                                        source: {
                                            _resDir + (controlled
                                                       ? "Objects-DataInput-Normal.png"
                                                       : "Objects-DataInput-Disabled.png")
                                        }


                                    }
                                }

                                Item {
                                    width: (controlledPropertyButton.visible
                                            ? 4 : controlledPropertyButton.width + 4)
                                    height: loadedItem.height + 4 // Add little space between items
                                }

                                ColumnLayout {
                                    StyledLabel {
                                        id: dataInputName
                                        Layout.preferredWidth: _valueWidth
                                        // use visible: modelData.controlled instead
                                        // if label needs to be shown
                                        // only when item is actually controlled
                                        // (causes re-layouting of inspector panel)
                                        visible: modelData.controllable
                                        text: modelData.controlled ?
                                                  modelData.controller : "[No datainput control]";
                                        color: modelData.controlled ?
                                                  _dataInputColor : _disabledColor;

                                        StyledTooltip {
                                            id: dILabelToolTip
                                            text: modelData.toolTip
                                            enabled: mouseareaDILabel.containsMouse
                                        }

                                        MouseArea {
                                            id: mouseareaDILabel
                                            anchors.fill: parent
                                            acceptedButtons: Qt.RightButton | Qt.LeftButton
                                            hoverEnabled: true
                                            onClicked: {
                                                if (mouse.button === Qt.LeftButton) {
                                                    _parentView.showDataInputChooser(
                                                                model.modelData.handle,
                                                                model.modelData.instance,
                                                                mapToGlobal(mouse.x, mouse.y));
                                                } else {
                                                    groupDelegateItem.showContextMenu(
                                                                mapToItem(root, mouse.x, mouse.y));
                                                }
                                            }
                                        }
                                    }

                                    Loader {
                                        id: loader
                                        readonly property var modelData: propertyRow.modelData
                                        sourceComponent: {
                                            const dataType = modelData.dataType;
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
                                                // TODO: Add AdditionalMetaDataType for String (QT3DS-1865)
                                                // Until then, 'String' type is received as 'None'
                                                if (modelData.propertyType === AdditionalMetaDataType.None)
                                                    return editLine;
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
                                            case DataModelDataType.Float2:
                                                if (modelData.propertyType === AdditionalMetaDataType.None)
                                                    return xyPropertyComponent;
                                                console.warn("TODO: implement handler for type:\"float2\" property:",
                                                             modelData.propertyType, "text ",
                                                             model.modelData.title);
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
    }

    Component {
        id: editLine

        TextField {
            id: textArea

            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant value: parent.modelData.value
            width: _valueWidth
            horizontalAlignment: TextInput.AlignLeft
            verticalAlignment: TextInput.AlignVCenter
            font.pixelSize: _fontSize
            color: _textColor
            selectionColor: _selectionColor
            selectedTextColor: _textColor

            background: Rectangle {
                height: textArea.height
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

            text: value

            onTextChanged:
                _inspectorModel.setPropertyValue(instance, handle, text, false)

            onEditingFinished:
                _inspectorModel.setPropertyValue(instance, handle, text, true)
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

                property bool ignoreHotkeys: true

                text: scrollView.value
                horizontalAlignment: TextInput.AlignLeft
                verticalAlignment: TextInput.AlignTop
                font.pixelSize: _fontSize
                color: _textColor
                wrapMode: TextEdit.WrapAnywhere
                selectionColor: _selectionColor
                selectedTextColor: _textColor

                topPadding: 6
                bottomPadding: 6
                rightPadding: 6

                background: Rectangle {
                    height: textArea.height
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
                activeBrowser = _parentView.showMeshChooser(handle, instance,
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
                activeBrowser = _parentView.showImageChooser(handle, instance,
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
                activeBrowser = _parentView.showFilesChooser(handle, instance,
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
                activeBrowser = _parentView.showTextureChooser(handle, instance,
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

            onValuesChanged: {
                // FloatTextField can set its text internally, thus breaking the binding, so
                // let's set the text value explicitly each time value changes
                xyzHandler.valueX = Number(values[0]).toFixed(xyzHandler.numberOfDecimal);
                xyzHandler.valueY = Number(values[1]).toFixed(xyzHandler.numberOfDecimal);
                xyzHandler.valueZ = Number(values[2]).toFixed(xyzHandler.numberOfDecimal);
            }

            Item {
                width: _valueWidth - xyzHandler.width
            }
            HandlerPropertyBaseXYZ {
                id: xyzHandler
                valueX: Number(values[0]).toFixed(numberOfDecimal)
                valueY: Number(values[1]).toFixed(numberOfDecimal)
                valueZ: Number(values[2]).toFixed(numberOfDecimal)
                onEditingFinished: {
                    _inspectorModel.setPropertyValue(parent.instance, parent.handle,
                                                     Qt.vector3d(valueX, valueY, valueZ), true);
                }
                onPreviewValueChanged: {
                    _inspectorModel.setPropertyValue(parent.instance, parent.handle,
                                                     Qt.vector3d(valueX, valueY, valueZ), false);
                }
            }
        }
    }

    Component {
        id: xyPropertyComponent

        RowLayout {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant values: parent.modelData.values
            property alias tabItem1: xyHandler.tabItem1
            property alias tabItem2: xyHandler.tabItem2
            spacing: 0

            onValuesChanged: {
                // FloatTextField can set its text internally, thus breaking the binding, so
                // let's set the text value explicitly each time value changes
                xyHandler.valueX = Number(values[0]).toFixed(xyHandler.numberOfDecimal);
                xyHandler.valueY = Number(values[1]).toFixed(xyHandler.numberOfDecimal);
            }

            Item {
                width: _valueWidth - xyHandler.width
            }
            HandlerPropertyBaseXY {
                id: xyHandler
                valueX: Number(values[0]).toFixed(numberOfDecimal)
                valueY: Number(values[1]).toFixed(numberOfDecimal)
                onEditingFinished: {
                    _inspectorModel.setPropertyValue(parent.instance, parent.handle,
                                                     Qt.vector2d(valueX, valueY), true);
                }
                onPreviewValueChanged: {
                    _inspectorModel.setPropertyValue(parent.instance, parent.handle,
                                                     Qt.vector2d(valueX, valueY), false);
                }
            }
        }
    }

    Component {
        id: valueComponent

        RowLayout {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property real value: Number(parent.modelData.value).toFixed(floatField.decimalValue)
            property Item tabItem1: floatField

            onValueChanged: {
                // FloatTextField can set its text internally, thus breaking the binding, so
                // let's set the text value explicitly each time value changes
                floatField.text = Number(value).toFixed(floatField.decimalValue);
            }

            spacing: 0
            Item {
                width: _valueWidth - floatField.width
            }
            FloatTextField {
                id: floatField
                text: Number(parent.value).toFixed(decimalValue)
                implicitHeight: _controlBaseHeight
                implicitWidth: _valueWidth / 2

                onPreviewValueChanged: {
                    _inspectorModel.setPropertyValue(parent.instance, parent.handle,
                                                     Number(text), false);
                }

                onEditingFinished: {
                    _inspectorModel.setPropertyValue(parent.instance, parent.handle,
                                                     Number(text), true);
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

            value: parent.modelData.value
            sliderMin: values[0]
            sliderMax: values[1]

            onCommitValue: _inspectorModel.setPropertyValue(instance, handle, desiredValue, true)
            onPreviewValue: _inspectorModel.setPropertyValue(instance, handle, desiredValue, false)
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
            property bool blockIndexChange: false

            model: values

            implicitWidth: _valueWidth
            implicitHeight: _controlBaseHeight

            Component.onCompleted: {
                currentIndex = find(value)
            }
            onCurrentIndexChanged: {
                var newValue = textAt(currentIndex)
                if (!blockIndexChange && value !== newValue && currentIndex !== -1)
                    _inspectorModel.setMaterialTypeValue(instance, handle, newValue)
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
            onColorSelected: _inspectorModel.setPropertyValue(instance, handle, selectedColor);
            onPreviewColorSelected: _inspectorModel.setPropertyValue(instance, handle,
                                                                     selectedColor, false);
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
                activeBrowser = _parentView.showObjectReference(handle, instance,
                                                                mapToGlobal(width, 0))
            }
        }
    }

    Component {
        id: intSliderComponent

        HandlerPropertyBaseSlider {
            intSlider: true;
            property int intValue: Math.round(desiredValue)
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant values: parent.modelData.values
            value: parent.modelData.value
            sliderMin: values[0]
            sliderMax: values[1]

            onCommitValue: _inspectorModel.setPropertyValue(instance, handle, intValue, true)
            onPreviewValue: _inspectorModel.setPropertyValue(instance, handle, intValue, false)
        }
    }

    Component {
        id: fontSizeComponent

        StyledComboBox {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property real value: parent.modelData.value

            editable: true
            property bool ready: false

            validator: IntValidator {
                bottom: 1
            }

            model: ["8", "9", "10", "11", "12", "14", "16", "18", "20", "22", "24", "26", "28",
                "36", "48", "72", "96", "120", "160", "200"]

            implicitWidth: _valueWidth
            implicitHeight: _controlBaseHeight

            Component.onCompleted: {
                editText = value;
                ready = true;
            }

            onValueChanged: {
                if (ready && !isNaN(value))
                    editText = value;
            }

            onEditTextChanged: {
                if (ready) {
                    var newvalue = parseInt(editText);
                    _inspectorModel.setPropertyValue(instance, handle, newvalue, false);
                }
            }

            onActiveFocusChanged: {
                if (!activeFocus) {
                    var newvalue = parseInt(editText);
                    _inspectorModel.setPropertyValue(instance, handle, newvalue);
                }
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
