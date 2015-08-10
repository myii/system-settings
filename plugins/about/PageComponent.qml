/*
 * This file is part of system-settings
 *
 * Copyright (C) 2013 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
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

import QtQuick 2.4
import QtSystemInfo 5.0
import SystemSettings 1.0
import Ubuntu.Components 1.3
import Ubuntu.Components.ListItems 1.3 as ListItem
import Ubuntu.SystemSettings.StorageAbout 1.0
import Ubuntu.SystemSettings.Update 1.0
import MeeGo.QOfono 0.2

ItemPage {
    id: root
    objectName: "aboutPage"

    title: i18n.tr("About this phone")
    flickable: scrollWidget
    property var modemsSorted: []

    UbuntuStorageAboutPanel {
        id: backendInfos
    }

    DeviceInfo {
        id: deviceInfos
    }

    OfonoManager {
        id: manager
        onModemsChanged: {
            root.modemsSorted = modems.slice(0).sort();
            if (modems.length === 1) {
                phoneNumbers.setSource("PhoneNumber.qml", {
                    path: manager.modems[0]
                });
            } else if (modems.length > 1) {
                phoneNumbers.setSource("PhoneNumbers.qml", {
                    paths: root.modemsSorted
                });
            }
        }
    }

    NetworkAbout {
        id: network
    }

    NetworkInfo {
        id: wlinfo
    }

    Flickable {
        id: scrollWidget
        anchors.fill: parent
        contentHeight: contentItem.childrenRect.height
        boundsBehavior: (contentHeight > root.height) ? Flickable.DragAndOvershootBounds : Flickable.StopAtBounds
        /* Set the direction to workaround https://bugreports.qt-project.org/browse/QTBUG-31905
           otherwise the UI might end up in a situation where scrolling doesn't work */
        flickableDirection: Flickable.VerticalFlick

        Column {
            anchors.left: parent.left
            anchors.right: parent.right

            ListItem.Empty {
                height: ubuntuLabel.height + deviceLabel.height + units.gu(6)

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.centerIn: parent
                    spacing: units.gu(2)
                    Label {
                        id: ubuntuLabel
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: ""
                        fontSize: "x-large"
                    }
                    Label {
                        id: deviceLabel
                        objectName: "deviceLabel"
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: deviceInfos.manufacturer() ? deviceInfos.manufacturer() + " " + deviceInfos.model() : backendInfos.vendorString
                    }
                }
                highlightWhenPressed: false
            }

            Loader {
                id: phoneNumbers
                anchors.left: parent.left
                anchors.right: parent.right
            }

            ListItem.SingleValue {
                id: serialItem
                objectName: "serialItem"
                text: i18n.tr("Serial")
                value: backendInfos.serialNumber
                visible: backendInfos.serialNumber
            }

            ListItem.SingleValue {
                objectName: "imeiItem"
                property string imeiNumber
                imeiNumber: deviceInfos.imei(0)
                text: "IMEI"
                value: modemsSorted.length ? (imeiNumber || i18n.tr("None")) :
                    i18n.tr("None")
                visible: modemsSorted.length <= 1
            }

            ListItem.MultiValue {
                text: "IMEI"
                objectName: "imeiItems"
                values: {
                    var imeis = [];
                    modemsSorted.forEach(function (path, i) {
                        var imei = deviceInfos.imei(i);
                        imei ? imeis.push(imei) : imeis.push(i18n.tr("None"));
                    });
                    return imeis;
                }
                visible: modemsSorted.length > 1
            }

            ListItem.SingleValue {
                property string address: wlinfo.macAddress(NetworkInfo.WlanMode, 0)
                text: i18n.tr("Wi-Fi address")
                value: address ? address.toUpperCase() : ""
                visible: address
                showDivider: bthwaddr.visible
            }

            ListItem.SingleValue {
                id: bthwaddr
                text: i18n.tr("Bluetooth address")
                value: network.bluetoothMacAddress
                visible: network.bluetoothMacAddress
                showDivider: false
            }

            ListItem.Divider {}

            ListItem.SingleValue {
                id: storageItem
                objectName: "storageItem"
                text: i18n.tr("Storage")
                /* TRANSLATORS: that's the free disk space, indicated in the most appropriate storage unit */
                value: i18n.tr("%1 free").arg(Utilities.formatSize(backendInfos.getFreeSpace("/home")))
                progression: true
                onClicked: pageStack.push(Qt.resolvedUrl("Storage.qml"))
            }

            SettingsItemTitle {
                objectName: "softwareItem"
                text: i18n.tr("Software:")
            }

            ListItem.SingleValue {
                objectName: "osItem"
                text: i18n.tr("OS")
                value: "Ubuntu " + deviceInfos.version(DeviceInfo.Os) +
                       (UpdateManager.currentBuildNumber ? " (r%1)".arg(UpdateManager.currentBuildNumber) : "")
                progression: true
                onClicked: pageStack.push(Qt.resolvedUrl("Version.qml"))
            }

            ListItem.SingleValue {
                objectName: "lastUpdatedItem"
                text: i18n.tr("Last updated")
                value: UpdateManager.lastUpdateDate && !isNaN(UpdateManager.lastUpdateDate) ?
                    Qt.formatDate(UpdateManager.lastUpdateDate) : i18n.tr("Never")
            }

            ListItem.SingleControl {
                control: Button {
                    objectName: "updateButton"
                    text: i18n.tr("Check for updates")
                    width: parent.width - units.gu(4)
                    onClicked: {
                        var upPlugin = pluginManager.getByName("system-update")
                        if (upPlugin) {
                            var updatePage = upPlugin.pageComponent
                            if (updatePage)
                                pageStack.push(updatePage)
                            else
                                console.warn("Failed to get system-update pageComponent")
                        } else {
                            console.warn("Failed to get system-update plugin instance")
                        }
                    }
                }
                showDivider: false
            }

            SettingsItemTitle {
                objectName: "legalItem"
                text: i18n.tr("Legal:")
            }

            ListItem.Standard {
                objectName: "licenseItem"
                text: i18n.tr("Software licenses")
                progression: true
                onClicked: pageStack.push(Qt.resolvedUrl("Software.qml"))
            }

            ListItem.Standard {
                property var regulatoryInfo:
                    pluginManager.getByName("regulatory-information")
                text: i18n.tr("Regulatory info")
                progression: true
                visible: regulatoryInfo
                onClicked: pageStack.push(regulatoryInfo.pageComponent)
            }

            ListItem.SingleValue {
                objectName: "devmodeItem"
                text: i18n.tr("Developer mode")
                progression: true
                onClicked: pageStack.push(Qt.resolvedUrl("DevMode.qml"))
            }
        }
    }
}
