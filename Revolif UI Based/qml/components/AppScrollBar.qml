import QtQuick
import QtQuick.Controls
import "../"

// Themed scrollbar used by every ScrollView/ListView in the app instead of
// the default Qt Quick Controls ScrollBar, which renders as a plain light
// grey/white track+thumb regardless of app theme. That's what made
// scrollbars look "white" even in dark mode. This one follows Theme.isDark
// like every other control (AppTextField, AppComboBox, etc).
ScrollBar {
    id: control

    implicitWidth: 10
    policy: ScrollBar.AsNeeded

    contentItem: Rectangle {
        implicitWidth: 6
        implicitHeight: 6
        radius: width / 2
        antialiasing: true
        color: Theme.isDark
               ? Qt.rgba(1, 1, 1, control.pressed ? 0.42 : (control.hovered ? 0.32 : 0.24))
               : Qt.rgba(0, 0, 0, control.pressed ? 0.38 : (control.hovered ? 0.28 : 0.20))

        Behavior on color { ColorAnimation { duration: 120 } }
    }

    background: Rectangle {
        implicitWidth: 10
        radius: width / 2
        antialiasing: true
        color: Theme.isDark ? Qt.rgba(1, 1, 1, 0.05) : Qt.rgba(0, 0, 0, 0.04)
        visible: control.policy === ScrollBar.AlwaysOn || control.active
    }
}
