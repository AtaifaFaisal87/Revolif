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
        for (var i = y - 1; i <= y + 5; i++) a.push(i);
        return a;
    }

    function resetDate() {
        var now = new Date();
        expDay.currentIndex = now.getDate() - 1;
        expMonth.currentIndex = now.getMonth();
        expYear.currentIndex = yearNumbers.indexOf(now.getFullYear());
    }

    // Expense dates are allowed to be in the past (you're logging spending
    // that already happened), matching the console's default, non-restricted
    // date input for expenses. Only the calendar date itself, amount, and
    // required text fields are validated.
    function validate() {
        if (expTitle.text.trim().length === 0) return "Expense title is required.";
        if (expDesc.text.trim().length === 0) return "Expense description is required.";

        var amount = parseFloat(expAmount.text);
        if (isNaN(amount) || amount <= 0) return "Enter a valid expense amount greater than 0.";

        var day = dialog.dayNumbers[expDay.currentIndex];
        var month = expMonth.currentIndex + 1;
        var year = dialog.yearNumbers[expYear.currentIndex];
        var dateError = JsHelpers.validateCalendarDate(day, month, year);
        if (dateError.length > 0) return dateError;

        if (expCategory.currentText === "Other" && expCategoryOther.text.trim().length === 0)
            return "Enter a name for the custom category.";

        return "";
    }

    onOpened: {
        resetDate();
        expCategoryOther.text = "";
    }

    contentItem: ColumnLayout {
        spacing: 12

        Label { text: "Add Expense"; font.pixelSize: Theme.fontSizeLg; font.bold: true; color: Theme.textPrimary }

        Label { text: "Title"; font.pixelSize: 12; color: Theme.textSecondary }
        AppTextField { id: expTitle; Layout.fillWidth: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                Label { text: "Category"; font.pixelSize: 12; color: Theme.textSecondary }
                AppComboBox {
                    id: expCategory
                    Layout.fillWidth: true
                    model: ["Food", "Transport", "Education", "Shopping", "Bills", "Entertainment", "Health", "Other"]
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                Label { text: "Amount"; font.pixelSize: 12; color: Theme.textSecondary }
                AppTextField { id: expAmount; Layout.fillWidth: true; text: "0.00" }
            }
        }

        AppTextField {
            id: expCategoryOther
            Layout.fillWidth: true
            visible: expCategory.currentText === "Other"
            placeholderText: "Enter custom category"
        }

        Label { text: "Date"; font.pixelSize: 12; color: Theme.textSecondary }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            AppComboBox { id: expDay; Layout.preferredWidth: 80; model: dialog.dayNumbers }
            AppComboBox { id: expMonth; Layout.fillWidth: true; model: dialog.monthNames }
            AppComboBox { id: expYear; Layout.preferredWidth: 100; model: dialog.yearNumbers }
        }

        Label { text: "Description"; font.pixelSize: 12; color: Theme.textSecondary }
        AppTextField { id: expDesc; Layout.fillWidth: true }

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
                text: "Add Expense"
                primary: true
                onClicked: {
                    var err = dialog.validate();
                    if (err.length > 0) {
                        revolif.errorOccurred(err);
                        return;
                    }
                    var categoryValue = expCategory.currentText === "Other"
                            ? expCategoryOther.text.trim()
                            : expCategory.currentText;
                    var ok = revolif.addExpense(expTitle.text, parseFloat(expAmount.text), categoryValue,
                                       dialog.dayNumbers[expDay.currentIndex],
                                       expMonth.currentIndex + 1,
                                       dialog.yearNumbers[expYear.currentIndex], expDesc.text);
                    if (ok) dialog.close();
                }
            }
        }
    }
}
