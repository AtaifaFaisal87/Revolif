import QtQuick
import QtQuick.Controls
import "../"

Rectangle {
    id: card
    color: Theme.cardBg
    radius: Theme.radiusLg
    border.color: Theme.border
    border.width: 1

    default property alias content: contentContainer.children

    Column {
        id: contentContainer
        anchors.fill: parent
        anchors.margins: 20
    }

    Behavior on scale {
        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: card.scale = 1.01
        onExited: card.scale = 1.0
    }

    Rectangle {
        z: -1
        anchors.fill: parent
        anchors.margins: -2
        radius: parent.radius + 2
        color: Qt.rgba(0.04, 0.24, 0.18, 0.06)
        visible: card.scale > 1.005
    }
}
