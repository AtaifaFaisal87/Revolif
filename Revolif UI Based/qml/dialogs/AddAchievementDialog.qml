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

    // Explicitly parent to the window's overlay so centering is always
    // relative to the full app window. Without this, "parent" falls back
    // to whatever page Rectangle this dialog happens to be declared inside
    // (e.g. AdminDashboardPage), so x/y centered against that page's
    // content area instead of the whole screen -- which is why the popup
    // showed up shifted off to one side, cut off by the sidebar.
    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    width: 420

    // -1 means "add new"; any other value means "editing that achievement id"
    property int editId: -1

    function openForAdd() {
        editId = -1
        reset()
        open()
    }

    function openForEdit(id, achName_, achDesc_, requiredGoals) {
        editId = id
        achName.text = achName_
        achDesc.text = achDesc_
        achGoals.value = requiredGoals
        open()
    }

    function reset() {
        achName.text = ""
        achDesc.text = ""
        achGoals.value = 1
    }

    contentItem: ColumnLayout {
        spacing: 12

        Label {
            text: dialog.editId >= 0 ? "Edit Achievement" : "Add Achievement"
            font.pixelSize: Theme.fontSizeLg
            font.bold: true
            color: Theme.textPrimary
        }

        Label { text: "Name"; font.pixelSize: 12; color: Theme.textSecondary }
        AppTextField { id: achName; Layout.fillWidth: true }

        Label { text: "Description"; font.pixelSize: 12; color: Theme.textSecondary }
        AppTextField { id: achDesc; Layout.fillWidth: true }

        Label { text: "Required Goals"; font.pixelSize: 12; color: Theme.textSecondary }
        AppSpinBox { id: achGoals; from: 1; to: 1000; value: 1; Layout.fillWidth: true }

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
                text: dialog.editId >= 0 ? "Save Changes" : "Add Achievement"
                primary: true
                onClicked: {
                    if (achName.text.trim().length === 0) {
                        revolif.errorOccurred("Achievement name is required.");
                        return;
                    }
                    if (achDesc.text.trim().length === 0) {
                        revolif.errorOccurred("Achievement description is required.");
                        return;
                    }
                    var ok;
                    if (dialog.editId >= 0) {
                        ok = revolif.updateAchievement(dialog.editId, "name", achName.text)
                        ok = revolif.updateAchievement(dialog.editId, "description", achDesc.text) && ok
                        ok = revolif.updateAchievement(dialog.editId, "requiredGoals", achGoals.value) && ok
                    } else {
                        ok = revolif.addAchievement(achName.text, achDesc.text, achGoals.value);
                    }
                    if (ok) dialog.close();
                }
            }
        }
    }
}
