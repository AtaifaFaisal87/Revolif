import QtQuick
import QtQuick.Controls
import "../"

// Multi-segment donut chart. Feed it a list of {name, value, color} objects
// via `segments` and it works out each slice's angle itself.
//
// Rendered on a Canvas (not QtQuick.Shapes) because PathAngleArc's
// Repeater-driven strokes were coming out blank on some backends -- the
// canvas 2D arc() call is far more predictable across platforms/themes.
Item {
    id: root
    width: 180
    height: 180

    property var segments: []   // [{ name, value, color }]
    property real strokeWidth: 26

    readonly property real total: {
        var t = 0;
        for (var i = 0; i < segments.length; i++) t += segments[i].value;
        return t;
    }

    Canvas {
        id: canvas
        anchors.fill: parent

        // Repaint whenever anything that affects the drawing changes --
        // new data, resize, or a light/dark theme swap (track + colors).
        property color trackColor: Theme.border
        onTrackColorChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        Connections {
            target: root
            function onSegmentsChanged() { canvas.requestPaint() }
        }

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();

            var cx = width / 2;
            var cy = height / 2;
            var r = (Math.min(width, height) - root.strokeWidth) / 2;
            var lineW = root.strokeWidth;

            // Base track (full ring) underneath everything.
            ctx.beginPath();
            ctx.lineWidth = lineW;
            ctx.strokeStyle = trackColor;
            ctx.lineCap = "butt";
            ctx.arc(cx, cy, r, 0, Math.PI * 2, false);
            ctx.stroke();

            if (root.total <= 0 || root.segments.length === 0) return;

            var startAngle = -Math.PI / 2; // 12 o'clock
            for (var i = 0; i < root.segments.length; i++) {
                var seg = root.segments[i];
                var fraction = seg.value / root.total;
                if (fraction <= 0) continue;
                var sweep = fraction * Math.PI * 2;
                var endAngle = startAngle + sweep;

                ctx.beginPath();
                ctx.lineWidth = lineW;
                ctx.strokeStyle = seg.color;
                ctx.lineCap = "butt";
                ctx.arc(cx, cy, r, startAngle, endAngle, false);
                ctx.stroke();

                startAngle = endAngle;
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 2
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: revolif.currencySymbol + root.total.toFixed(0)
            font.bold: true
            font.pixelSize: 18
            color: Theme.textPrimary
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "spent"
            font.pixelSize: 11
            color: Theme.textSecondary
        }
    }
}
