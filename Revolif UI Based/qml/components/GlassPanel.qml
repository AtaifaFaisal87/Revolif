import QtQuick
import "../"

// The dark card surface used throughout the Admin Dashboard, floating on
// the light control-center canvas. Distinct from the light, solid Card.qml
// used on the user side.
Rectangle {
    id: panel
    color: AdminTheme.surface
    radius: Theme.radiusLg
    border.color: AdminTheme.border
    border.width: 1

    default property alias content: inner.data

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: parent.radius - 1
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.03) }
            GradientStop { position: 0.3; color: Qt.rgba(1, 1, 1, 0.0) }
        }
    }

    Item {
        id: inner
        anchors.fill: parent
    }
}
