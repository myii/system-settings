/*
 * This file is part of ubuntu-system-settings
 *
 * Copyright (C) 2013-2015 Canonical Ltd.
 *
 * Contact: Charles Kerr <charles.kerr@canonical.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QMenuModel 0.1
import QtQuick 2.4
import SystemSettings 1.0
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3
import Ubuntu.Components.ListItems 1.3 as ListItem
import Ubuntu.SystemSettings.Bluetooth 1.0
import Ubuntu.Settings.Components 0.1 as USC


ItemPage {
    id: root
    title: i18n.tr("Bluetooth")
    objectName: "bluetoothPage"

    property var dialogPopupId

    UbuntuBluetoothPanel { id: backend }

    Timer {
        id: discoverableTimer
        onTriggered: backend.trySetDiscoverable(true)
    }

    /* Disable bt visiblity when switching out */
    Connections {
        target: Qt.application
        onActiveChanged: {
            if (Qt.application.state !== Qt.ApplicationActive) {
                backend.trySetDiscoverable(false)
            }
            else {
                discoverableTimer.start()
            }
        }
    }

    Component {
        id: confirmPasskeyDialog
        ConfirmPasskeyDialog { }
    }

    Component {
        id: providePasskeyDialog
        ProvidePasskeyDialog { }
    }

    Component {
        id: providePinCodeDialog
        ProvidePinCodeDialog { }
    }

    Component {
        id: displayPasskeyDialog
        DisplayPasskeyDialog { }
    }

    Connections {
        target: backend.agent
        onPasskeyConfirmationNeeded: {
            var request_tag = tag
            var popup = PopupUtils.open(confirmPasskeyDialog, root, {passkey: passkey, name: device.name})
            popup.canceled.connect(function() {target.confirmPasskey(request_tag, false)})
            popup.confirmed.connect(function() {target.confirmPasskey(request_tag, true)})
        }
        onPasskeyNeeded: {
            var request_tag = tag
            var popup = PopupUtils.open(providePasskeyDialog, root, {name: device.name})
            popup.canceled.connect(function() {target.providePasskey(request_tag, false, 0)})
            popup.provided.connect(function(passkey) {target.providePasskey(request_tag, true, passkey)})
        }
        onPinCodeNeeded: {
            var request_tag = tag
            var popup = PopupUtils.open(providePinCodeDialog, root, {name: device.name})
            popup.canceled.connect(function() {target.providePinCode(request_tag, false, "")})
            popup.provided.connect(function(pinCode) {target.providePinCode(request_tag, true, pinCode)})
        }
        onDisplayPasskeyNeeded: {
            if (!root.dialogPopupId)
            {
                var request_tag = tag
                root.dialogPopupId  = PopupUtils.open(displayPasskeyDialog, root, {passkey: passkey, name: device.name,
                                                 entered: entered})
                root.dialogPopupId.canceled.connect(function() {root.dialogPopupId = null;
                    target.displayPasskeyCallback(request_tag)})
            }
            else
            {
                root.dialogPopupId.entered = entered
            }
        }
        onPairingDone: {
            if (root.dialogPopupId)
                PopupUtils.close(root.dialogPopupId)
        }
    }

    function getDisplayName(type, displayName) {
        /* TODO: If the device requires a PIN, show ellipsis.
         * Right now we don't have a way to check this, just return the name
         */
        return displayName;
    }

    Flickable {
        anchors.fill: parent
        contentHeight: contentItem.childrenRect.height
        boundsBehavior: (contentHeight > root.height) ?
                            Flickable.DragAndOvershootBounds :
                            Flickable.StopAtBounds
        /* Set the direction to workaround https://bugreports.qt-project.org/browse/QTBUG-31905
           otherwise the UI might end up in a situation where scrolling doesn't work */
        flickableDirection: Flickable.VerticalFlick

        Column {
            anchors.left: parent.left
            anchors.right: parent.right

            QDBusActionGroup {
                id: bluetoothActionGroup
                busType: DBus.SessionBus
                busName: "com.canonical.indicator.bluetooth"
                objectPath: "/com/canonical/indicator/bluetooth"

                property variant enabled: action("bluetooth-enabled")

                Component.onCompleted: start()
            }

            ListItem.Standard {
                text: i18n.tr("Bluetooth")
                control: Switch {
                    id: btSwitch
                    property bool serverChecked: bluetoothActionGroup.enabled.state
                    USC.ServerPropertySynchroniser {
                        userTarget: btSwitch
                        userProperty: "checked"
                        serverTarget: btSwitch
                        serverProperty: "serverChecked"

                        onSyncTriggered: bluetoothActionGroup.enabled.activate()
                    }
                }
            }

            // Discoverability
            ListItem.Standard {
                enabled: bluetoothActionGroup.enabled
                showDivider: false

                Rectangle {
                    color: "transparent"
                    anchors.fill: parent
                    anchors.topMargin: units.gu(1)
                    anchors.leftMargin: units.gu(2)
                    anchors.rightMargin: units.gu(2)

                    Label {
                        anchors {
                            top: parent.top
                            left: parent.left
                            topMargin: units.gu(1)
                        }
                        height: units.gu(3)
                        text: backend.discoverable ? i18n.tr("Discoverable") : i18n.tr("Not discoverable")
                    }

                    Label {
                        anchors {
                            top: parent.top
                            right: parent.right
                            topMargin: units.gu(1)
                        }
                        height: units.gu(3)
                        text: backend.discoverable ? backend.adapterName() : ""
                        color: "darkgrey"
                        visible: backend.discoverable
                        enabled: false
                    }

                    ActivityIndicator {
                        anchors {
                            top: parent.top
                            right: parent.right
                            topMargin: units.gu(1)
                        }
                        visible: backend.powered && !backend.discoverable
                        running: true
                    }
                }
            }

            //  Connnected Headset(s)
            ListItem.Standard {
                id: connectedHeader
                text: i18n.tr("Connected devices:")

                enabled: bluetoothActionGroup.enabled
                visible: connectedList.visible
            }

            Column {
                id: connectedList
                anchors {
                    left: parent.left
                    right: parent.right
                }
                visible: bluetoothActionGroup.enabled && (connectedRepeater.count > 0)
                objectName: "connectedList"

                Repeater {
                    id: connectedRepeater
                    model: backend.connectedDevices
                    delegate: ListItem.Standard {
                        iconSource: iconPath
                        iconFrame: false
                        text: getDisplayName(type, displayName)
                        control: ActivityIndicator {
                            visible: connection == Device.Connecting
                            running: true
                        }
                        onClicked: {
                            backend.setSelectedDevice(addressName);
                            pageStack.push(Qt.resolvedUrl("DevicePage.qml"), {backend: backend, root: root});
                        }
                        progression: true
                    }
                }
            }

            //  Disconnnected Headset(s)

            SettingsItemTitle {
                id: disconnectedHeader
                text: connectedList.visible ? i18n.tr("Connect another device:") : i18n.tr("Connect a device:")
                enabled: bluetoothActionGroup.enabled
                control: ActivityIndicator {
                    visible: backend.powered && backend.discovering
                    running: true
                }
            }

            Column {
                id: disconnectedList
                anchors {
                    left: parent.left
                    right: parent.right
                }
                visible: bluetoothActionGroup.enabled && (disconnectedRepeater.count > 0)
                objectName: "disconnectedList"

                Repeater {
                    id: disconnectedRepeater
                    model: backend.disconnectedDevices
                    delegate: ListItem.Standard {
                        iconSource: iconPath
                        iconFrame: false
                        text: getDisplayName(type, displayName)
                        onClicked: {
                            backend.setSelectedDevice(addressName);
                            pageStack.push(Qt.resolvedUrl("DevicePage.qml"), {backend: backend, root: root});
                        }
                        progression: true
                    }
                }
            }
            ListItem.Standard {
                id: disconnectedNone
                text: i18n.tr("None detected")
                visible: !disconnectedList.visible
                enabled: false
            }

            //  Devices that connect automatically
            SettingsItemTitle {
                id: autoconnectHeader
                text: i18n.tr("Connect automatically when detected:")
                visible: autoconnectList.visible
                enabled: bluetoothActionGroup.enabled
            }
            Column {
                id: autoconnectList
                anchors {
                    left: parent.left
                    right: parent.right
                }

                visible: bluetoothActionGroup.enabled && (autoconnectRepeater.count > 0)

                Repeater {
                    id: autoconnectRepeater
                    model: backend.autoconnectDevices
                    delegate: ListItem.Standard {
                        iconSource: iconPath
                        iconFrame: false
                        text: getDisplayName(type, displayName)
                        onClicked: {
                            backend.setSelectedDevice(addressName);
                            pageStack.push(Qt.resolvedUrl("DevicePage.qml"), {backend: backend, root: root});
                        }
                        progression: true
                    }
                }
            }
        }
    }
}
