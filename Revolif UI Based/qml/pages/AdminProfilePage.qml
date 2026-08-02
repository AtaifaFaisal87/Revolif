import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../"

Rectangle {
    id: adminProfilePage
    color: "transparent"

    property var stats: ({})
    function refresh() { stats = revolif.getSystemStatistics() }
    Component.onCompleted: refresh()
    Connections { target: revolif; function onStatsChanged() { refresh() } }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical: AppScrollBar {}

        ColumnLayout {
            x: 28
            y: 12
            width: adminProfilePage.width - 56
            spacing: 24

            GlassPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: identityRow.implicitHeight + 48

                RowLayout {
                    id: identityRow
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 24

                    Rectangle {
                        Layout.preferredWidth: 84
                        Layout.preferredHeight: 84
                        radius: 42
                        color: AdminTheme.primary

                        Label {
                            anchors.centerIn: parent
                            text: "A"
                            font.pixelSize: 32
                            font.bold: true
                            color: "white"
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        Layout.fillWidth: true

                        Label {
                            text: "System Administrator"
                            font.pixelSize: Theme.fontSizeXl
                            font.bold: true
                            color: "white"
                        }
                        Label {
                            text: "Full access to user management, achievements, and system oversight."
                            font.pixelSize: Theme.fontSizeSm
                            color: Qt.rgba(1, 1, 1, 0.55)
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            Layout.topMargin: 8
                            Layout.preferredWidth: accessLabel.implicitWidth + 24
                            Layout.preferredHeight: 26
                            radius: Theme.radiusFull
                            color: Qt.rgba(1, 1, 1, 0.1)
                            border.color: Qt.rgba(1, 1, 1, 0.2)
                            border.width: 1

                            Label {
                                id: accessLabel
                                anchors.centerIn: parent
                                text: "⚡ Administrator Access"
                                font.pixelSize: 11
                                font.bold: true
                                color: Theme.accentLight
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 14

                Label {
                    text: "AT A GLANCE"
                    font.pixelSize: 13
                    font.bold: true
                    font.letterSpacing: 2
                    color: AdminTheme.primaryHover
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    Repeater {
                        model: [
                            { label: "Users Managed", value: adminProfilePage.stats.totalUsers || 0 },
                            { label: "Achievements", value: adminProfilePage.stats.totalAchievements || 0 },
                            { label: "Active Right Now", value: adminProfilePage.stats.activeUsers || 0 }
                        ]

                        delegate: GlassPanel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 90

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 18
                                spacing: 4

                                Label {
                                    text: modelData.value
                                    font.pixelSize: 26
                                    font.bold: true
                                    color: "white"
                                }
                                Label {
                                    text: modelData.label
                                    font.pixelSize: 12
                                    color: Qt.rgba(1, 1, 1, 0.55)
                                }
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 12 }
        }
    }
}
