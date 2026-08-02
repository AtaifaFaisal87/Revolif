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
    width: 420

    readonly property var monthNames: ["January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"]
    readonly property var dayNumbers: Array.from({length: 31}, function(_, i) { return i + 1 })
    readonly property var yearNumbers: {
        var y = new Date().getFullYear();
        var a = [];
        for (var i = y; i <= y + 5; i++) a.push(i);
        return a;
    }

    function resetDate() {
        var now = new Date();
        goalDay.currentIndex = now.getDate() - 1;
        goalMonth.currentIndex = now.getMonth();
        goalYear.currentIndex = 0;
    }

    function validate() {
        if (goalTitle.text.trim().length === 0) return "Goal title is required.";
        if (goalDesc.text.trim().length === 0) return "Goal description is required.";

        var day = dialog.dayNumbers[goalDay.currentIndex];
        var month = goalMonth.currentIndex + 1;
        var year = dialog.yearNumbers[goalYear.currentIndex];

        var dateError = JsHelpers.validateCalendarDate(day, month, year);
        if (dateError.length > 0) return dateError;

        if (JsHelpers.isPastDate(day, month, year)) return "Deadline can't be in the past.";

        if (goalCategory.currentText === "Other" && goalCategoryOther.text.trim().length === 0)
            return "Enter a name for the custom category.";

        return "";
    }

    onOpened: {
        resetDate();
        goalCategory.currentIndex = 0;
        goalCategoryOther.text = "";
    }

    contentItem: ColumnLayout {
        spacing: 12

        Label { text: "Add Goal"; font.pixelSize: Theme.fontSizeLg; font.bold: true; color: Theme.textPrimary }

        Label { text: "Title"; font.pixelSize: 12; color: Theme.textSecondary }
        AppTextField { id: goalTitle; Layout.fillWidth: true }

        Label { text: "Category"; font.pixelSize: 12; color: Theme.textSecondary }
        AppComboBox {
            id: goalCategory
            Layout.fillWidth: true
            model: ["Academic", "Career", "Health", "Personal Development", "Financial", "Other"]
        }

        AppTextField {
            id: goalCategoryOther
            Layout.fillWidth: true
            visible: goalCategory.currentText === "Other"
            placeholderText: "Enter custom category"
        }

        Label { text: "Description"; font.pixelSize: 12; color: Theme.textSecondary }
        AppTextField { id: goalDesc; Layout.fillWidth: true }

        Label { text: "Deadline"; font.pixelSize: 12; color: Theme.textSecondary }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            AppComboBox { id: goalDay; Layout.preferredWidth: 80; model: dialog.dayNumbers }
            AppComboBox { id: goalMonth; Layout.fillWidth: true; model: dialog.monthNames }
            AppComboBox { id: goalYear; Layout.preferredWidth: 100; model: dialog.yearNumbers }
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
                text: "Add Goal"
                primary: true
                onClicked: {
                    var err = dialog.validate();
                    if (err.length > 0) {
                        revolif.errorOccurred(err);
                        return;
                    }
                    var categoryValue = goalCategory.currentText === "Other"
                            ? goalCategoryOther.text.trim()
                            : goalCategory.currentText;
                    var ok = revolif.addGoal(goalTitle.text, goalDesc.text, categoryValue,
                                    dialog.dayNumbers[goalDay.currentIndex],
                                    goalMonth.currentIndex + 1,
                                    dialog.yearNumbers[goalYear.currentIndex]);
                    if (ok) dialog.close();
                }
            }
        }
    }
}
