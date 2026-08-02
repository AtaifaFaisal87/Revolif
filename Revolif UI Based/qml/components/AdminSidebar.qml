import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"

// Deliberately minimal control-center sidebar for the Admin experience.
// Unlike the productivity Sidebar (Tasks/Goals/Focus/etc.), the admin only
// ever needs three destinations -- everything else happens inline on the
// Dashboard itself.
Rectangle {
    id: sidebar
    property string currentPage: "dashboard"
    signal navigate(string page)

    color: AdminTheme.surface

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Layout.bottomMargin: 4

            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                radius: width / 2
                color: "#FFFFFF"
                border.color: Qt.rgba(1, 1, 1, 0.18)
                border.width: 1
                antialiasing: true
                clip: true

                Image {
                    anchors.fill: parent
                    source: "qrc:/assets/logo_circle_full.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }
            }

            ColumnLayout {
                spacing: 0
                Label {
                    text: "REVOLIF"
                    font.pixelSize: 16
                    font.bold: true
                    color: "white"
                    font.letterSpacing: 2
                }
                Label {
                    text: "CONTROL CENTER"
                    font.pixelSize: 10
                    font.letterSpacing: 1.5
                    color: Theme.accentLight
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: AdminTheme.border
            Layout.topMargin: 12
            Layout.bottomMargin: 12
        }

        Repeater {
            model: [
                { key: "dashboard", label: "Dashboard", icon: "▤" },
                { key: "profile", label: "Profile", icon: "◎" },
                { key: "settings", label: "Settings", icon: "⚙" }
            ]

            delegate: Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                radius: Theme.radiusBase
                color: sidebar.currentPage === modelData.key ? Qt.rgba(0.063, 0.725, 0.506, 0.16) : "transparent"
                border.color: sidebar.currentPage === modelData.key ? AdminTheme.primary : "transparent"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 12

                    Label {
                        text: modelData.icon
                        font.pixelSize: 16
                        color: sidebar.currentPage === modelData.key ? AdminTheme.primary : Qt.rgba(1, 1, 1, 0.6)
                    }
                    Label {
                        text: modelData.label
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: sidebar.currentPage === modelData.key
                        color: sidebar.currentPage === modelData.key ? "white" : Qt.rgba(1, 1, 1, 0.75)
                        Layout.fillWidth: true
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: if (sidebar.currentPage !== modelData.key) parent.color = Qt.rgba(1, 1, 1, 0.06)
                    onExited: if (sidebar.currentPage !== modelData.key) parent.color = "transparent"
                    onClicked: {
                        sidebar.currentPage = modelData.key
                        sidebar.navigate(modelData.key)
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            radius: Theme.radiusBase
            color: Qt.rgba(1, 1, 1, 0.08)
            border.color: Qt.rgba(1, 1, 1, 0.15)
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                Rectangle {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    radius: 18
                    color: AdminTheme.primary
                    Label {
                        anchors.centerIn: parent
                        text: "A"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 14
                    }
                }
                ColumnLayout {
                    spacing: 2
                    Layout.fillWidth: true
                    Label {
                        text: "Administrator"
                        font.pixelSize: Theme.fontSizeSm
                        font.bold: true
                        color: "white"
                    }
                    Label {
                        text: "System Access"
                        font.pixelSize: 11
                        color: Theme.accentLight
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    radius: 16
                    color: logoutMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.12) : "transparent"

                    Label {
                        anchors.centerIn: parent
                        text: "⏻"
                        font.pixelSize: 16
                        color: "#EF4444"
                    }

                    MouseArea {
                        id: logoutMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: revolif.logout()

                        ToolTip.visible: containsMouse
                        ToolTip.text: "Logout"
                    }
                }
            }
        }
    }
}
