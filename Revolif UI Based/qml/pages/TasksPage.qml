import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../dialogs"
import "../"

Rectangle {
    id: tasksPage
    color: Theme.bg

    property string filter: "all"
    property int totalTasks: 0
    property int completedTasks: 0
    property int pendingTasks: 0

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
                        Label { anchors.centerIn: parent; text: "☰"; color: Theme.accent; font.pixelSize: 18 }
                    }

                    ColumnLayout {
                        spacing: 2
                        Label {
                            text: tasksPage.totalTasks
                            font.pixelSize: Theme.fontSizeXl
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Label {
                            text: "Total Tasks"
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
                            text: tasksPage.completedTasks
                            font.pixelSize: Theme.fontSizeXl
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Label {
                            text: "Completed Tasks"
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
                            text: tasksPage.pendingTasks
                            font.pixelSize: Theme.fontSizeXl
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Label {
                            text: "Pending Tasks"
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
                        color: tasksPage.filter.toLowerCase() === modelData.toLowerCase() ? Theme.primary : Theme.cardBg
                        border.color: Theme.border
                        border.width: 1

                        Label {
                            id: label
                            anchors.centerIn: parent
                            text: modelData
                            color: tasksPage.filter.toLowerCase() === modelData.toLowerCase() ? "white" : Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSm
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                tasksPage.filter = modelData.toLowerCase();
                                taskListView.refresh();
                            }
                        }
                    }
                }
            }

            AnimatedButton {
                text: "+ Add Task"
                primary: true
                onClicked: addTaskDialog.open()
            }
        }

        ListView {
            id: taskListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12
            clip: true
            model: []

            function refresh() {
                var all = revolif.getTasks();
                tasksPage.totalTasks = all.length;
                tasksPage.completedTasks = all.filter(function(t) { return t.status === "Completed"; }).length;
                tasksPage.pendingTasks = all.filter(function(t) { return t.status === "Pending"; }).length;
                if (tasksPage.filter === "all") {
                    taskListView.model = all;
                } else {
                    taskListView.model = all.filter(function(t) {
                        return tasksPage.filter === "pending" ? t.status === "Pending" : t.status === "Completed";
                    });
                }
            }

            Component.onCompleted: refresh()

            Connections {
                target: revolif
                function onStatsChanged() { taskListView.refresh() }
            }

            delegate: Rectangle {
                width: ListView.view.width
                height: 80
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
                                if (modelData.status !== "Completed") revolif.completeTask(modelData.id);
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        Layout.fillWidth: true
                        // Without this, a long "type · category · deadline ·
                        // priority" string forces this column wider than its
                        // fair share of the row, which shoves the priority
                        // pill and action icons sideways and breaks
                        // alignment between rows of different text lengths.
                        Layout.minimumWidth: 0

                        Label {
                            text: modelData.title
                            font.bold: true
                            font.pixelSize: Theme.fontSizeBase
                            color: modelData.status === "Completed" ? Theme.textMuted : Theme.textPrimary
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            elide: Text.ElideRight
                        }

                        Label {
                            text: modelData.type + " · " + modelData.category + " · " + modelData.deadline + " · " + modelData.priority
                            font.pixelSize: Theme.fontSizeSm
                            color: Theme.textSecondary
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            elide: Text.ElideRight
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 28
                        Layout.alignment: Qt.AlignVCenter
                        radius: Theme.radiusFull
                        color: modelData.priority === "High" ? "#FDE8E8" : modelData.priority === "Medium" ? "#FEF3C7" : "#D1FAE5"

                        Label {
                            anchors.centerIn: parent
                            text: modelData.priority
                            font.pixelSize: 11
                            font.bold: true
                            color: modelData.priority === "High" ? Theme.danger : modelData.priority === "Medium" ? Theme.warning : Theme.success
                        }
                    }

                    // Fixed-width action column so the view/edit/delete
                    // icons land in the same spot on every row, regardless
                    // of title/category text length.
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
                                onClicked: taskDetailsDialog.openFor(modelData)
                            }
                        }

                        ActionIcon {
                            kind: "edit"
                            size: 18
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    taskDetailsDialog.openFor(modelData);
                                    taskDetailsDialog.mode = "edit";
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
                                    deleteConfirm.taskId = modelData.id
                                    deleteConfirm.message = "Delete task \"" + modelData.title + "\"? This cannot be undone."
                                    deleteConfirm.open()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    AddTaskDialog {
        id: addTaskDialog
    }

    TaskDetailsDialog {
        id: taskDetailsDialog
    }

    ConfirmDialog {
        id: deleteConfirm
        property int taskId: -1
        confirmText: "Delete"
        onConfirmed: revolif.deleteTask(taskId)
    }
}
