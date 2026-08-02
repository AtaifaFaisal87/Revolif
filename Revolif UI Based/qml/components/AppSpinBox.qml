import QtQuick
import QtQuick.Controls
import "../"

// Themed SpinBox matching AppTextField/AppComboBox.
SpinBox {
    id: control

    implicitWidth: 140
    implicitHeight: Theme.inputHeight
    font.pixelSize: Theme.fontSizeBase
    // SpinBox.editable defaults to false, which is why typing a value in
    // directly wasn't possible before -- only the scroll/+/- interactions
    // worked. This turns the text area into a real, focusable input.
    editable: true

    background: Rectangle {
        antialiasing: true
        radius: Theme.inputRadius
        color: control.activeFocus ? Theme.inputBgFocus : Theme.inputBg
        border.color: control.activeFocus ? Theme.inputBorderFocus : Theme.inputBorder
        border.width: control.activeFocus ? 2 : 1
    }

    contentItem: TextInput {
        text: control.textFromValue(control.value, control.locale)
        font: control.font
        color: Theme.textPrimary
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        // Keep the typed text clear of the +/- buttons, which are docked
        // 6px from the right edge at 28px wide each.
        leftPadding: 12
        rightPadding: 44
        readOnly: !control.editable
        validator: control.validator
        selectByMouse: true
        selectionColor: Theme.accent
        activeFocusOnTab: true
    }

    up.indicator: Rectangle {
        x: control.width - width - 6
        y: 6
        width: 28
        height: (control.height - 12) / 2
        radius: 8
        color: control.up.pressed ? Theme.hoverBg : "transparent"
        Text { text: "+"; anchors.centerIn: parent; color: Theme.textSecondary; font.pixelSize: 14; font.bold: true }
    }

    down.indicator: Rectangle {
        x: control.width - width - 6
        y: control.height - height - 6
        width: 28
        height: (control.height - 12) / 2
        radius: 8
        color: control.down.pressed ? Theme.hoverBg : "transparent"
        Text { text: "\u2212"; anchors.centerIn: parent; color: Theme.textSecondary; font.pixelSize: 14; font.bold: true }
    }
}
