pragma Singleton
import QtQuick

// Color palette for the Admin Control Center only. The rest of the app
// keeps using Theme.qml -- this singleton exists so the admin dashboard's
// light-canvas / dark-card look can evolve independently.
QtObject {
    // Base surfaces
    property color pageBg: "#E2ECEB"      // main content canvas (light)
    property color surface: "#161E2E"     // sidebar + all cards/containers (dark)

    // Brand / emerald -- use ONLY for primary buttons, active nav, status
    // badges, focus states, links and small highlights (per design spec).
    property color primary: "#10B981"
    property color primaryHover: "#059669"
    property color primaryLight: "#6EE7B7"

    // Text on dark surfaces (sidebar, cards)
    property color textPrimaryDark: "#F9FAFB"
    property color textSecondaryDark: "#9CA3AF"

    // Text directly on the light page background
    property color textPrimaryLight: "#111827"
    property color textSecondaryLight: "#4B5563"

    // Borders / dividers (on dark surfaces)
    property color border: "#2A3345"

    // Semantic status colors
    property color success: "#10B981"
    property color danger: "#EF4444"
    property color warning: "#F59E0B"
}
