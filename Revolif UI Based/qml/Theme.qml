pragma Singleton
import QtQuick

QtObject {
    id: theme

    // Set once at startup from the saved preference, and whenever the user
    // flips the toggle in Settings -- see main.qml (revolif.darkMode binding)
    // and SettingsPage.qml. Every color below re-evaluates automatically
    // since they're bindings on this flag.
    property bool isDark: false

    property color primaryDark: "#0A3C30"
    property color primary: isDark ? "#3EBB9E" : "#00674F"
    property color accent: "#3EBB9E"
    property color accentLight: isDark ? "#6FE0C4" : "#73E6CB"
    property color accentHover: isDark ? "#52D1B2" : "#2DA88C"

    property color bg: isDark ? "#12201C" : "#F5F3EE"
    property color sidebarBg: isDark ? "#0E1A17" : "#F7FAF9"
    property color cardBg: isDark ? "#182B26" : "#FFFFFF"
    property color hoverBg: isDark ? "#213933" : "#EEF3F1"
    property color selectedBg: isDark ? "#25443C" : "#E3F0EC"

    property color textPrimary: isDark ? "#EAF2EF" : "#1A2E2A"
    property color textSecondary: isDark ? "#9FB3AE" : "#6B7B77"
    property color textMuted: isDark ? "#6C837D" : "#A9B6B2"
    property color textOnPrimary: "#FFFFFF"

    property color success: "#3EBB9E"
    property color warning: "#E0A030"
    property color danger: isDark ? "#E8746F" : "#D9534F"
    property color info: "#3EBB9E"

    property color border: isDark ? "#28413A" : "#E3EBE8"
    property color borderLight: isDark ? "#1F332E" : "#F0F4F3"

    // ---- Input field tokens ----
    // Filled, Facebook-style fields: a soft tinted fill instead of a bare
    // outline, generous height/padding so they don't read as "thin", and a
    // clear focus state. Used by AppTextField / AppComboBox / AppSpinBox.
    property color inputBg: isDark ? "#1E3831" : "#F0F2F2"
    property color inputBgFocus: isDark ? "#233F37" : "#FFFFFF"
    property color inputBorder: isDark ? "#33564C" : "#DCE4E1"
    property color inputBorderFocus: accent
    property color inputPlaceholder: isDark ? "#7C948E" : "#8A9A96"
    property int inputHeight: 52
    property int inputRadius: 14

    // ---- Elevation / shadow tokens ----
    // No box-shadow in QML, so cards fake depth with a soft, low-opacity
    // rectangle offset behind them (see shadowColor / shadowSoft below).
    property color shadowColor: isDark ? Qt.rgba(0, 0, 0, 0.35) : Qt.rgba(0.02, 0.16, 0.12, 0.10)
    property color shadowSoft: isDark ? Qt.rgba(0, 0, 0, 0.22) : Qt.rgba(0.02, 0.16, 0.12, 0.05)

    property string fontFamily: "Segoe UI, Helvetica, Arial, sans-serif"
    property int fontSizeXs: 11
    property int fontSizeSm: 13
    property int fontSizeBase: 15
    property int fontSizeLg: 18
    property int fontSizeXl: 25
    property int fontSize2xl: 34

    property int radiusSm: 12
    property int radiusBase: 16
    property int radiusLg: 22
    property int radiusXl: 32
    property int radiusFull: 999

    property int spacingXs: 6
    property int spacingSm: 12
    property int spacingBase: 16
    property int spacingLg: 24
    property int spacingXl: 32

    property int sidebarWidth: 260
}
