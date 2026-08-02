import QtQuick
import QtQuick.Controls
import "../"

Rectangle {
    id: toast
    width: toastLabel.implicitWidth + 48
    height: 48
    radius: Theme.radiusBase
    color: type === "error" ? Theme.danger : type === "success" ? Theme.success : Theme.primary
    opacity: 0
    visible: opacity > 0

    property string message: ""
    property string type: "success"

    Label {
        id: toastLabel
        anchors.centerIn: parent
        text: toast.message
        color: "white"
        font.bold: true
        font.pixelSize: Theme.fontSizeSm
    }

    function show(msg, t) {
        toast.message = msg
        toast.type = t || "success"
        toast.opacity = 1
        hideTimer.restart()
    }

    Timer {
        id: hideTimer
        interval: 3000
        onTriggered: toast.opacity = 0
    }

    Behavior on opacity {
        NumberAnimation { duration: 300 }
    }
}
