import QtQuick
import "../"

// Small line-drawn icon used for row actions (view / edit / delete).
// Drawn on a Canvas instead of using emoji characters (👁 ✎ 🗑) so it
// always renders as a plain, single-color glyph that matches the app's
// theme, instead of a platform-specific colorful emoji graphic.
Item {
    id: root

    // "eye" | "edit" | "trash"
    property string kind: "eye"
    property color color: Theme.textMuted
    property int size: 18

    implicitWidth: size
    implicitHeight: size

    Canvas {
        id: canvas
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            var w = width;
            var h = height;
            ctx.strokeStyle = root.color;
            ctx.fillStyle = root.color;
            ctx.lineWidth = Math.max(1.3, w * 0.09);
            ctx.lineCap = "round";
            ctx.lineJoin = "round";

            if (root.kind === "eye") {
                ctx.beginPath();
                ctx.moveTo(w * 0.06, h * 0.5);
                ctx.quadraticCurveTo(w * 0.5, h * 0.08, w * 0.94, h * 0.5);
                ctx.quadraticCurveTo(w * 0.5, h * 0.92, w * 0.06, h * 0.5);
                ctx.stroke();
                ctx.beginPath();
                ctx.arc(w * 0.5, h * 0.5, w * 0.13, 0, Math.PI * 2);
                ctx.fill();
            } else if (root.kind === "edit") {
                // Pencil body
                ctx.beginPath();
                ctx.moveTo(w * 0.16, h * 0.84);
                ctx.lineTo(w * 0.62, h * 0.30);
                ctx.stroke();
                // Pencil tip
                ctx.beginPath();
                ctx.moveTo(w * 0.62, h * 0.30);
                ctx.lineTo(w * 0.80, h * 0.12);
                ctx.lineTo(w * 0.88, h * 0.20);
                ctx.lineTo(w * 0.70, h * 0.38);
                ctx.closePath();
                ctx.fill();
                // Eraser end mark
                ctx.beginPath();
                ctx.moveTo(w * 0.12, h * 0.90);
                ctx.lineTo(w * 0.22, h * 0.78);
                ctx.lineTo(w * 0.16, h * 0.84);
                ctx.closePath();
                ctx.fill();
            } else if (root.kind === "trash") {
                // Lid
                ctx.beginPath();
                ctx.moveTo(w * 0.16, h * 0.28);
                ctx.lineTo(w * 0.84, h * 0.28);
                ctx.stroke();
                ctx.beginPath();
                ctx.moveTo(w * 0.38, h * 0.28);
                ctx.lineTo(w * 0.42, h * 0.14);
                ctx.lineTo(w * 0.58, h * 0.14);
                ctx.lineTo(w * 0.62, h * 0.28);
                ctx.stroke();
                // Body
                ctx.beginPath();
                ctx.moveTo(w * 0.22, h * 0.28);
                ctx.lineTo(w * 0.28, h * 0.90);
                ctx.lineTo(w * 0.72, h * 0.90);
                ctx.lineTo(w * 0.78, h * 0.28);
                ctx.stroke();
                // Ridges
                ctx.beginPath();
                ctx.moveTo(w * 0.40, h * 0.42);
                ctx.lineTo(w * 0.42, h * 0.78);
                ctx.moveTo(w * 0.60, h * 0.42);
                ctx.lineTo(w * 0.58, h * 0.78);
                ctx.stroke();
            }
        }
    }

    onColorChanged: canvas.requestPaint()
    onKindChanged: canvas.requestPaint()
    Component.onCompleted: canvas.requestPaint()
}
