import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import "../"

Rectangle {
    id: root
    width: 100
    height: 100
    color: "transparent"

    property real progress: 0.65
    property color trackColor: Theme.border
    property color progressColor: Theme.primary
    property real strokeWidth: 8

    Shape {
        anchors.fill: parent
        ShapePath {
            strokeColor: root.trackColor
            strokeWidth: root.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: (root.width - root.strokeWidth) / 2
                radiusY: (root.height - root.strokeWidth) / 2
                startAngle: 0
                sweepAngle: 360
            }
        }
        ShapePath {
            strokeColor: root.progressColor
            strokeWidth: root.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: (root.width - root.strokeWidth) / 2
                radiusY: (root.height - root.strokeWidth) / 2
                startAngle: 90
                sweepAngle: -360 * root.progress
            }
        }
    }

    Column {
        anchors.centerIn: parent
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Math.round(root.progress * 100) + "%"
            font.bold: true
            font.pixelSize: 18
            color: Theme.textPrimary
        }
    }
}
