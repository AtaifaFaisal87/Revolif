import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../"

Rectangle {
    id: adminSettingsPage
    color: "transparent"

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical: AppScrollBar {}

        ColumnLayout {
            x: 28
            y: 12
            width: adminSettingsPage.width - 56
            spacing: 24

            // ---- Security ----
            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: securityContent.implicitHeight + 48

                ColumnLayout {
                    id: securityContent
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 14

                    Label {
                        text: "Security"
                        font.bold: true
                        font.pixelSize: Theme.fontSizeLg
                        color: "white"
                    }
                    Label {
                        text: "Change the administrator password."
                        font.pixelSize: Theme.fontSizeSm
                        color: Qt.rgba(1, 1, 1, 0.55)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        spacing: 10
                        Layout.preferredWidth: 360

                        Label { text: "Current Password"; font.pixelSize: 12; color: Qt.rgba(1,1,1,0.6) }
                        TextField {
                            id: oldPassField
                            Layout.fillWidth: true
                            implicitHeight: 50
                            leftPadding: 14
                            rightPadding: 14
                            verticalAlignment: TextInput.AlignVCenter
                            font.pixelSize: 14
                            echoMode: TextInput.Password
                            color: "white"
                            selectionColor: "#10B981"
                            background: Rectangle {
                                radius: Theme.radiusSm
                                color: "#1F2937"
                                border.color: oldPassField.activeFocus ? "#10B981" : "#374151"
                                border.width: oldPassField.activeFocus ? 2 : 1
                                Behavior on border.color { ColorAnimation { duration: 120 } }
                            }
                        }

                        Label { text: "New Password"; font.pixelSize: 12; color: Qt.rgba(1,1,1,0.6) }
                        TextField {
                            id: newPassField
                            Layout.fillWidth: true
                            implicitHeight: 50
                            leftPadding: 14
                            rightPadding: 14
                            verticalAlignment: TextInput.AlignVCenter
                            font.pixelSize: 14
                            echoMode: TextInput.Password
                            color: "white"
                            selectionColor: "#10B981"
                            background: Rectangle {
                                radius: Theme.radiusSm
                                color: "#1F2937"
                                border.color: newPassField.activeFocus ? "#10B981" : "#374151"
                                border.width: newPassField.activeFocus ? 2 : 1
                                Behavior on border.color { ColorAnimation { duration: 120 } }
                            }
                        }

                        AnimatedButton {
                            Layout.topMargin: 6
                            text: "Update Password"
                            primary: true
                            dark: true
                            onClicked: {
                                if (oldPassField.text.length === 0) {
                                    revolif.errorOccurred("Current password is required.");
                                    return;
                                }
                                if (newPassField.text.length < 6) {
                                    revolif.errorOccurred("New password must be at least 6 characters.");
                                    return;
                                }
                                var ok = revolif.adminChangePassword(oldPassField.text, newPassField.text)
                                if (ok) {
                                    oldPassField.text = ""
                                    newPassField.text = ""
                                }
                            }
                        }
                    }
                }
            }

            // ---- System ----
            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: systemContent.implicitHeight + 48

                ColumnLayout {
                    id: systemContent
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 14

                    Label {
                        text: "System"
                        font.bold: true
                        font.pixelSize: Theme.fontSizeLg
                        color: "white"
                    }
                    Label {
                        text: "Export a full snapshot of current system statistics to disk."
                        font.pixelSize: Theme.fontSizeSm
                        color: Qt.rgba(1, 1, 1, 0.55)
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        spacing: 12

                        AnimatedButton {
                            text: "Generate System Report"
                            primary: false
                            dark: true
                            onClicked: revolif.generateSystemReport()
                        }

                        AnimatedButton {
                            text: "Open Reports Folder"
                            primary: false
                            dark: true
                            onClicked: revolif.openReportsFolder()
                        }
                    }

                    Label {
                        text: "Saved as a .txt file in Documents > Revolif Reports."
                        font.pixelSize: 11
                        color: Qt.rgba(1, 1, 1, 0.45)
                    }
                }
            }

            // ---- Session ----
            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: sessionContent.implicitHeight + 48

                ColumnLayout {
                    id: sessionContent
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 14

                    Label {
                        text: "Session"
                        font.bold: true
                        font.pixelSize: Theme.fontSizeLg
                        color: "white"
                    }

                    AnimatedButton {
                        text: "Logout"
                        primary: true
                        dark: true
                        onClicked: revolif.logout()
                    }
                }
            }

            Item { Layout.preferredHeight: 12 }
        }
    }
}
