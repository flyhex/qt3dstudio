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
import Qt3DStudio 1.0
import "../controls"

ColumnLayout {
    id: root

    property alias propertyModel: propertyCombo.model
    property int defaultPropertyIndex: 0

    signal propertySelected(int index)

    onDefaultPropertyIndexChanged: propertyCombo.currentIndex = defaultPropertyIndex

    RowLayout {

        Layout.fillWidth: true

        StyledLabel {
            text: qsTr("Property")
        }

        StyledComboBox {
            id: propertyCombo
            textRole: "name"
            onActivated: root.propertySelected(currentIndex)
            onModelChanged: currentIndex = root.defaultPropertyIndex
        }
    }

    Component {
        id: multiLineComponent

        HandlerMultilineText {
            readonly property var actionProperty: parent ? _parentView.property : null

            label: parent ? parent.label : ""
            value: propertyModel && !_parentView.propertyValueInvalid
                   && propertyModel.value !== undefined ? propertyModel.value : ""
            onEditingFinished: _parentView.setArgumentValue(propertyModel.valueHandle, value)
        }
    }

    Component {
        id: fontSizeComponent

        HandlerPropertyCombo {
            readonly property var actionProperty: parent ? _parentView.property : null
            property var propertyValue: propertyModel && !_parentView.propertyValueInvalid
                                        && propertyModel.value !== undefined
                                        ? propertyModel.value : ""

            label: parent ? parent.label : ""
            comboModel: ["8", "9", "10", "11", "12", "14", "16", "18", "20", "22", "24", "26",
                "28", "36", "48", "72", "96", "120"];

            onValueChanged: _parentView.setArgumentValue(propertyModel.valueHandle, value)
            onPropertyValueChanged: currentIndex = find(propertyValue)
        }
    }

    Component {
        id: xyzPropertyComponent

        HandlerPropertyXYZ {
            readonly property var propValue: propertyModel && !_parentView.propertyValueInvalid
                                             && propertyModel.value !== undefined
                                             ? propertyModel.value : undefined
            label: parent ? parent.label : ""
            valueX: propValue !== undefined ? Number(propValue.x).toFixed(numberOfDecimal) : "0.000"
            valueY: propValue !== undefined ? Number(propValue.y).toFixed(numberOfDecimal) : "0.000"
            valueZ: propValue !== undefined ? Number(propValue.z).toFixed(numberOfDecimal) : "0.000"

            onPropValueChanged: {
                // FloatTextField can set its text internally, thus breaking the binding, so
                // let's set the text value explicitly each time value changes
                if (propValue !== undefined) {
                    valueX = Number(propValue.x).toFixed(numberOfDecimal);
                    valueY = Number(propValue.y).toFixed(numberOfDecimal);
                    valueZ = Number(propValue.z).toFixed(numberOfDecimal);
                }
            }

            onEditingFinished: {
                _parentView.setArgumentValue(propertyModel.valueHandle,
                                             Qt.vector3d(valueX, valueY, valueZ), true);
            }
        }
    }

    Component {
        id: xyzwPropertyComponent

        HandlerPropertyXYZW {
            readonly property var propValue: propertyModel && !_parentView.propertyValueInvalid
                                             && propertyModel.value !== undefined
                                             ? propertyModel.value : undefined
            label: parent ? parent.label : ""
            valueX: propValue !== undefined ? Number(propValue.x).toFixed(numberOfDecimal) : "0.000"
            valueY: propValue !== undefined ? Number(propValue.y).toFixed(numberOfDecimal) : "0.000"
            valueZ: propValue !== undefined ? Number(propValue.z).toFixed(numberOfDecimal) : "0.000"
            valueW: propValue !== undefined ? Number(propValue.w).toFixed(numberOfDecimal) : "0.000"

            onPropValueChanged: {
                // FloatTextField can set its text internally, thus breaking the binding, so
                // let's set the text value explicitly each time value changes
                if (propValue !== undefined) {
                    valueX = Number(propValue.x).toFixed(numberOfDecimal);
                    valueY = Number(propValue.y).toFixed(numberOfDecimal);
                    valueZ = Number(propValue.z).toFixed(numberOfDecimal);
                    valueW = Number(propValue.w).toFixed(numberOfDecimal);
                }
            }

            onEditingFinished: {
                _parentView.setArgumentValue(propertyModel.valueHandle,
                                             Qt.vector4d(valueX, valueY, valueZ, valueW), true);
            }
        }
    }


    Component {
        id: sliderPropertyComponent

        HandlerPropertySlider {
            readonly property var actionProperty: parent ? _parentView.property : null

            sliderMin: actionProperty ? actionProperty.min : 0
            sliderMax: actionProperty ? actionProperty.max : 100
            intSlider: actionProperty ? actionProperty.type === DataModelDataType.Long : false
            value: propertyModel && !_parentView.propertyValueInvalid
                   && propertyModel.value !== undefined ? propertyModel.value : sliderMin
            label: parent ? parent.label : ""

            // We don't need to care about preview for action sliders
            onCommitValue: _parentView.setArgumentValue(propertyModel.valueHandle, desiredValue)
        }
    }

    Component {
        id: comboPropertyComponent

        HandlerPropertyCombo {
            readonly property var actionProperty: parent ? _parentView.property : null
            property var propertyValue: propertyModel && !_parentView.propertyValueInvalid
                                        && propertyModel.value !== undefined
                                        ? propertyModel.value : ""

            label: parent ? parent.label : ""
            comboModel: actionProperty ? actionProperty.possibleValues : null

            onValueChanged: _parentView.setArgumentValue(propertyModel.valueHandle, value)
            onPropertyValueChanged: currentIndex = find(propertyValue)
        }
    }

    Component {
        id: booleanComponent

        HandlerGenericCheckbox {
            label: parent ? parent.label : ""
            checked: propertyModel && !_parentView.propertyValueInvalid
                     && propertyModel.value !== undefined ? propertyModel.value : false

            onClicked: {
                _parentView.setArgumentValue(propertyModel.valueHandle, !checked)
            }
        }
    }

    Component {
        id: colorBox

        HandlerGenericColor {
            readonly property var propValue: propertyModel && !_parentView.propertyValueInvalid
                                             ? propertyModel.value : undefined

            label: parent ? parent.label : ""
            color:  "black"
            onColorSelected: {
                color = selectedColor;
                _parentView.setArgumentValue(propertyModel.valueHandle, selectedColor);
            }
            onPreviewColorSelected: color = selectedColor
            onPropValueChanged: {
                color = propValue ? Qt.rgba(propValue.x, propValue.y, propValue.z, 1)
                                  : "black";
            }
        }
    }

    Component {
        id: genericTextComponent

        HandlerGenericText {
            label: parent ? parent.label : ""
            value: propertyModel && !_parentView.propertyValueInvalid
                   && propertyModel.value !== undefined ? propertyModel.value : ""
            onEditingFinished: _parentView.setArgumentValue(propertyModel.valueHandle, value)
        }
    }

    Component {
        id: floatPropertyComponent

        HandlerGenericFloat {
            label: parent ? parent.label : ""
            value: propertyModel && !_parentView.propertyValueInvalid
                   && propertyModel.value !== undefined
                   ? Number(propertyModel.value).toFixed(numberOfDecimal) : 0

            onEditingFinished: _parentView.setArgumentValue(propertyModel.valueHandle, desiredValue)
        }
    }

    Loader {
        readonly property string label: qsTr("New Value")
        readonly property var actionProperty: _parentView.property

        Layout.fillWidth: true

        onLoaded: {
            _tabOrderHandler.clear();
            if (item.tabItem1 !== undefined) {
                _tabOrderHandler.addItem(0, item.tabItem1)
                if (item.tabItem2 !== undefined) {
                    _tabOrderHandler.addItem(0, item.tabItem2)
                    if (item.tabItem3 !== undefined) {
                        _tabOrderHandler.addItem(0, item.tabItem3)
                        if (item.tabItem4 !== undefined)
                            _tabOrderHandler.addItem(0, item.tabItem4)
                    }
                }
            }
        }

        sourceComponent: {
            // KDAB_TODO Handle additionaltype
            switch (actionProperty.type) {
            case DataModelDataType.Float:
                switch (actionProperty.additionalType) {
                case AdditionalMetaDataType.FontSize:
                    return fontSizeComponent;
                case AdditionalMetaDataType.Range:
                    return sliderPropertyComponent;
                default:
                    return floatPropertyComponent;
                }
            case DataModelDataType.Long:
                return sliderPropertyComponent;
            case DataModelDataType.Float3:
                switch (actionProperty.additionalType) {
                case AdditionalMetaDataType.None:
                case AdditionalMetaDataType.Rotation:
                    return xyzPropertyComponent;
                default:
                    console.warn("KDAB_TODO implement property handler for additional " +
                                 "typeDataModelDataType.Float3: ", actionProperty.additionalType);
                    return xyzPropertyComponent;
                }
            case DataModelDataType.Float4:
                if (actionProperty.additionalType === AdditionalMetaDataType.Color)
                    return colorBox;
                if (actionProperty.additionalType === AdditionalMetaDataType.None)
                    return xyzwPropertyComponent;
                break;

            case DataModelDataType.String:
                switch (actionProperty.additionalType) {
                case AdditionalMetaDataType.StringList:
                    return comboPropertyComponent;
                case AdditionalMetaDataType.MultiLine:
                    return multiLineComponent;
                case AdditionalMetaDataType.Font:
                    return comboPropertyComponent;
                case AdditionalMetaDataType.Import:
                case AdditionalMetaDataType.Renderable:
                case AdditionalMetaDataType.String:
                    return genericTextComponent;
                default:
                    console.warn("KDAB_TODO implement property handler for additional type: ",
                                 actionProperty.additionalType)
                    return null;
                }
            case DataModelDataType.Bool:
                return booleanComponent;
            case DataModelDataType.None:
                return null;
            default: console.warn("KDAB_TODO implement property handler for type: ",
                                  actionProperty.type)

            }
            return null;
        }
    }
}
