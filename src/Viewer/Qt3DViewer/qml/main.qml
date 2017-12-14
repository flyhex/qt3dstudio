/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.2
import Qt3DStudioViewer 1.0
import QtStudio3D 1.0
import QtQuick.Window 2.2

ApplicationWindow {
    id: window
    width: 1280
    height: 768
    visible: true
    title: qsTr("Qt 3D Studio Viewer")

    property bool menuOpen: fileMenu.visible || viewMenu.visible
    property Item loadedContent: contentLoader ? contentLoader.item : null
    property string error
    property int previousVisibility

    property color matteColor: Qt.rgba(0, 0, 0, 1)
    property bool showRenderStats: false
    property int scaleMode: ViewerSettings.ScaleModeCenter

    function closeMenus() {
        fileMenu.close();
        scaleMenu.close();
        viewMenu.close();
    }

    Component.onCompleted: {
        _viewerHelper.restoreWindowState(window);
        previousVisibility = visibility;
    }

    onClosing: {
        _viewerHelper.storeWindowState(window);
    }

    MouseArea {
        property int swipeStart: 0

        anchors.fill: parent
        z: 10

        onPressed: {
            if (window.visibility === Window.FullScreen)
                swipeStart = mouse.y;
            _viewerHelper.handleMousePress(mouse.x, mouse.y, mouse.button, mouse.buttons,
                                           mouse.modifiers);
            mouse.accepted = true;
        }
        onReleased: {
            _viewerHelper.handleMouseRelease(mouse.x, mouse.y, mouse.button, mouse.buttons,
                                             mouse.modifiers);
            mouse.accepted = true;
        }
        onPositionChanged: {
            // Swipe down to exit fullscreen mode
            if (window.visibility === Window.FullScreen && mouse.y > swipeStart + (height / 8)) {
                window.visibility = window.previousVisibility;
            } else {
                _viewerHelper.handleMouseMove(mouse.x, mouse.y, mouse.button, mouse.buttons,
                                              mouse.modifiers);
            }
        }
    }

    DropArea {
        anchors.fill: parent
        onEntered: {
            if (drag.hasUrls) {
                var filename = _viewerHelper.convertUrlListToFilename(drag.urls);
                if (filename === "")
                    drag.accepted = false;
            }
        }
        onDropped: {
            if (drop.hasUrls) {
                var filename = _viewerHelper.convertUrlListToFilename(drop.urls);
                if (filename === "")
                    drag.accepted = false;
                else
                    _viewerHelper.loadFile(filename);
            }
        }
    }

    Loader {
        id: contentLoader
        anchors.fill: parent
        sourceComponent: {
            switch (_viewerHelper.contentView) {
            case ViewerHelper.StudioView:
                return studioContent;
            case ViewerHelper.ConnectView:
                return connectContent;
            case ViewerHelper.SequenceView:
                return sequenceContent;
            default:
                return emptyContent;
            }
        }
        Timer {
            id: asyncContentChanger
            repeat: false
            interval: 0
            property int view: ViewerHelper.DefaultView
            function changeView(newView) {
                view = newView;
                start();
            }

            onTriggered: {
                _viewerHelper.contentView = view;
            }
        }
    }

    Component {
        id: emptyContent
        Rectangle {
            color: "black"
            Label {
                anchors.fill: parent
                text: window.error
                color: _textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: width / 80
            }
        }
    }

    Component {
        id: studioContent
        Studio3D {
            id: studio3D

            property alias hiderVisible: hider.visible

            focus: true
            ViewerSettings {
                matteColor: window.matteColor
                showRenderStats: window.showRenderStats
                scaleMode: window.scaleMode
            }

            // Hider item keeps the Studio3D hidden until it starts running and we reset the
            // animation time to the start
            Rectangle {
                id: hider
                color: "black"
                anchors.fill: parent
            }

            Timer {
                id: revealTimer
                repeat: false
                interval: 0
                onTriggered: {
                    hider.visible = false;
                }
            }

            onRunningChanged: {
                if (running) {
                    // Successfully opened a presentation, update the open folder
                    _viewerHelper.openFolder = presentation.source.toString();
                    // Force the animation to start from the beginning, as the first frame render
                    // can take some time as shaders are compiled on-demand
                    // Localization note: "Scene" needs to be the same as the default "Scene"
                    // element name generated in the Studio application.
                    presentation.goToTime(qsTr("Scene"), 0);
                    revealTimer.start();
                }
            }
            onErrorChanged: {
                if (error.length > 0) {
                    window.error = error;
                    asyncContentChanger.changeView(ViewerHelper.DefaultView);
                }
            }
        }
    }

    Component {
        id: connectContent
        Rectangle {
            color: "black"
            Label {
                anchors.fill: parent
                text: _viewerHelper.connectText
                color: _textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: width / 40
            }
        }
    }

    Component {
        id: sequenceContent
        Rectangle {
            property alias mainText: mainLabel.text
            property alias detailsText: detailsLabel.text
            color: "black"
            Item {
                anchors.fill: parent
                Label {
                    id: mainLabel
                    color: _textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignBottom
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottomMargin: _controlPadding
                    height: parent.height / 2
                    font.pixelSize: width / 40
                    text: qsTr("Image sequence generation initializing...")
                }
                Label {
                    id: detailsLabel
                    color: _textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignTop
                    anchors.top: mainLabel.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.topMargin: _controlPadding
                    height: parent.height / 2
                    font.pixelSize: width / 50
                }
            }
        }
    }

    Rectangle {
        id: ipEntry
        visible: false
        color: _dialogBackgroundColor
        border.color: _dialogBorderColor
        y: (parent.height - height) / 2
        x: (parent.width - width) / 2
        width: connectionEntry.width + (2 * _controlPadding)
        height: connectionEntry.height + (2 * _controlPadding)

        onVisibleChanged: {
            if (visible) {
                connectText.forceActiveFocus();
                connectText.selectAll();
            }
        }

        Grid {
            id: connectionEntry
            spacing: _controlPadding
            columns: 2
            y: _controlPadding
            x: _controlPadding
            Label {
                id: ipEntryLabel
                width: _controlBaseWidth
                height: _controlBaseHeight
                text: qsTr("Enter IP port:")
                color: _textColor
                font.pixelSize: _fontSize
                verticalAlignment: Text.AlignVCenter
                padding: _controlPadding / 2
            }
            TextField {
                id: connectText
                width: _controlBaseWidth
                height: _controlBaseHeight
                font.pixelSize: _fontSize
                color: _textColor
                selectByMouse: true
                padding: _controlPadding / 6
                enabled: ipEntry.visible
                text: _viewerHelper.connectPort
                validator: IntValidator {
                    bottom: 1
                    top: 65535
                }

                onAccepted: {
                    if (ipEntry.visible) {
                        _viewerHelper.contentView = ViewerHelper.ConnectView;
                        _viewerHelper.connectPort = Number(text);
                        _viewerHelper.connectRemote();
                        ipEntry.visible = false;
                    }
                }

                background: Rectangle {
                    id: textBackground
                    color: _dialogFieldColor
                    border.width: 1
                    border.color: _dialogFieldBorderColor
                    radius: 2
                }
            }
            StyledButton {
                id: connectButton
                width: _controlBaseWidth
                text: qsTr("Connect")
                onClicked: {
                    _viewerHelper.contentView = ViewerHelper.ConnectView;
                    _viewerHelper.connectRemote();
                    ipEntry.visible = false;
                }
            }
            StyledButton {
                id: cancelButton
                width: _controlBaseWidth
                text: qsTr("Cancel")
                onClicked: {
                    ipEntry.visible = false;
                }
            }
        }
    }

    Component {
        id: fileDialogComponent
        FileDialog {
            id: fileDialog
            title: qsTr("Choose Presentation or Project")
            folder: _viewerHelper.openFolder
            nameFilters: [qsTr("All supported formats (*.uip *.uia *.uiab)"),
                qsTr("Studio UI Presentation (*.uip)"),
                qsTr("Application file (*.uia)"),
                qsTr("Binary Application (*.uiab)")]
            onAccepted: {
                _viewerHelper.contentView = ViewerHelper.StudioView;
                contentLoader.item.presentation.setSource(fileUrls[0]);
            }
        }
    }

    header: Rectangle {
        height: _controlBaseHeight
        color: _menuBackgroundColor
        visible: window.visibility !== Window.FullScreen

        Row {
            anchors.fill: parent
            StyledMenuButton {
                id: fileButton
                text: qsTr("File")
                menu: fileMenu
                window: window

                StyledMenu {
                    id: fileMenu
                    StyledMenuItem {
                        text: qsTr("Open...")
                        shortcut: StandardKey.Open
                        enabled: _viewerHelper.contentView !== ViewerHelper.SequenceView
                        onTriggered: {
                            if (enabled) {
                                fileDialogLoader.sourceComponent = fileDialogComponent;
                                fileDialogLoader.item.open();
                            }
                        }
                        Loader {
                            id: fileDialogLoader
                            sourceComponent: Item {}
                        }
                    }
                    StyledMenuItem {
                        text: qsTr("Connect")
                        shortcut: "F9"
                        enabled: _viewerHelper.contentView !== ViewerHelper.SequenceView
                        showCheckMark: _viewerHelper.connected
                        onTriggered: {
                            if (enabled)
                                ipEntry.visible = !ipEntry.visible;
                        }
                    }
                    StyledMenuItem {
                        text: qsTr("Reload")
                        enabled: _viewerHelper.contentView === ViewerHelper.StudioView
                        shortcut: "F5"
                        onTriggered: {
                            if (enabled) {
                                contentLoader.item.hiderVisible = true;
                                contentLoader.item.reset();
                            }
                        }
                    }
                    StyledMenuSeparator {}
                    StyledMenuItem {
                        text: qsTr("Quit")
                        shortcut: "Ctrl+Q"
                        onTriggered: {
                            window.close();
                        }
                    }
                }
            }
            StyledMenuButton {
                id: viewButton
                text: qsTr("View")
                menu: viewMenu
                window: window

                StyledMenu {
                    id: viewMenu
                    StyledMenuItem {
                        text: qsTr("Show Matte")
                        shortcut: "F6"
                        enabled: _viewerHelper.contentView === ViewerHelper.StudioView
                        onTriggered: {
                            if (enabled) {
                                if (window.matteColor === Qt.rgba(0, 0, 0, 1)) {
                                    window.matteColor = Qt.rgba(0.2, 0.2, 0.2, 1);
                                    showCheckMark = true;
                                } else {
                                    window.matteColor = Qt.rgba(0, 0, 0, 1);
                                    showCheckMark = false;
                                }
                            }
                        }
                    }
                    StyledMenuItem {
                        id: scaleMenuItem
                        text: qsTr("Scale Mode")
                        showArrow: true
                        arrowMenu: scaleMenu
                        enabled: _viewerHelper.contentView === ViewerHelper.StudioView
                        StyledMenu {
                            id: scaleMenu
                            x: parent.width
                            y: 0
                            function clearChecks() {
                                scaleCenter.showCheckMark = false;
                                scaleFit.showCheckMark = false;
                                scaleFill.showCheckMark = false;
                            }

                            StyledMenuItem {
                                id: scaleCenter
                                text: qsTr("Center")
                                shortcut: "F2"
                                showCheckMark: true
                                enabled: _viewerHelper.contentView === ViewerHelper.StudioView
                                onTriggered: {
                                    if (enabled) {
                                        scaleMenu.clearChecks();
                                        window.scaleMode = ViewerSettings.ScaleModeCenter;
                                        showCheckMark = true;
                                    }
                                }
                            }
                            StyledMenuItem {
                                id: scaleFit
                                text: qsTr("Scale to Fit")
                                shortcut: "F3"
                                enabled: _viewerHelper.contentView === ViewerHelper.StudioView
                                onTriggered: {
                                    if (enabled) {
                                        scaleMenu.clearChecks();
                                        window.scaleMode = ViewerSettings.ScaleModeFit;
                                        showCheckMark = true;
                                    }
                                }
                            }
                            StyledMenuItem {
                                id: scaleFill
                                text: qsTr("Scale to Fill")
                                shortcut: "F4"
                                enabled: _viewerHelper.contentView === ViewerHelper.StudioView
                                onTriggered: {
                                    if (enabled) {
                                        scaleMenu.clearChecks();
                                        window.scaleMode = ViewerSettings.ScaleModeFill;
                                        showCheckMark = true;
                                    }
                                }
                            }
                        }
                    }
                    StyledMenuItem {
                        text: qsTr("Show Render Statistics")
                        shortcut: "F1"
                        enabled: _viewerHelper.contentView === ViewerHelper.StudioView
                        onTriggered: {
                            if (enabled) {
                                window.showRenderStats =
                                        !contentLoader.item.viewerSettings.showRenderStats;
                                showCheckMark = window.showRenderStats;
                            }
                        }
                    }
                    StyledMenuSeparator {}
                    StyledMenuItem {
                        text: qsTr("Full Screen")
                        shortcut: "F11"
                        onTriggered: {
                            if (window.visibility !== Window.FullScreen) {
                                window.previousVisibility = window.visibility
                                window.visibility = Window.FullScreen;
                            } else {
                                window.visibility = window.previousVisibility;
                            }
                        }
                    }
                }
            }
        }
    }
}
