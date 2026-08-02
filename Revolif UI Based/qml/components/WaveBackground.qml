import QtQuick
import "../"

Rectangle {
    id: root
    color: Theme.isDark ? "#16302A" : "#E8F0EE"

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.isDark ? "#1B382F" : "#F0F7F5" }
            GradientStop { position: 1.0; color: Theme.isDark ? "#0F241E" : "#D4E9E3" }
        }
    }

    Canvas {
        id: waveCanvas
        anchors.fill: parent
        property bool dark: Theme.isDark
        onDarkChanged: requestPaint()
        onPaint: {
            var ctx = getContext("2d");
            var w = width;
            var h = height;
            ctx.clearRect(0, 0, w, h);

            var waveColors = dark
                ? ["#204A3E", "#193A31", "#122C25"]
                : ["#C8E6E0", "#A8D8CE", "#88CABE"];

            ctx.fillStyle = waveColors[0];
            ctx.beginPath();
            ctx.moveTo(0, h * 0.6);
            for (var x = 0; x <= w; x += 10) {
                ctx.lineTo(x, h * 0.6 + Math.sin(x / w * Math.PI * 2) * 20);
            }
            ctx.lineTo(w, h);
            ctx.lineTo(0, h);
            ctx.closePath();
            ctx.fill();

            ctx.fillStyle = waveColors[1];
            ctx.beginPath();
            ctx.moveTo(0, h * 0.7);
            for (var x2 = 0; x2 <= w; x2 += 10) {
                ctx.lineTo(x2, h * 0.7 + Math.sin(x2 / w * Math.PI * 2 + 1) * 25);
            }
            ctx.lineTo(w, h);
            ctx.lineTo(0, h);
            ctx.closePath();
            ctx.fill();

            ctx.fillStyle = waveColors[2];
            ctx.beginPath();
            ctx.moveTo(0, h * 0.8);
            for (var x3 = 0; x3 <= w; x3 += 10) {
                ctx.lineTo(x3, h * 0.8 + Math.sin(x3 / w * Math.PI * 2 + 2) * 30);
            }
            ctx.lineTo(w, h);
            ctx.lineTo(0, h);
            ctx.closePath();
            ctx.fill();
        }
    }
}
