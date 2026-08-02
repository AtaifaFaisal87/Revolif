import QtQuick
import QtQuick.Controls
import "../"

Rectangle {
    id: btn

    property string text: "Button"
    property bool primary: false
    property int horizontalPadding: 20
    // When true, renders with the neutral dark "control center" palette
    // used by the Admin experience instead of the light Theme colors used
    // on the User Dashboard. Defaults to false so existing User Dashboard
    // buttons are completely unaffected.
    property bool dark: false
    property bool hovered: false
    signal clicked()

    // Admin-only palette (kept local so the shared light Theme used by the
    // User Dashboard is never touched).
    readonly property color adminPrimary: "#10B981"
    readonly property color adminSurface: "#1F2937"
    readonly property color adminSurfaceHover: "#2B3646"
    readonly property color adminBorder: "#374151"
    readonly property color adminText: "#F9FAFB"

    implicitWidth: Math.max(label.implicitWidth + horizontalPadding * 2, 96)
    implicitHeight: 48

    radius: Theme.radiusBase
    color: primary
           ? (dark ? adminPrimary : (hovered ? Theme.accentHover : Theme.primary))
           : (dark ? (hovered ? adminSurfaceHover : adminSurface) : "transparent")
    border.color: primary ? "transparent" : (dark ? adminBorder : Theme.border)
    border.width: primary ? 0 : 1

    Behavior on color { ColorAnimation { duration: 120 } }

    Label {
        id: label
        anchors.centerIn: parent
        text: btn.text
        color: btn.primary ? (btn.dark ? "#FFFFFF" : Theme.textOnPrimary) : (btn.dark ? btn.adminText : Theme.textPrimary)
        font.bold: true
        font.pixelSize: Theme.fontSizeBase
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: btn.hovered = true
        onExited: btn.hovered = false
        onPressed: btn.scale = 0.97
        onReleased: btn.scale = 1.0
        onClicked: btn.clicked()
    }

    Behavior on scale {
        NumberAnimation { duration: 100 }
    }
}
