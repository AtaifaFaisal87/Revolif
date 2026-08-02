import QtQuick
import QtQuick.Controls
import "../"

// Themed ComboBox to match AppTextField. Qt's default ComboBox popup
// renders as a plain white list regardless of app theme, which is what
// made dropdowns look out of place in dark mode — this restyles the
// field itself and the popup/list so both follow Theme colors.
ComboBox {
    id: control

    implicitHeight: Theme.inputHeight
    font.pixelSize: Theme.fontSizeBase

    background: Rectangle {
        antialiasing: true
        radius: Theme.inputRadius
        color: control.activeFocus || control.popup.visible ? Theme.inputBgFocus : Theme.inputBg
        border.color: control.activeFocus || control.popup.visible ? Theme.inputBorderFocus : Theme.inputBorder
        border.width: control.activeFocus || control.popup.visible ? 2 : 1

        Behavior on border.color { ColorAnimation { duration: 130 } }
    }

    contentItem: Text {
        text: control.displayText
        font: control.font
        color: Theme.textPrimary
        leftPadding: 16
        rightPadding: 36
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Text {
        x: control.width - width - 14
        y: (control.height - height) / 2
        text: "\u25BE"
        font.pixelSize: 13
        color: Theme.textSecondary
    }

    popup: Popup {
        y: control.height + 6
        width: control.width
        implicitHeight: Math.min(listView.contentHeight + 12, 260)
        padding: 6
        clip: true

        background: Rectangle {
            antialiasing: true
            color: Theme.cardBg
            radius: Theme.radiusBase
            border.color: Theme.border
            border.width: 1
        }

        contentItem: ListView {
            id: listView
            clip: true
            implicitHeight: contentHeight
            // Bind directly to the delegateModel rather than swapping it for
            // null while closed -- that ternary made the list populate a
            // frame late after opening (height computed before delegates
            // existed), which showed up as a small stray highlight box
            // flashing above the popup.
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }

    delegate: ItemDelegate {
        width: control.width
        height: 40
        hoverEnabled: true

        contentItem: Text {
            // control.textAt(index) is ComboBox's own built-in role
            // resolution -- it always matches textRole correctly regardless
            // of whether the model is a plain array, an array of objects,
            // or a QVariantList from C++. The old code re-implemented this
            // by hand (Array.isArray(control.model) ? modelData[...] :
            // model[...]) and it was resolving to undefined, which is why
            // every row rendered with no visible text.
            text: control.textAt(index)
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeBase
            verticalAlignment: Text.AlignVCenter
            leftPadding: 14
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: Theme.radiusSm
            color: control.highlightedIndex === index ? Theme.selectedBg
                   : (hovered ? Theme.hoverBg : "transparent")
        }
    }
}
