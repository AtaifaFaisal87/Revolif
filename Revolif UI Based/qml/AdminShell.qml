import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"
import "pages"

// The Admin experience is intentionally a completely separate shell from
// MainShell: a dark "control center" look, a three-item sidebar, and a
// single dashboard that hosts nearly every administrative action inline.
Rectangle {
    id: adminShell

    // Light "control center" canvas: sidebar + cards stay dark (AdminTheme.surface),
    // while the page itself is a soft mint-gray. Emerald identity is preserved
    // through accents (buttons, the active nav item, focus states, badges).
    color: AdminTheme.pageBg

    Component.onCompleted: revolif.pageTitle = "Dashboard"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        AdminSidebar {
            id: adminSidebar
            Layout.preferredWidth: Theme.sidebarWidth
            Layout.fillHeight: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            AdminTopBar {
                id: topBar
                Layout.fillWidth: true
                Layout.preferredHeight: 76
            }

            AdminBanner {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.bottomMargin: 16
            }

            StackView {
                id: contentStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                initialItem: adminDashboardPage
                clip: true

                replaceEnter: Transition {
                    PropertyAnimation { property: "opacity"; from: 0; to: 1; duration: 200 }
                    PropertyAnimation { property: "x"; from: 20; to: 0; duration: 200; easing.type: Easing.OutCubic }
                }
                replaceExit: Transition {
                    PropertyAnimation { property: "opacity"; from: 1; to: 0; duration: 150 }
                }
            }
        }
    }

    Component { id: adminDashboardPage; AdminDashboardPage {} }
    Component { id: adminProfilePage; AdminProfilePage {} }
    Component { id: adminSettingsPage; AdminSettingsPage {} }

    Connections {
        target: adminSidebar
        function onNavigate(page) {
            switch (page) {
            case "dashboard": contentStack.replace(adminDashboardPage); revolif.pageTitle = "Dashboard"; break;
            case "profile": contentStack.replace(adminProfilePage); revolif.pageTitle = "Profile"; break;
            case "settings": contentStack.replace(adminSettingsPage); revolif.pageTitle = "Settings"; break;
            }
        }
    }
}
