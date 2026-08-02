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
    width: 440

    property string mode: "view"

    property int goalId: -1
    property string goalTitle: ""
    property string goalDescription: ""
    property string goalCategory: ""
    property string goalStatus: ""
    property string goalDisplayStatus: ""
    property string goalDeadline: ""

    function openFor(goal) {
        dialog.goalId = goal.id;
        dialog.goalTitle = goal.title;
        dialog.goalDescription = goal.description;
        dialog.goalCategory = goal.category;
        dialog.goalStatus = goal.status;
        dialog.goalDisplayStatus = goal.displayStatus;
        dialog.goalDeadline = goal.deadline;
        dialog.mode = "view";
        dialog.open();
    }

    onOpened: {
        editTitle.text = dialog.goalTitle;
        editDesc.text = dialog.goalDescription;
        editCategory.text = dialog.goalCategory;
    }

    function validate() {
        if (editTitle.text.trim().length === 0) return "Goal title is required.";
        if (editDesc.text.trim().length === 0) return "Goal description is required.";
        return "";
    }

    function saveEdits() {
        var err = dialog.validate();
        if (err.length > 0) {
            revolif.errorOccurred(err);
            return;
        }
        revolif.updateGoal(dialog.goalId, "title", editTitle.text.trim());
        revolif.updateGoal(dialog.goalId, "description", editDesc.text.trim());
        revolif.updateGoal(dialog.goalId, "category", editCategory.text.trim());

        dialog.goalTitle = editTitle.text.trim();
        dialog.goalDescription = editDesc.text.trim();
        dialog.goalCategory = editCategory.text.trim();
        dialog.mode = "view";
    }

    contentItem: ColumnLayout {
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: dialog.mode === "edit" ? "Edit Goal" : "Goal Details"
                font.pixelSize: Theme.fontSizeLg
                font.bold: true
                color: Theme.textPrimary
                Layout.fillWidth: true
            }
            Rectangle {
                visible: dialog.mode === "view"
                Layout.preferredWidth: 90
                Layout.preferredHeight: 26
                radius: Theme.radiusFull
                color: dialog.goalDisplayStatus === "Overdue" ? "#FDE8E8" : dialog.goalDisplayStatus === "Completed" ? "#D1FAE5" : "#E0F2FE"
                Label {
                    anchors.centerIn: parent
                    text: dialog.goalDisplayStatus
                    font.pixelSize: 11
                    font.bold: true
                    color: dialog.goalDisplayStatus === "Overdue" ? Theme.danger : dialog.goalDisplayStatus === "Completed" ? Theme.success : Theme.primary
                }
            }
        }

        // ---- View mode ----
        ColumnLayout {
            visible: dialog.mode === "view"
            Layout.fillWidth: true
            spacing: 10

            Label { text: dialog.goalTitle; font.bold: true; font.pixelSize: Theme.fontSizeBase; color: Theme.textPrimary; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: dialog.goalDescription; font.pixelSize: Theme.fontSizeSm; color: Theme.textSecondary; wrapMode: Text.WordWrap; Layout.fillWidth: true }

            GridLayout {
                columns: 2
                columnSpacing: 16
                rowSpacing: 8
                Layout.fillWidth: true
                Layout.topMargin: 4

                Label { text: "Category"; font.pixelSize: 12; color: Theme.textMuted }
                Label { text: dialog.goalCategory; font.pixelSize: Theme.fontSizeSm; color: Theme.textPrimary }

                Label { text: "Deadline"; font.pixelSize: 12; color: Theme.textMuted }
                Label { text: dialog.goalDeadline; font.pixelSize: Theme.fontSizeSm; color: Theme.textPrimary }
            }
        }

        // ---- Edit mode ----
        ColumnLayout {
            visible: dialog.mode === "edit"
            Layout.fillWidth: true
            spacing: 10

            Label { text: "Title"; font.pixelSize: 12; color: Theme.textSecondary }
            AppTextField { id: editTitle; Layout.fillWidth: true }

            Label { text: "Category"; font.pixelSize: 12; color: Theme.textSecondary }
            AppTextField { id: editCategory; Layout.fillWidth: true }

            Label { text: "Description"; font.pixelSize: 12; color: Theme.textSecondary }
            AppTextField { id: editDesc; Layout.fillWidth: true }

            Label {
                text: "Deadline and status can't be changed here."
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
