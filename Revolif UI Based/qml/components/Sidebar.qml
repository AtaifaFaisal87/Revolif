import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"

Rectangle {
    id: sidebar
    color: Theme.sidebarBg

    signal navigate(string page)

    // Subtle separator shadow between sidebar and main content, instead of
    // a flat hairline — reads more like a modern SaaS app.
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 10
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: Theme.shadowSoft }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 4
            spacing: 12
            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                radius: width / 2
                color: "#FFFFFF"
                border.color: Theme.isDark ? Qt.rgba(1, 1, 1, 0.14) : Theme.border
                border.width: 1
                antialiasing: true
                clip: true

                Rectangle { z: -1; anchors.fill: parent; anchors.margins: -1; radius: parent.radius + 1; color: Theme.shadowSoft; antialiasing: true }

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
                    font.pixelSize: 19
                    font.bold: true
                    color: Theme.primary
                    font.letterSpacing: 2.5
                }
                Label {
                    text: "PERSONAL GROWTH"
                    font.pixelSize: 9
                    font.letterSpacing: 1.5
                    color: Theme.textMuted
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.border
            Layout.topMargin: 6
            Layout.bottomMargin: 10
        }

        ColumnLayout {
            id: navColumn
            Layout.fillWidth: true
            spacing: 3

            Repeater {
                model: [
                    { key: "dashboard", label: "Dashboard", icon: "▤" },
                    { key: "focus", label: "Focus", icon: "◷" },
                    { key: "tasks", label: "Tasks", icon: "☑" },
                    { key: "goals", label: "Goals", icon: "◉" },
                    { key: "expenses", label: "Finance", icon: "₹" },
                    { key: "achievements", label: "Achievements", icon: "★" },
                    { key: "profile", label: "Profile", icon: "◎" },
                    { key: "settings", label: "Settings", icon: "⚙" }
                ]

                delegate: Rectangle {
                    id: navItem
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    radius: Theme.radiusBase
                    color: sidebar.currentPage === modelData.key ? Theme.selectedBg
                           : (navMouse.containsMouse ? Theme.hoverBg : "transparent")

                    Behavior on color { ColorAnimation { duration: 120 } }

                    // Active-page accent bar, like Linear/Notion-style sidebars
                    Rectangle {
                        visible: sidebar.currentPage === modelData.key
                        anchors.left: parent.left
                        anchors.leftMargin: -22
                        anchors.verticalCenter: parent.verticalCenter
                        width: 4
                        height: 22
                        radius: 2
                        color: Theme.primary
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 12
                        spacing: 12
                        Label {
                            text: modelData.key === "expenses" ? revolif.currencySymbol : modelData.icon
                            font.pixelSize: 16
                            color: sidebar.currentPage === modelData.key ? Theme.primary : Theme.textSecondary
                        }
                        Label {
                            text: modelData.label
                            font.pixelSize: Theme.fontSizeBase
                            color: sidebar.currentPage === modelData.key ? Theme.primary : Theme.textPrimary
                            font.bold: sidebar.currentPage === modelData.key
                            Layout.fillWidth: true
                        }
                    }

                    MouseArea {
                        id: navMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            sidebar.currentPage = modelData.key
                            sidebar.navigate(modelData.key)
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            radius: Theme.radiusBase
            color: Theme.cardBg
            border.color: Theme.border
            border.width: 1

            // Faint drop shadow beneath the account card for a touch of depth
            Rectangle {
                z: -1
                anchors.fill: parent
                anchors.margins: -2
                radius: parent.radius + 2
                color: Theme.shadowSoft
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12
                Rectangle {
                    Layout.preferredWidth: 38
                    Layout.preferredHeight: 38
                    radius: 19
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Theme.accentLight }
                        GradientStop { position: 1.0; color: Theme.accent }
                    }
                    Label {
                        anchors.centerIn: parent
                        text: revolif.currentUserName.charAt(0).toUpperCase()
                        color: "white"
                        font.bold: true
                        font.pixelSize: 15
                    }
                }
                ColumnLayout {
                    spacing: 2
                    Layout.fillWidth: true
                    Label {
                        text: revolif.currentUserName
                        font.pixelSize: Theme.fontSizeSm
                        font.bold: true
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        text: revolif.featuredAchievementName || revolif.currentUserTitle || "Member"
                        font.pixelSize: 11
                        font.bold: revolif.featuredAchievementName.length > 0 || revolif.currentUserTitle.length > 0
                        color: (revolif.featuredAchievementName.length > 0 || revolif.currentUserTitle.length > 0) ? Theme.accentHover : Theme.textMuted
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    radius: 16
                    color: logoutMouse.containsMouse ? Theme.hoverBg : "transparent"
                    Behavior on color { ColorAnimation { duration: 120 } }

                    Label {
                        anchors.centerIn: parent
                        text: "⏻"
                        font.pixelSize: 16
                        color: Theme.danger
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

    property string currentPage: "dashboard"
}
