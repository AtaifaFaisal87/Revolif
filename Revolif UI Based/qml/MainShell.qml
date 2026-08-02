import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"
import "pages"

Rectangle {
    id: mainShell
    color: Theme.bg

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            id: sidebar
            Layout.preferredWidth: Theme.sidebarWidth
            Layout.fillHeight: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            TopBar {
                id: topBar
                Layout.fillWidth: true
                Layout.preferredHeight: 70
            }

            StackView {
                id: contentStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                initialItem: dashboardPage
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

    Component { id: dashboardPage; DashboardPage {} }
    Component { id: focusPage; FocusPage {} }
    Component { id: tasksPage; TasksPage {} }
    Component { id: goalsPage; GoalsPage {} }
    Component { id: expensesPage; ExpensesPage {} }
    Component { id: profilePage; ProfilePage {} }
    Component { id: settingsPage; SettingsPage {} }
    Component { id: achievementsPage; AchievementsPage {} }

    Connections {
        target: sidebar
        function onNavigate(page) {
            switch(page) {
            case "dashboard": contentStack.replace(dashboardPage); revolif.pageTitle = "Dashboard"; break;
            case "focus": contentStack.replace(focusPage); revolif.pageTitle = "Focus"; break;
            case "tasks": contentStack.replace(tasksPage); revolif.pageTitle = "Tasks"; break;
            case "goals": contentStack.replace(goalsPage); revolif.pageTitle = "Goals"; break;
            case "expenses": contentStack.replace(expensesPage); revolif.pageTitle = "Expenses"; break;
            case "profile": contentStack.replace(profilePage); revolif.pageTitle = "Profile"; break;
            case "settings": contentStack.replace(settingsPage); revolif.pageTitle = "Settings"; break;
            case "achievements": contentStack.replace(achievementsPage); revolif.pageTitle = "Achievements"; break;
            }
        }
    }
}
