import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../"

Rectangle {
    id: focusPage
    color: Theme.bg

    property var stats: ({
        todayFormatted: "0m", weekFormatted: "0m",
        totalFormatted: "0m", longestFormatted: "0m", streakDays: 0
    })
    property var history: []

    function refresh() {
        focusPage.stats = revolif.getFocusStats();
        focusPage.history = revolif.getFocusHistory();
    }

    Component.onCompleted: refresh()

    Connections {
        target: revolif
        function onFocusTick() { focusPage.refresh() }
        function onFocusStateChanged() { focusPage.refresh() }
        function onFocusHistoryChanged() { focusPage.refresh() }
    }

    // Calm, distraction-free backdrop — a soft vertical wash rather than the
    // busy wave pattern used on the dashboard.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: Theme.bg }
            GradientStop { position: 1.0; color: Qt.rgba(0.243, 0.733, 0.620, 0.06) }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        clip: true
        ScrollBar.vertical: AppScrollBar {}

        ColumnLayout {
            x: 24
            y: 24
            width: focusPage.width - 48
            spacing: 28

            // ============================================================
            //  MAIN TIMER — glassmorphism card, centered, distraction-free
            // ============================================================
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 420
                radius: Theme.radiusXl
                color: Theme.isDark ? Qt.rgba(1, 1, 1, 0.05) : Qt.rgba(1, 1, 1, 0.55)
                border.color: Theme.isDark ? Theme.border : Qt.rgba(1, 1, 1, 0.7)
                border.width: 1

                Rectangle {
                    z: -1
                    anchors.fill: parent
                    anchors.margins: -1
                    radius: parent.radius + 1
                    color: "transparent"
                    border.color: Qt.rgba(0.243, 0.733, 0.620, 0.18)
                    border.width: 1
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 22

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "FOCUS SESSION"
                        font.pixelSize: Theme.fontSizeXs
                        font.bold: true
                        font.letterSpacing: 3
                        color: Theme.primary
                    }

                    // ---- Timer with pulsing glow while running ----
                    Item {
                        Layout.alignment: Qt.AlignHCenter
                        width: 260
                        height: 260

                        Rectangle {
                            id: glow
                            anchors.centerIn: parent
                            width: parent.width
                            height: parent.height
                            radius: width / 2
                            color: Theme.accent
                            opacity: 0.12
                            visible: revolif.focusRunning

                            SequentialAnimation on opacity {
                                running: revolif.focusRunning
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.10; to: 0.32; duration: 1100; easing.type: Easing.InOutQuad }
                                NumberAnimation { from: 0.32; to: 0.10; duration: 1100; easing.type: Easing.InOutQuad }
                            }
                            SequentialAnimation on scale {
                                running: revolif.focusRunning
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.94; to: 1.0; duration: 1100; easing.type: Easing.InOutQuad }
                                NumberAnimation { from: 1.0; to: 0.94; duration: 1100; easing.type: Easing.InOutQuad }
                            }
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: 220
                            height: 220
                            radius: width / 2
                            color: Theme.isDark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(1, 1, 1, 0.6)
                            border.color: revolif.focusRunning ? Theme.accent : Theme.border
                            border.width: 3

                            Behavior on border.color { ColorAnimation { duration: 300 } }

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 4

                                Label {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: revolif.focusElapsedFormatted
                                    font.pixelSize: 40
                                    font.bold: true
                                    font.family: "Consolas, 'Courier New', monospace"
                                    color: Theme.textPrimary
                                }

                                Label {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: revolif.focusRunning ? "In focus" : (revolif.focusPaused ? "Paused" : "Ready when you are")
                                    font.pixelSize: Theme.fontSizeSm
                                    color: Theme.textSecondary
                                }
                            }
                        }
                    }

                    // ---- Controls ----
                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 14

                        AnimatedButton {
                            text: "Start"
                            primary: true
                            visible: !revolif.focusRunning && !revolif.focusPaused
                            onClicked: revolif.focusStart()
                        }
                        AnimatedButton {
                            text: "Pause"
                            visible: revolif.focusRunning
                            onClicked: revolif.focusPause()
                        }
                        AnimatedButton {
                            text: "Resume"
                            primary: true
                            visible: revolif.focusPaused
                            onClicked: revolif.focusResume()
                        }
                        AnimatedButton {
                            text: "Stop"
                            visible: revolif.focusRunning || revolif.focusPaused
                            onClicked: revolif.focusStop()
                        }
                        AnimatedButton {
                            text: "Reset"
                            visible: revolif.focusPaused
                            onClicked: revolif.focusReset()
                        }
                    }
                }
            }

            // ============================================================
            //  SESSION STATISTICS
            // ============================================================
            GridLayout {
                Layout.fillWidth: true
                columns: 5
                columnSpacing: 16
                rowSpacing: 16

                Repeater {
                    model: [
                        { label: "Today's Focus", value: focusPage.stats.todayFormatted, icon: "☀" },
                        { label: "This Week", value: focusPage.stats.weekFormatted, icon: "▤" },
                        { label: "Lifetime Total", value: focusPage.stats.totalFormatted, icon: "∞" },
                        { label: "Longest Session", value: focusPage.stats.longestFormatted, icon: "⌁" },
                        { label: "Current Streak", value: focusPage.stats.streakDays + " days", icon: "★" }
                    ]

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 110
                        radius: Theme.radiusLg
                        color: Theme.isDark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(1, 1, 1, 0.6)
                        border.color: Theme.border
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 6

                            Label {
                                text: modelData.icon
                                font.pixelSize: 18
                                color: Theme.accent
                            }
                            Item { Layout.fillHeight: true }
                            Label {
                                text: modelData.value
                                font.pixelSize: Theme.fontSizeLg
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Label {
                                text: modelData.label
                                font.pixelSize: Theme.fontSizeXs
                                color: Theme.textSecondary
                            }
                        }
                    }
                }
            }

            // ============================================================
            //  SESSION HISTORY
            // ============================================================
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(140, historyColumn.implicitHeight + 40)
                radius: Theme.radiusLg
                color: Theme.cardBg
                border.color: Theme.border
                border.width: 1
                
                Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }

                ColumnLayout {
                    id: historyColumn
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 10

                    Label {
                        text: "Recent Sessions"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Label {
                        visible: focusPage.history.length === 0
                        text: "No focus sessions recorded yet. Press Start above to begin your first one."
                        font.pixelSize: Theme.fontSizeSm
                        color: Theme.textMuted
                        Layout.topMargin: 4
                    }

                    Repeater {
                        model: focusPage.history

                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            radius: Theme.radiusSm
                            color: index % 2 === 0 ? Theme.hoverBg : "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 16

                                Label {
                                    text: modelData.date
                                    font.pixelSize: Theme.fontSizeSm
                                    color: Theme.textPrimary
                                    Layout.preferredWidth: 130
                                }
                                Label {
                                    text: modelData.startTime + " – " + modelData.endTime
                                    font.pixelSize: Theme.fontSizeSm
                                    color: Theme.textSecondary
                                    Layout.fillWidth: true
                                }
                                Label {
                                    text: modelData.duration
                                    font.pixelSize: Theme.fontSizeSm
                                    font.bold: true
                                    color: Theme.primary
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
