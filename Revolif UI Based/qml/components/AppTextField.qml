import QtQuick
import QtQuick.Controls
import "../"

// A standard, professional text input used across the entire app —
// Dashboard, Auth screen, dialogs, settings, profile — so every field
// shares the same broad height, padding, and theme-correct fill/border
// instead of each screen hand-rolling its own (often too-thin, unstyled)
// TextField.
TextField {
    id: control

    implicitHeight: Theme.inputHeight
    leftPadding: 16
    rightPadding: 16
    topPadding: 0
    bottomPadding: 0
    verticalAlignment: TextInput.AlignVCenter

    font.pixelSize: Theme.fontSizeBase
    color: Theme.textPrimary
    placeholderTextColor: Theme.inputPlaceholder
    selectionColor: Theme.accent
    selectedTextColor: Theme.textOnPrimary

    background: Rectangle {
        antialiasing: true
        radius: Theme.inputRadius
        color: control.activeFocus ? Theme.inputBgFocus : Theme.inputBg
        border.color: control.activeFocus ? Theme.inputBorderFocus : Theme.inputBorder
        border.width: control.activeFocus ? 2 : 1

        Behavior on border.color { ColorAnimation { duration: 130 } }
        Behavior on color { ColorAnimation { duration: 130 } }
    }
}
