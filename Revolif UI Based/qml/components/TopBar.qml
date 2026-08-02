import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"

Rectangle {
    id: topBar
    color: Theme.bg

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        ColumnLayout {
            spacing: 4
            Label {
                text: revolif.pageTitle
                font.pixelSize: Theme.fontSizeXl
                font.bold: true
                color: Theme.textPrimary
            }
            Rectangle {
                width: 30
                height: 4
                radius: 2
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Theme.primary }
                    GradientStop { position: 1.0; color: Theme.accent }
                }
            }
        }

        Item { Layout.fillWidth: true }

        Rectangle {
            Layout.preferredWidth: 190
            Layout.preferredHeight: 46
            radius: Theme.radiusFull
            color: Theme.cardBg
            border.color: Theme.border
            border.width: 1

            Rectangle {
                z: -1
                anchors.fill: parent
                anchors.margins: -2
                radius: parent.radius + 2
                color: Theme.shadowSoft
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 16
                spacing: 10

                Rectangle {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    radius: 17
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Theme.accentLight }
                        GradientStop { position: 1.0; color: Theme.accent }
                    }

                    Label {
                        anchors.centerIn: parent
                        text: revolif.currentStreak
                        font.bold: true
                        font.pixelSize: 14
                        color: Theme.primaryDark
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Label {
                        text: revolif.currentStreak + " Day Streak"
                        font.pixelSize: Theme.fontSizeSm
                        font.bold: true
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                    }

                    Label {
                        text: "Best: " + revolif.bestStreak + " days"
                        font.pixelSize: Theme.fontSizeXs
                        color: Theme.textMuted
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 46
            Layout.preferredHeight: 46
            radius: 23
            gradient: Gradient {
                GradientStop { position: 0.0; color: Theme.primary }
                GradientStop { position: 1.0; color: Theme.primaryDark }
            }
            scale: avatarMouse.containsMouse ? 1.05 : 1.0
            Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

            Label {
                anchors.centerIn: parent
                text: revolif.currentUserName.charAt(0).toUpperCase()
                color: "white"
                font.bold: true
                font.pixelSize: 17
            }
            MouseArea {
                id: avatarMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    sidebar.currentPage = "profile"
                    sidebar.navigate("profile")
                }

                ToolTip.visible: containsMouse
                ToolTip.text: "View profile"
            }
        }
    }
}
