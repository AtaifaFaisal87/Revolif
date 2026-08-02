import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../dialogs"
import "../"

Rectangle {
    id: goalsPage
    color: Theme.bg

    property string filter: "all"
    property int totalGoals: 0
    property int completedGoals: 0
    property int activeGoals: 0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 96
                radius: Theme.radiusLg
                color: Theme.cardBg
                border.color: Theme.border
                border.width: 1
                
                Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: 14
                        color: Qt.rgba(0.243, 0.733, 0.620, 0.15)
                        Label { anchors.centerIn: parent; text: "◉"; color: Theme.accent; font.pixelSize: 18 }
                    }

                    ColumnLayout {
                        spacing: 2
                        Label {
                            text: goalsPage.totalGoals
                            font.pixelSize: Theme.fontSizeXl
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Label {
                            text: "Total Goals"
                            font.pixelSize: Theme.fontSizeSm
                            color: Theme.textSecondary
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 96
                radius: Theme.radiusLg
                color: Theme.cardBg
                border.color: Theme.border
                border.width: 1
                
                Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: 14
                        color: Qt.rgba(0.243, 0.733, 0.620, 0.15)
                        Label { anchors.centerIn: parent; text: "✓"; color: Theme.success; font.bold: true; font.pixelSize: 18 }
                    }

                    ColumnLayout {
                        spacing: 2
                        Label {
                            text: goalsPage.completedGoals
                            font.pixelSize: Theme.fontSizeXl
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Label {
                            text: "Completed Goals"
                            font.pixelSize: Theme.fontSizeSm
                            color: Theme.textSecondary
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 96
                radius: Theme.radiusLg
                color: Theme.cardBg
                border.color: Theme.border
                border.width: 1
                
                Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: 14
                        color: Qt.rgba(0.878, 0.627, 0.188, 0.15)
                        Label { anchors.centerIn: parent; text: "○"; color: Theme.warning; font.bold: true; font.pixelSize: 18 }
                    }

                    ColumnLayout {
                        spacing: 2
                        Label {
                            text: goalsPage.activeGoals
                            font.pixelSize: Theme.fontSizeXl
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Label {
                            text: "Active Goals"
                            font.pixelSize: Theme.fontSizeSm
                            color: Theme.textSecondary
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Item { Layout.fillWidth: true }

            Row {
                spacing: 8
                Repeater {
                    model: ["All", "Pending", "Completed"]
                    delegate: Rectangle {
                        width: label.implicitWidth + 24
                        height: 36
                        radius: Theme.radiusFull
                        color: goalsPage.filter.toLowerCase() === modelData.toLowerCase() ? Theme.primary : Theme.cardBg
                        border.color: Theme.border
                        border.width: 1

                        Label {
                            id: label
                            anchors.centerIn: parent
                            text: modelData
                            color: goalsPage.filter.toLowerCase() === modelData.toLowerCase() ? "white" : Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSm
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                goalsPage.filter = modelData.toLowerCase();
                                goalListView.refresh();
                            }
                        }
                    }
                }
            }

            AnimatedButton {
                text: "+ Add Goal"
                primary: true
                onClicked: addGoalDialog.open()
            }
        }

        ListView {
            id: goalListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12
            clip: true
            model: []

            function refresh() {
                var all = revolif.getGoals();
                goalsPage.totalGoals = all.length;
                goalsPage.completedGoals = all.filter(function(g) { return g.status === "Completed"; }).length;
                goalsPage.activeGoals = all.filter(function(g) { return g.status !== "Completed"; }).length;
                if (goalsPage.filter === "all") {
                    goalListView.model = all;
                } else if (goalsPage.filter === "completed") {
                    goalListView.model = all.filter(function(g) { return g.displayStatus === "Completed"; });
                } else {
                    // "Pending" covers both Pending and Overdue goals, i.e.
                    // anything not yet completed — same grouping as the
                    // Active Goals stat above.
                    goalListView.model = all.filter(function(g) { return g.displayStatus !== "Completed"; });
                }
            }

            Component.onCompleted: refresh()

            Connections {
                target: revolif
                function onStatsChanged() { goalListView.refresh() }
            }

            delegate: Rectangle {
                width: ListView.view.width
                height: 90
                radius: Theme.radiusLg
                color: Theme.cardBg
                border.color: Theme.border
                border.width: 1
                
                Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 16

                    Rectangle {
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        radius: 6
                        border.color: modelData.status === "Completed" ? Theme.success : Theme.border
                        border.width: 2
                        color: modelData.status === "Completed" ? Theme.success : "transparent"

                        Label {
                            anchors.centerIn: parent
                            text: "✓"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 12
                            visible: modelData.status === "Completed"
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (modelData.status !== "Completed") revolif.completeGoal(modelData.id);
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        Layout.fillWidth: true
                        // Prevents a long "category · Due: date" string from
                        // forcing this column wider than its fair share of
                        // the row, which would shove the status pill and
                        // action icons sideways and break alignment between
                        // rows of different text lengths.
                        Layout.minimumWidth: 0

                        Label {
                            text: modelData.title
                            font.bold: true
                            font.pixelSize: Theme.fontSizeBase
                            color: Theme.textPrimary
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            elide: Text.ElideRight
                        }

                        Label {
                            text: modelData.category + " · Due: " + modelData.deadline
                            font.pixelSize: Theme.fontSizeSm
                            color: Theme.textSecondary
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            elide: Text.ElideRight
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 28
                        Layout.alignment: Qt.AlignVCenter
                        radius: Theme.radiusFull
                        color: modelData.displayStatus === "Overdue" ? "#FDE8E8" : modelData.displayStatus === "Completed" ? "#D1FAE5" : "#E0F2FE"

                        Label {
                            anchors.centerIn: parent
                            text: modelData.displayStatus
                            font.pixelSize: 11
                            font.bold: true
                            color: modelData.displayStatus === "Overdue" ? Theme.danger : modelData.displayStatus === "Completed" ? Theme.success : Theme.primary
                        }
                    }

                    // Fixed-width action column so the view/edit/delete
                    // icons land in the same spot on every row.
                    Row {
                        Layout.preferredWidth: 76
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 14

                        ActionIcon {
                            kind: "eye"
                            size: 18
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: goalDetailsDialog.openFor(modelData)
                            }
                        }

                        ActionIcon {
                            kind: "edit"
                            size: 18
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    goalDetailsDialog.openFor(modelData);
                                    goalDetailsDialog.mode = "edit";
                                }
                            }
                        }

                        ActionIcon {
                            kind: "trash"
                            size: 18
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    deleteConfirm.goalId = modelData.id
                                    deleteConfirm.message = "Delete goal \"" + modelData.title + "\"? This cannot be undone."
                                    deleteConfirm.open()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    AddGoalDialog { id: addGoalDialog }

    GoalDetailsDialog { id: goalDetailsDialog }

    ConfirmDialog {
        id: deleteConfirm
        property int goalId: -1
        confirmText: "Delete"
        onConfirmed: revolif.deleteGoal(goalId)
    }
}
