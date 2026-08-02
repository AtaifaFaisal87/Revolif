import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtMultimedia
import "components"

ApplicationWindow {
    id: root
    visible: true
    width: 1366
    height: 768
    minimumWidth: 1024
    minimumHeight: 600
    title: "REVOLIF - Life, Beautifully Aligned"
    color: Theme.bg

    // Anything that isn't individually restyled (default TextField/ComboBox/
    // Dialog chrome in dialogs, etc.) falls back to this palette, so it
    // still follows light/dark mode instead of staying stuck on the
    // Qt Quick Controls default light colors.
    palette.window: Theme.bg
    palette.windowText: Theme.textPrimary
    palette.base: Theme.cardBg
    palette.text: Theme.textPrimary
    palette.button: Theme.cardBg
    palette.buttonText: Theme.textPrimary
    palette.highlight: Theme.primary
    palette.highlightedText: "#FFFFFF"
    palette.placeholderText: Theme.textMuted
    palette.mid: Theme.border
    palette.dark: Theme.border

    Component.onCompleted: Theme.isDark = revolif.darkMode

    // ---- Login sound cue. Replaces the earlier TTS "Welcome to Revolif"
    // voice greeting -- on this MinGW-built Qt kit, TTS was stuck on the
    // dated "sapi" engine (the more natural "winrt" voices need an
    // MSVC-built Qt), so no amount of tuning made it sound human. A short,
    // pleasant chime sidesteps that problem entirely: it's instant
    // (SoundEffect is a lightweight, low-latency player, no engine/process
    // startup at all) and there's no "robotic" quality to worry about. ----
    SoundEffect {
        id: loginChime
        source: "qrc:/assets/login_chime.wav"
        volume: 0.7
    }

    // Set when a chime is owed, cleared once it's actually played. The
    // chime should be heard once the dashboard is on screen, not before --
    // so we don't play it the moment the login signal arrives. Instead we
    // wait for stackView to finish swapping to the new shell (see
    // onBusyChanged below) and fire it then.
    property bool pendingChime: false

    function playLoginChimeNow() {
        // QSoundEffect silently no-ops play() if it's called before the
        // file has finished its first async load (status != Ready) -- it
        // doesn't queue the request. That's rare in practice, but possible
        // right at app launch, so fall back to playing as soon as it
        // becomes ready instead of dropping the chime.
        if (loginChime.status === SoundEffect.Ready) {
            loginChime.play()
        } else {
            loginChime.statusChanged.connect(function onReady() {
                if (loginChime.status === SoundEffect.Ready) {
                    loginChime.statusChanged.disconnect(onReady)
                    loginChime.play()
                }
            })
        }
    }

    Connections {
        target: revolif
        function onLoginChimeRequested() { pendingChime = true }
    }

    Connections {
        target: revolif
        function onDarkModeChanged() { Theme.isDark = revolif.darkMode }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: revolif.isAdmin ? adminShell : (revolif.isLoggedIn ? mainShell : authScreen)
        replaceEnter: Transition {
            PropertyAnimation { property: "opacity"; from: 0; to: 1; duration: 250 }
        }
        replaceExit: Transition {
            PropertyAnimation { property: "opacity"; from: 1; to: 0; duration: 250 }
        }
        onBusyChanged: {
            // busy goes false once the replace() transition has fully
            // settled, i.e. the dashboard is actually visible -- that's
            // the right moment for the chime, not when the request first
            // came in.
            if (!busy && pendingChime) {
                pendingChime = false
                playLoginChimeNow()
            }
        }
    }

    Component { id: authScreen; AuthScreen {} }
    Component { id: mainShell; MainShell {} }
    Component { id: adminShell; AdminShell {} }

    Connections {
        target: revolif
        function onIsLoggedInChanged() {
            if (revolif.isAdmin) {
                stackView.replace(adminShell)
            } else if (revolif.isLoggedIn) {
                stackView.replace(mainShell)
            } else {
                stackView.replace(authScreen)
            }
        }
    }

    Toast {
        id: globalToast
        anchors.top: parent.top
        anchors.topMargin: 24
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Connections {
        target: revolif
        function onErrorOccurred(message) { globalToast.show(message, "error") }
        function onSuccessMessage(message) { globalToast.show(message, "success") }
    }
}
