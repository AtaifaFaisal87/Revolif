import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"

// Full-width "hero" banner shown at the top of the Admin Control Center's
// main content area, below AdminTopBar. Functional: the right-hand label
// is a live UTC clock that ticks every second.
Rectangle {
    id: banner
    radius: 16
    clip: true

    gradient: Gradient {
        orientation: Gradient.Vertical
        GradientStop { position: 0.0; color: "#0A2A22" }
        GradientStop { position: 0.55; color: "#123D33" }
        GradientStop { position: 1.0; color: "#1B5445" }
    }

    // ---- Live clock -------------------------------------------------
    property date now: new Date()

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: banner.now = new Date()
    }

    function ordinal(n) {
        if (n >= 11 && n <= 13) return n + "th"
        switch (n % 10) {
            case 1: return n + "st"
            case 2: return n + "nd"
            case 3: return n + "rd"
            default: return n + "th"
        }
    }

    function formatted() {
        var days = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"]
        var months = ["January", "February", "March", "April", "May", "June", "July",
                       "August", "September", "October", "November", "December"]
        // Shift from UTC to UTC+5 for display purposes
        var shifted = new Date(banner.now.getTime() + 5 * 60 * 60 * 1000)
        var h = shifted.getUTCHours()
        var m = shifted.getUTCMinutes()
        var ampm = h >= 12 ? "PM" : "AM"
        var h12 = h % 12
        if (h12 === 0) h12 = 12
        var mm = m < 10 ? "0" + m : "" + m
        return h12 + ":" + mm + " " + ampm + " UTC+5 | " + days[shifted.getUTCDay()] + ", " +
               months[shifted.getUTCMonth()] + " " + ordinal(shifted.getUTCDate()) + ", " +
               shifted.getUTCFullYear()
    }

    // ---- Decorative waves (bottom edge) ------------------------------
    Canvas {
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            var w = width, h = height
            ctx.clearRect(0, 0, w, h)

            ctx.fillStyle = "rgba(255,255,255,0.05)"
            ctx.beginPath()
            ctx.moveTo(0, h * 0.72)
            for (var x = 0; x <= w; x += 12) ctx.lineTo(x, h * 0.72 + Math.sin(x / w * Math.PI * 2) * 10)
            ctx.lineTo(w, h); ctx.lineTo(0, h); ctx.closePath(); ctx.fill()

            ctx.fillStyle = "rgba(255,255,255,0.08)"
            ctx.beginPath()
            ctx.moveTo(0, h * 0.85)
            for (var x2 = 0; x2 <= w; x2 += 12) ctx.lineTo(x2, h * 0.85 + Math.sin(x2 / w * Math.PI * 2 + 1.4) * 8)
            ctx.lineTo(w, h); ctx.lineTo(0, h); ctx.closePath(); ctx.fill()
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 2
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: Qt.rgba(0.243, 0.898, 0.635, 0) }
            GradientStop { position: 0.5; color: Qt.rgba(0.243, 0.898, 0.635, 0.55) }
            GradientStop { position: 1.0; color: Qt.rgba(0.243, 0.898, 0.635, 0) }
        }
    }

    // ---- Content ------------------------------------------------------
    RowLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        ColumnLayout {
            spacing: 4
            Label {
                text: "Revolif Control Center"
                font.pixelSize: 24
                font.weight: Font.DemiBold
                color: "#A7F3D0"
            }
            Label {
                text: "System Management & Administration"
                font.pixelSize: 14
                color: "#99F6E4"
            }
        }

        Item { Layout.fillWidth: true }

        Label {
            Layout.alignment: Qt.AlignTop
            text: banner.formatted()
            font.pixelSize: 14
            color: "#D1D5DB"
        }
    }
}
