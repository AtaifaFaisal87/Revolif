import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../"

Rectangle {
    id: settingsPage
    color: Theme.bg

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            radius: Theme.radiusLg
            color: Theme.cardBg
            border.color: Theme.border
            border.width: 1
            
            Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 16

                Label {
                    text: "Account Settings"
                    font.bold: true
                    font.pixelSize: Theme.fontSizeLg
                    color: Theme.textPrimary
                }

                Label {
                    text: "Manage your account preferences and data."
                    font.pixelSize: Theme.fontSizeBase
                    color: Theme.textSecondary
                }

                AnimatedButton {
                    text: "Logout"
                    primary: true
                    onClicked: revolif.logout()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: currencyContent.implicitHeight + 48
            radius: Theme.radiusLg
            color: Theme.cardBg
            border.color: Theme.border
            border.width: 1
            
            Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }

            ColumnLayout {
                id: currencyContent
                anchors.fill: parent
                anchors.margins: 24
                spacing: 12

                Label {
                    text: "Currency"
                    font.bold: true
                    font.pixelSize: Theme.fontSizeLg
                    color: Theme.textPrimary
                }

                Label {
                    text: "Choose the currency symbol used across Finance, Dashboard, and reports."
                    font.pixelSize: Theme.fontSizeBase
                    color: Theme.textSecondary
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                AppComboBox {
                    id: currencyCombo
                    Layout.preferredWidth: 260
                    textRole: "label"
                    model: revolif.getCurrencyOptions()

                    Component.onCompleted: {
                        for (var i = 0; i < model.length; i++) {
                            if (model[i].code === revolif.currencyCode) {
                                currentIndex = i;
                                break;
                            }
                        }
                    }

                    onActivated: revolif.setCurrency(model[currentIndex].code)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: appearanceContent.implicitHeight + 48
            radius: Theme.radiusLg
            color: Theme.cardBg
            border.color: Theme.border
            border.width: 1

            Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }

            ColumnLayout {
                id: appearanceContent
                anchors.fill: parent
                anchors.margins: 24
                spacing: 12

                Label {
                    text: "Appearance"
                    font.bold: true
                    font.pixelSize: Theme.fontSizeLg
                    color: Theme.textPrimary
                }

                Label {
                    text: "Switch between a light and dark color theme across the whole app."
                    font.pixelSize: Theme.fontSizeBase
                    color: Theme.textSecondary
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.topMargin: 4
                    spacing: 14

                    Rectangle {
                        id: themeSwitch
                        Layout.preferredWidth: 52
                        Layout.preferredHeight: 30
                        radius: 15
                        color: revolif.darkMode ? Theme.primary : Theme.border

                        Behavior on color { ColorAnimation { duration: 150 } }

                        Rectangle {
                            width: 24
                            height: 24
                            radius: 12
                            color: "#FFFFFF"
                            y: 3
                            x: revolif.darkMode ? parent.width - width - 3 : 3
                            Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: revolif.setDarkMode(!revolif.darkMode)
                        }
                    }

                    Label {
                        text: revolif.darkMode ? "Dark theme" : "Light theme"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }
                }
            }
        }
    }
}
