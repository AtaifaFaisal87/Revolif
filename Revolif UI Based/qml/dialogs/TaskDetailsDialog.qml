import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../"

Dialog {
    id: dialog
    modal: true
    standardButtons: Dialog.NoButton

    background: Rectangle {
        antialiasing: true
        color: Theme.cardBg
        radius: Theme.radiusLg
        border.color: Theme.border
        border.width: 1
    }
    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, Theme.isDark ? 0.55 : 0.35) }

    // See AddAchievementDialog.qml for why this parent binding is needed.
    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    width: 460

    // "view" shows read-only details, "edit" swaps the editable fields in.
    property string mode: "view"

    property int taskId: -1
    property string taskTitle: ""
    property string taskDescription: ""
    property string taskCategory: ""
    property string taskType: ""
    property string taskPriority: ""
    property string taskStatus: ""
    property string taskDeadline: ""
    property string taskTime: ""
    property bool taskRecurring: false
    property string taskInterval: ""

    function openFor(task) {
        dialog.taskId = task.id;
        dialog.taskTitle = task.title;
        dialog.taskDescription = task.description;
        dialog.taskCategory = task.category;
        dialog.taskType = task.type;
        dialog.taskPriority = task.priority;
        dialog.taskStatus = task.status;
        dialog.taskDeadline = task.deadline;
        dialog.taskTime = task.time;
        dialog.taskRecurring = task.isRecurring;
        dialog.taskInterval = task.recurrenceInterval;
        dialog.mode = "view";
        dialog.open();
    }

    onOpened: {
        editTitle.text = dialog.taskTitle;
        editDesc.text = dialog.taskDescription;
        editCategory.text = dialog.taskCategory;
        editPriority.currentIndex = Math.max(0, ["High", "Medium", "Low"].indexOf(dialog.taskPriority));
    }

    function validate() {
        if (editTitle.text.trim().length === 0) return "Task title is required.";
        if (editDesc.text.trim().length === 0) return "Task description is required.";
        return "";
    }

    function saveEdits() {
        var err = dialog.validate();
        if (err.length > 0) {
            revolif.errorOccurred(err);
            return;
        }
        revolif.updateTask(dialog.taskId, "title", editTitle.text.trim());
        revolif.updateTask(dialog.taskId, "description", editDesc.text.trim());
        revolif.updateTask(dialog.taskId, "category", editCategory.text.trim());
        revolif.updateTask(dialog.taskId, "priority", editPriority.currentText);

        dialog.taskTitle = editTitle.text.trim();
        dialog.taskDescription = editDesc.text.trim();
        dialog.taskCategory = editCategory.text.trim();
        dialog.taskPriority = editPriority.currentText;
        dialog.mode = "view";
    }

    contentItem: ColumnLayout {
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: dialog.mode === "edit" ? "Edit Task" : "Task Details"
                font.pixelSize: Theme.fontSizeLg
                font.bold: true
                color: Theme.textPrimary
                Layout.fillWidth: true
            }
            Rectangle {
                visible: dialog.mode === "view"
                Layout.preferredWidth: 76
                Layout.preferredHeight: 26
                radius: Theme.radiusFull
                color: dialog.taskPriority === "High" ? "#FDE8E8" : dialog.taskPriority === "Medium" ? "#FEF3C7" : "#D1FAE5"
                Label {
                    anchors.centerIn: parent
                    text: dialog.taskPriority
                    font.pixelSize: 11
                    font.bold: true
                    color: dialog.taskPriority === "High" ? Theme.danger : dialog.taskPriority === "Medium" ? Theme.warning : Theme.success
                }
            }
        }

        // ---- View mode ----
        ColumnLayout {
            visible: dialog.mode === "view"
            Layout.fillWidth: true
            spacing: 10

            Label { text: dialog.taskTitle; font.bold: true; font.pixelSize: Theme.fontSizeBase; color: Theme.textPrimary; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: dialog.taskDescription; font.pixelSize: Theme.fontSizeSm; color: Theme.textSecondary; wrapMode: Text.WordWrap; Layout.fillWidth: true }

            GridLayout {
                columns: 2
                columnSpacing: 16
                rowSpacing: 8
                Layout.fillWidth: true
                Layout.topMargin: 4

                Label { text: "Type"; font.pixelSize: 12; color: Theme.textMuted }
                Label { text: dialog.taskType; font.pixelSize: Theme.fontSizeSm; color: Theme.textPrimary }

                Label { text: "Category"; font.pixelSize: 12; color: Theme.textMuted }
                Label { text: dialog.taskCategory; font.pixelSize: Theme.fontSizeSm; color: Theme.textPrimary }

                Label { text: "Status"; font.pixelSize: 12; color: Theme.textMuted }
                Label { text: dialog.taskStatus; font.pixelSize: Theme.fontSizeSm; color: Theme.textPrimary }

                Label { text: "Deadline"; font.pixelSize: 12; color: Theme.textMuted }
                Label { text: dialog.taskDeadline + (dialog.taskTime ? " · " + dialog.taskTime : ""); font.pixelSize: Theme.fontSizeSm; color: Theme.textPrimary }

                Label { text: "Recurring"; font.pixelSize: 12; color: Theme.textMuted }
                Label { text: dialog.taskRecurring ? ("Yes · " + dialog.taskInterval) : "No"; font.pixelSize: Theme.fontSizeSm; color: Theme.textPrimary }
            }
        }

        // ---- Edit mode ----
        ColumnLayout {
            visible: dialog.mode === "edit"
            Layout.fillWidth: true
            spacing: 10

            Label { text: "Title"; font.pixelSize: 12; color: Theme.textSecondary }
            AppTextField { id: editTitle; Layout.fillWidth: true }

            Label { text: "Description"; font.pixelSize: 12; color: Theme.textSecondary }
            AppTextField { id: editDesc; Layout.fillWidth: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Label { text: "Category"; font.pixelSize: 12; color: Theme.textSecondary }
                    AppTextField { id: editCategory; Layout.fillWidth: true }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Label { text: "Priority"; font.pixelSize: 12; color: Theme.textSecondary }
                    AppComboBox { id: editPriority; Layout.fillWidth: true; model: ["High", "Medium", "Low"] }
                }
            }

            Label {
                text: "Type, deadline and status can't be changed here."
                font.pixelSize: 11
                font.italic: true
                color: Theme.textMuted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            AnimatedButton {
                Layout.fillWidth: true
                text: dialog.mode === "edit" ? "Cancel" : "Close"
                primary: false
                onClicked: {
                    if (dialog.mode === "edit") dialog.mode = "view";
                    else dialog.close();
                }
            }

            AnimatedButton {
                Layout.fillWidth: true
                text: dialog.mode === "edit" ? "Save" : "Edit"
                primary: true
                onClicked: {
                    if (dialog.mode === "edit") dialog.saveEdits();
                    else dialog.mode = "edit";
                }
            }
        }
    }
}
