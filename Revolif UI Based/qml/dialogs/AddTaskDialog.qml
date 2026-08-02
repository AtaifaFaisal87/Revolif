import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../"
import "../JsHelpers.js" as JsHelpers

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
    width: 520

    readonly property var monthNames: ["January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"]
    readonly property var academicCategories: ["Exam", "Assignment", "Project", "Midterm",
        "Test", "Submission", "Viva", "Presentation", "Other"]
    readonly property var dailyCategories: ["Laundry", "Cleaning House", "Washing Dishes",
        "Exercise", "Grocery Shopping", "Cooking", "Reading", "Other"]
    readonly property var categoryModel: taskType.currentIndex === 1 ? dailyCategories : academicCategories
    readonly property var dayNumbers: Array.from({length: 31}, function(_, i) { return i + 1 })
    readonly property var yearNumbers: {
        var y = new Date().getFullYear();
        var a = [];
        for (var i = y; i <= y + 5; i++) a.push(i);
        return a;
    }

    function resetDate() {
        var now = new Date();
        taskDay.currentIndex = now.getDate() - 1;
        taskMonth.currentIndex = now.getMonth();
        taskYear.currentIndex = 0;
    }

    // Mirrors the validation rules the console/backend applies when adding a
    // task: non-empty title/description, a real calendar date, a deadline
    // that isn't already in the past, and a real category name if "Other"
    // is picked. Returns an error string, or "" if everything is valid.
    function validate() {
        if (taskTitle.text.trim().length === 0) return "Task title is required.";
        if (taskDesc.text.trim().length === 0) return "Task description is required.";

        var day = dialog.dayNumbers[taskDay.currentIndex];
        var month = taskMonth.currentIndex + 1;
        var year = dialog.yearNumbers[taskYear.currentIndex];

        var dateError = JsHelpers.validateCalendarDate(day, month, year);
        if (dateError.length > 0) return dateError;

        if (JsHelpers.isPastDate(day, month, year)) return "Deadline can't be in the past.";

        if (taskCategory.currentText === "Other" && taskCategoryOther.text.trim().length === 0)
            return "Enter a name for the custom category.";

        return "";
    }

    onOpened: {
        resetDate();
        taskType.currentIndex = 0;
        taskCategory.currentIndex = 0;
        taskCategoryOther.text = "";
    }

    contentItem: ColumnLayout {
        spacing: 12

        Label { text: "Add Task"; font.pixelSize: Theme.fontSizeLg; font.bold: true; color: Theme.textPrimary }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                Label { text: "Type"; font.pixelSize: 12; color: Theme.textSecondary }
                AppComboBox {
                    id: taskType
                    Layout.fillWidth: true
                    model: ["Academic", "Daily"]
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                Label { text: "Priority"; font.pixelSize: 12; color: Theme.textSecondary }
                AppComboBox {
                    id: taskPriority
                    Layout.fillWidth: true
                    model: ["High", "Medium", "Low"]
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            Label { text: "Category"; font.pixelSize: 12; color: Theme.textSecondary }
            AppComboBox {
                id: taskCategory
                Layout.fillWidth: true
                model: dialog.categoryModel
                onModelChanged: currentIndex = 0
            }
        }

        AppTextField {
            id: taskCategoryOther
            Layout.fillWidth: true
            visible: taskCategory.currentText === "Other"
            placeholderText: "Enter custom category"
        }

        Label { text: "Title"; font.pixelSize: 12; color: Theme.textSecondary }
        AppTextField { id: taskTitle; Layout.fillWidth: true }

        Label { text: "Description"; font.pixelSize: 12; color: Theme.textSecondary }
        AppTextField { id: taskDesc; Layout.fillWidth: true }

        Label { text: "Deadline"; font.pixelSize: 12; color: Theme.textSecondary }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            AppComboBox { id: taskDay; Layout.preferredWidth: 80; model: dialog.dayNumbers }
            AppComboBox { id: taskMonth; Layout.fillWidth: true; model: dialog.monthNames }
            AppComboBox { id: taskYear; Layout.preferredWidth: 100; model: dialog.yearNumbers }
        }

        Label { text: "Time"; font.pixelSize: 12; color: Theme.textSecondary }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            AppSpinBox { id: taskHour; from: 1; to: 12; value: 12; Layout.preferredWidth: 90 }
            Label { text: ":"; font.pixelSize: 14; color: Theme.textSecondary }
            AppSpinBox { id: taskMin; from: 0; to: 59; value: 0; Layout.preferredWidth: 90 }
            AppComboBox { id: taskMeridiem; Layout.preferredWidth: 90; model: ["AM", "PM"] }
            Item { Layout.fillWidth: true }
        }

        RowLayout {
            spacing: 12
            CheckBox { id: taskRecurring; text: "Recurring" }
            AppComboBox {
                id: taskInterval
                enabled: taskRecurring.checked
                model: ["Daily", "Weekly", "Monthly"]
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            AnimatedButton {
                Layout.fillWidth: true
                text: "Cancel"
                primary: false
                onClicked: dialog.close()
            }

            AnimatedButton {
                Layout.fillWidth: true
                text: "Add Task"
                primary: true
                onClicked: {
                    var err = dialog.validate();
                    if (err.length > 0) {
                        revolif.errorOccurred(err);
                        return;
                    }
                    var typeNum = taskType.currentIndex + 1;
                    var categoryValue = taskCategory.currentText === "Other"
                            ? taskCategoryOther.text.trim()
                            : taskCategory.currentText;
                    if (categoryValue.length === 0) categoryValue = taskType.currentText;
                    var ok = revolif.addTask(typeNum, taskTitle.text, taskDesc.text,
                                    dialog.dayNumbers[taskDay.currentIndex],
                                    taskMonth.currentIndex + 1,
                                    dialog.yearNumbers[taskYear.currentIndex],
                                    taskHour.value, taskMin.value, taskMeridiem.currentText,
                                    taskPriority.currentText, taskRecurring.checked, taskInterval.currentText,
                                    categoryValue);
                    if (ok) dialog.close();
                }
            }
        }
    }
}
