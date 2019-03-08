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
import QtQuick.Controls.Styles 1.4
import QtQuick.Extras 1.4

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
           inspectorToolbar.model = null;
           if (_inspectorModel.isDefaultMaterial())
               inspectorToolbar.model = defaultMaterialToolbarModel;
           else if (_inspectorModel.isMaterial())
               inspectorToolbar.model = materialToolbarModel;
           inspectorToolbar.visible = inspectorToolbar.model !== null;
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

        ListModel {
            id: materialToolbarModel

            ListElement {
                image: "add.png"
                name: qsTr("New")
                inUse: true
            }

            ListElement {
                image: "add.png"
                name: qsTr("Duplicate")
                inUse: true
            }

            property var actions: [
                function(){ _inspectorModel.addMaterial(); },
                function(){ _inspectorModel.duplicateMaterial(); }
            ]
        }

        ListModel {
            id: defaultMaterialToolbarModel

            ListElement {
                image: "add.png"
                name: qsTr("New")
                inUse: true
            }

            ListElement {
                image: "add-disabled.png"
                name: qsTr("Duplicate")
                inUse: false
            }

            property var actions: [
                function(){ _inspectorModel.addMaterial(); }
            ]
        }

        ListView {
            id: inspectorToolbar
            model: null
            visible: false

            Layout.fillWidth: true
            Layout.preferredHeight: 32
            orientation: ListView.Horizontal

            spacing: 4

            delegate: ToolButton {
                id: control
                enabled: inUse

                onClicked: {
                    inspectorToolbar.model.actions[index]();
                }

                background: Rectangle {
                    color: control.pressed ? _selectionColor : (hovered ? _studioColor1 : "transparent")
                    border.color: _studioColor1
                }

                contentItem: RowLayout {
                    Image {
                        source: _resDir + image
                    }
                    StyledLabel {
                        text: name
                        Layout.preferredWidth: -1
                        color: control.enabled ? _textColor : _disabledColor
                    }
                }
            }
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

                    Rectangle { // group header
                        x: -10
                        width: delegateItem.width
                        height: 25
                        color: _inspectorGroupHeaderColor

                        StyledLabel {
                            x: 30
                            text: model.title
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Image {
                            id: collapseButton
                            x: 10
                            anchors.verticalCenter: parent.verticalCenter
                            source: {
                                _resDir + (groupItems.visible ? "arrow_down.png" : "arrow.png")
                            }
                        }

                        MouseArea {
                            id: collapseButtonMouseArea
                            anchors.fill: parent
                            onClicked: {
                                if (mouse.button === Qt.LeftButton) {
                                    groupItems.visible = !groupItems.visible;
                                    _inspectorModel.updateGroupCollapseState(indexOfThisDelegate,
                                                                             !groupItems.visible)
                                }
                            }
                        }
                    }

                    Column {
                        spacing: 4
                        id: groupItems

                        visible: !_inspectorModel.isGroupCollapsed(indexOfThisDelegate)

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
                                enabled: _parentView.isEditable(modelData.handle)

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

                                ColumnLayout { // Property row and datainput control
                                    Layout.alignment: Qt.AlignTop
                                    visible: modelData.title !== "variants"
                                    spacing: 0
                                    RowLayout { // Property row
                                        Layout.alignment: Qt.AlignLeft
                                        Rectangle { // Property animation control button
                                            width: 16
                                            height: 16
                                            color: animateButtonMouseArea.containsMouse
                                                   ? _studioColor1 : _backgroundColor

                                            Image {
                                                id: animatedPropertyButton
                                                visible: model.modelData.animatable

                                                property bool animated: model.modelData.animated

                                                anchors.fill: parent
                                                fillMode: Image.Pad

                                                source: {
                                                    _resDir + (animated
                                                               ? "Inspector-AnimateToggle-Active.png"
                                                               : "Inspector-AnimateToggle-Normal.png")
                                                }

                                                MouseArea {
                                                    id: animateButtonMouseArea
                                                    anchors.fill: parent
                                                    acceptedButtons: Qt.RightButton | Qt.LeftButton
                                                    hoverEnabled: true
                                                    onClicked: {
                                                        if (mouse.button === Qt.LeftButton) {
                                                            _inspectorModel.setPropertyAnimated(
                                                                        model.modelData.instance,
                                                                        model.modelData.handle,
                                                                        !model.modelData.animated)
                                                        } else {
                                                            const coords = mapToItem(root,
                                                                                     mouse.x,
                                                                                     mouse.y);
                                                            groupDelegateItem.showContextMenu(coords);
                                                        }
                                                    }
                                                }
                                                StyledTooltip {
                                                    text: qsTr("Enable animation")
                                                    enabled: animateButtonMouseArea.containsMouse
                                                }
                                            }
                                        }

                                        Rectangle { // Datainput control button
                                            width: 16
                                            height: 16
                                            color: dataInputButtonMouseArea.containsMouse
                                                   ? _studioColor1 : _backgroundColor

                                            Image {
                                                id: ctrldPropButton

                                                property bool controlled: model.modelData.controlled
                                                visible: model.modelData.controllable
                                                anchors.fill: parent
                                                fillMode: Image.Pad

                                                source: {
                                                    _resDir + (controlled
                                                               ? "Objects-DataInput-Active.png"
                                                               : "Objects-DataInput-Inactive.png")
                                                }

                                                MouseArea {
                                                    id: dataInputButtonMouseArea
                                                    anchors.fill: parent
                                                    acceptedButtons: Qt.LeftButton
                                                    hoverEnabled: true
                                                    onClicked: {
                                                        _parentView.showDataInputChooser(
                                                                    model.modelData.handle,
                                                                    model.modelData.instance,
                                                                    mapToGlobal(
                                                                        ctrldPropButton.x
                                                                        + ctrldPropButton.width,
                                                                        ctrldPropButton.y
                                                                        + ctrldPropButton.height));

                                                    }
                                                }

                                                StyledTooltip {
                                                    text: model.modelData.controlled
                                                          ? qsTr("Data Input controller:\n")
                                                            + model.modelData.controller
                                                          : qsTr("Set Data Input controller")
                                                    enabled: dataInputButtonMouseArea.containsMouse
                                                }
                                            }
                                        }

                                        StyledLabel { // Property label
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
                                                id: propertyLabelMouseArea
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
                                                enabled: propertyLabelMouseArea.containsMouse
                                            }
                                        }
                                    }
                                }

                                Loader {
                                    id: loader
                                    readonly property var modelData: propertyRow.modelData
                                    enabled: modelData.enabled
                                    opacity: enabled ? 1 : .5
                                    Layout.alignment: Qt.AlignTop
                                    sourceComponent: {
                                        if (modelData.title === "variants")
                                            return variantTagsComponent;

                                        const dataType = modelData.dataType;
                                        switch (dataType) {
                                        case DataModelDataType.Long:
                                            if (modelData.propertyType ===
                                                    AdditionalMetaDataType.ShadowMapResolution) {
                                                return shadowResolutionComponent;
                                            }
                                            if (modelData.propertyType === AdditionalMetaDataType.Range)
                                                return intSliderComponent;
                                            console.warn("KDAB_TODO: implement handler for type \"Long\", property:",
                                                         modelData.propertyType);
                                            return null;
                                        case DataModelDataType.Long4:
                                            if (modelData.propertyType === AdditionalMetaDataType.Image)
                                                return imageChooser;
                                            console.warn("KDAB_TODO: implement handler for type \"long4\" property:",
                                                         modelData.propertyType);
                                            return null;
                                        case DataModelDataType.ObjectRef:
                                            if (modelData.propertyType === AdditionalMetaDataType.ObjectRef) {
                                                    return _parentView.isRefMaterial(modelData.instance)
                                                           ? materialReference
                                                           : objectReference;
                                            }
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
                                                return fontDropDown;
                                            if (modelData.propertyType === AdditionalMetaDataType.Texture)
                                                return textureChooser;
                                            if (modelData.propertyType === AdditionalMetaDataType.String)
                                                return editLine;
                                            if (modelData.propertyType === AdditionalMetaDataType.None)
                                                return null;
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
                                                return materialTypeDropDown;
                                            if (modelData.propertyType === AdditionalMetaDataType.Renderable)
                                                return shaderDropDown;
                                            if (modelData.propertyType === AdditionalMetaDataType.ObjectRef)
                                                return matDataDropDown;
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

                    Column {
                        visible: model.info.length > 0
                        StyledLabel {
                            width: groupElements.width
                            horizontalAlignment: Text.AlignHCenter
                            text: model.info;
                        }
                    }
                }
            }
        }
    }

    Component {
        id: editLine

        StyledTextField {
            id: textArea
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant value: parent.modelData.value
            property Item tabItem1: this
            width: _valueWidth
            height: _controlBaseHeight
            horizontalAlignment: TextInput.AlignLeft
            verticalAlignment: TextInput.AlignVCenter

            // Don't just bind text to value, since changing text explicitly in onEditingFinished
            // would break binding
            onValueChanged: text = value

            onTextChanged: _inspectorModel.setPropertyValue(instance, handle, text, false)

            onEditingFinished: {
                _inspectorModel.setPropertyValue(instance, handle, text, true);
                // Ensure we update the text to current value also in cases where making name
                // unique results in no change to model value
                text = value;
            }
        }
    }

    Component {
        id: multiLine

        HandlerBaseMultilineText {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            width: _valueWidth
            height: _controlBaseHeight * 3
            value: parent.modelData.value

            onTextChanged: _inspectorModel.setPropertyValue(instance, handle, value, false)
            onEditingFinished: _inspectorModel.setPropertyValue(instance, handle, value, true)
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
            value: {
                var renderableId = _inspectorModel.renderableId(parent.modelData.value);
                renderableId === "" ? parent.modelData.value : renderableId;
            }
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
            value: {
                parent.modelData.value === "" ? _parentView.noneString()
                                              : _parentView.convertPathToProjectRoot(
                                                    parent.modelData.value)
            }
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

            FloatTextField {
                id: floatField
                text: Number(parent.value).toFixed(decimalValue)
                implicitHeight: _controlBaseHeight
                implicitWidth: _valueWidth / 3

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
        id: fontDropDown

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
                    _inspectorModel.setPropertyValue(instance, handle, newValue)
            }
            onValueChanged: {
                var newNewIndex = find(value);
                if (!blockIndexChange || newNewIndex > 0)
                    currentIndex = newNewIndex;
                blockIndexChange = false;
            }
            onValuesChanged : {
                // Changing the values list will reset the currentIndex to zero, so block setting
                // the actual font. We'll get the proper index right after.
                if (currentIndex > 0)
                    blockIndexChange = true;
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
        id: materialTypeDropDown

        MaterialDropDown {
            callback: _inspectorModel.setMaterialTypeValue
        }
    }

    Component  {
        id: shaderDropDown

        MaterialDropDown {
            callback: _inspectorModel.setShaderValue
        }
    }

    Component  {
        id: matDataDropDown

        MaterialDropDown {
            callback: _inspectorModel.setMatDataValue
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

            showArrow: enabled

            implicitWidth: _valueWidth
            implicitHeight: _controlBaseHeight

            Component.onCompleted: {
                // Disable for non-layer
                enabled = _inspectorModel.isLayer(instance);
                currentIndex = find(value);
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

        Item {
            id: checkboxItem
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property bool checked: parent.modelData.value

            width: 16
            height: _controlBaseHeight
            Image {
                anchors.fill: parent
                fillMode: Image.Pad
                source: (_resDir + (checked ? "checkbox-checked.png" : "checkbox-unchecked.png"))

                MouseArea {
                    anchors.fill: parent
                    onClicked: _inspectorModel.setPropertyValue(checkboxItem.instance,
                                                                checkboxItem.handle,
                                                                !checkboxItem.checked)
                }
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
        id: materialReference

        HandlerGenericChooser {
            property int instance: parent.modelData.instance
            property int handle: parent.modelData.handle
            property variant values: parent.modelData.values
            value: parent.modelData.value
            onShowBrowser: {
                activeBrowser = _parentView.showMaterialReference(handle, instance,
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

    Component {
        id: variantTagsComponent

        Column {
            width: root.width - 10
            spacing: 10

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 5
                spacing: 5

                ToolButton {
                    id: importButton
                    text: qsTr("Import...")
                    width: 70
                    height: 20

                    onClicked: {
                        _variantsGroupModel.importVariants()
                    }
                }

                ToolButton {
                    id: exportButton
                    text: qsTr("Export...")
                    width: 70
                    height: 20
                    enabled: !_variantsGroupModel.variantsEmpty

                    onClicked: {
                        _variantsGroupModel.exportVariants()
                    }
                }
            }

            Text {
                text: qsTr("There are no variant tags yet. Click [+ Group] to add a new tags group and start adding tags.")
                color: _textColor
                visible: _variantsGroupModel.variantsEmpty
            }

            Repeater {
                id: tagsRepeater
                model: _variantsGroupModel
                property int maxGroupLabelWidth;

                onItemAdded: {
                    // make all group labels have equal width as the widest one
                    if (index == 0)
                        maxGroupLabelWidth = 20; // min group label width

                    if (item.groupLabelWidth > maxGroupLabelWidth) {
                        maxGroupLabelWidth = item.groupLabelWidth;

                        if (maxGroupLabelWidth > 150) // max group label width
                            maxGroupLabelWidth = 150;
                    }
                }

                Row {
                    id: variantTagsRow
                    spacing: 5

                    readonly property var tagsModel: model.tags
                    readonly property var groupModel: model
                    readonly property int groupLabelWidth: tLabel.implicitWidth

                    Text {
                        id: tLabel
                        text: model.group
                        color: model.color
                        width: tagsRepeater.maxGroupLabelWidth;
                        elide: Text.ElideRight
                        anchors.top: parent.top
                        anchors.topMargin: 5

                        MouseArea {
                            anchors.fill: parent;
                            acceptedButtons: Qt.RightButton
                            onClicked: {
                                if (mouse.button === Qt.RightButton) {
                                    const coords = mapToItem(root, mouse.x, mouse.y);
                                    _parentView.showGroupContextMenu(coords.x, coords.y, model.group);
                                }
                            }
                        }
                    }

                    Flow {
                        width: root.width - 110
                        spacing: 5

                        Repeater {
                            model: tagsModel

                            Loader {
                                readonly property var tagsModel: model
                                readonly property var grpModel: groupModel
                                sourceComponent: tagComponent
                            }
                        }

                        ToolButton {
                            id: addTagButton
                            text: qsTr("+ Tag")
                            height: 25

                            onClicked: {
                                _variantsGroupModel.addNewTag(groupModel.group)
                            }

                        }
                    }
                }
            }

            Item { width: 1; height: 5 } // vertical spacer

            ToolButton {
                id: addGroupButton
                text: qsTr("+ Group")
                width: 60
                height: 25
                onClicked: {
                    _variantsGroupModel.addNewGroup()
                }
            }

            Item { width: 1; height: 5 } // vertical spacer
        }
    }

    Component {
        id: tagComponent

        Rectangle {
            property bool toggled: tagsModel ? tagsModel.selected : false
            property string grpColor: grpModel ? grpModel.color : ""

            width: Math.max(tLabel.width + 10, 60)
            height: 25
            color: toggled ? grpColor : _backgroundColor
            border.color: _studioColor4

            Text {
                id: tLabel
                anchors.centerIn: parent
                text: tagsModel ? tagsModel.tag : ""
                color: toggled ? _textColor : _studioColor4
            }

            MouseArea {
                anchors.fill: parent;
                acceptedButtons: Qt.RightButton | Qt.LeftButton
                onClicked: {
                    if (mouse.button === Qt.LeftButton) {
                        toggled = !toggled;
                        _variantsGroupModel.setTagState(grpModel.group, tagsModel.tag, toggled);
                    } else if (mouse.button === Qt.RightButton) {
                        const coords = mapToItem(root, mouse.x, mouse.y);
                        _parentView.showTagContextMenu(coords.x, coords.y, grpModel.group,
                                                       tagsModel.tag);
                    }
                }
            }
        }
    }
}
