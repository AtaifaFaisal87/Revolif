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
    width: 420

    property string mode: "view"

    property int expenseId: -1
    property string expenseTitle: ""
    property real expenseAmount: 0
    property string expenseCategory: ""
    property string expenseDate: ""
    property string expenseDescription: ""

    function openFor(expense) {
        dialog.expenseId = expense.id;
        dialog.expenseTitle = expense.title;
        dialog.expenseAmount = expense.amount;
        dialog.expenseCategory = expense.category;
        dialog.expenseDate = expense.date;
        dialog.expenseDescription = expense.description;
        dialog.mode = "view";
        dialog.open();
    }

    onOpened: {
        editTitle.text = dialog.expenseTitle;
        editAmount.text = dialog.expenseAmount.toFixed(2);
        editCategory.text = dialog.expenseCategory;
        editDesc.text = dialog.expenseDescription;
    }

    function validate() {
        if (editTitle.text.trim().length === 0) return "Expense title is required.";
        var amount = parseFloat(editAmount.text);
        if (isNaN(amount) || amount <= 0) return "Enter a valid expense amount greater than 0.";
        return "";
    }

    function saveEdits() {
        var err = dialog.validate();
        if (err.length > 0) {
            revolif.errorOccurred(err);
            return;
        }
        var amount = parseFloat(editAmount.text);
        revolif.updateExpense(dialog.expenseId, "title", editTitle.text.trim());
        revolif.updateExpense(dialog.expenseId, "amount", amount);
        revolif.updateExpense(dialog.expenseId, "category", editCategory.text.trim());
        revolif.updateExpense(dialog.expenseId, "description", editDesc.text.trim());

        dialog.expenseTitle = editTitle.text.trim();
        dialog.expenseAmount = amount;
        dialog.expenseCategory = editCategory.text.trim();
        dialog.expenseDescription = editDesc.text.trim();
        dialog.mode = "view";
    }

    contentItem: ColumnLayout {
        spacing: 14

        Label {
            text: dialog.mode === "edit" ? "Edit Expense" : "Expense Details"
            font.pixelSize: Theme.fontSizeLg
            font.bold: true
            color: Theme.textPrimary
        }

        // ---- View mode ----
        ColumnLayout {
            visible: dialog.mode === "view"
            Layout.fillWidth: true
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Label { text: dialog.expenseTitle; font.bold: true; font.pixelSize: Theme.fontSizeBase; color: Theme.textPrimary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                Label { text: revolif.currencySymbol + dialog.expenseAmount.toFixed(2); font.bold: true; font.pixelSize: Theme.fontSizeBase; color: Theme.primary }
            }

            Label { text: dialog.expenseDescription; font.pixelSize: Theme.fontSizeSm; color: Theme.textSecondary; wrapMode: Text.WordWrap; Layout.fillWidth: true }

            GridLayout {
                columns: 2
                columnSpacing: 16
                rowSpacing: 8
                Layout.fillWidth: true
                Layout.topMargin: 4

                Label { text: "Category"; font.pixelSize: 12; color: Theme.textMuted }
                Label { text: dialog.expenseCategory; font.pixelSize: Theme.fontSizeSm; color: Theme.textPrimary }

                Label { text: "Date"; font.pixelSize: 12; color: Theme.textMuted }
                Label { text: dialog.expenseDate; font.pixelSize: Theme.fontSizeSm; color: Theme.textPrimary }
            }
        }

        // ---- Edit mode ----
        ColumnLayout {
            visible: dialog.mode === "edit"
            Layout.fillWidth: true
            spacing: 10

            Label { text: "Title"; font.pixelSize: 12; color: Theme.textSecondary }
            AppTextField { id: editTitle; Layout.fillWidth: true }

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
                    Label { text: "Amount"; font.pixelSize: 12; color: Theme.textSecondary }
                    AppTextField { id: editAmount; Layout.fillWidth: true }
                }
            }

            Label { text: "Description"; font.pixelSize: 12; color: Theme.textSecondary }
            AppTextField { id: editDesc; Layout.fillWidth: true }

            Label {
                text: "Date can't be changed here."
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
