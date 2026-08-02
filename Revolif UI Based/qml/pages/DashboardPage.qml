import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../"

Rectangle {
    id: dashboard
    color: Theme.bg

    // ---- Shared dashboard data, pulled straight from the database via
    // revolif.getDashboardData(). Every card/chart below reads from this
    // single snapshot so one refresh keeps the whole page in sync. ----
    property var dashData: ({})

    // The Tasks/Goals/Expenses summary cards below all stack the same
    // shape of content (icon row, big number, label, filler, small caption)
    // inside a fixed-height Rectangle. A hardcoded height doesn't reliably
    // fit that stack -- when the caption line is present it needs more
    // room than 128px gave it, which pushed the caption below the card's
    // visible edge. Sizing from the tallest of the three actual columns
    // (plus the 20px top/bottom margins each card uses) fixes that and
    // keeps all three cards the same height.
    readonly property real summaryCardHeight:
        Math.max(taskStatsCol.implicitHeight, goalStatsCol.implicitHeight, expenseStatsCol.implicitHeight) + 40

    function refreshDashboard() {
        dashData = revolif.getDashboardData();
    }

    Component.onCompleted: refreshDashboard()

    Connections {
        target: revolif
        function onStatsChanged() { dashboard.refreshDashboard() }
    }

    // ---- Formatting helpers ----
    function formatCurrency(amount) {
        var n = Number(amount) || 0;
        var fixed = n.toFixed(2);
        var parts = fixed.split(".");
        var intPart = parts[0];
        var withCommas = intPart.replace(/\B(?=(\d{3})+(?!\d))/g, ",");
        return revolif.currencySymbol + withCommas + "." + parts[1];
    }

    function parseRecordDate(dateStr) {
        // "DD/MM/YYYY" -> JS Date
        var parts = (dateStr || "").split("/");
        if (parts.length !== 3) return null;
        return new Date(parseInt(parts[2]), parseInt(parts[1]) - 1, parseInt(parts[0]));
    }

    function relativeDay(dateStr) {
        var d = parseRecordDate(dateStr);
        if (!d) return dateStr;
        var today = new Date();
        var a = new Date(today.getFullYear(), today.getMonth(), today.getDate());
        var b = new Date(d.getFullYear(), d.getMonth(), d.getDate());
        var diffDays = Math.round((a - b) / 86400000);
        if (diffDays === 0) return "Today";
        if (diffDays === 1) return "Yesterday";
        return dateStr;
    }

    function activityIcon(type) {
        switch (type) {
        case "task_added": return "＋";
        case "task_completed": return "✓";
        case "goal_added": return "◉";
        case "goal_completed": return "★";
        case "expense_added": return revolif.currencySymbol;
        default: return "•";
        }
    }

    function activityColor(type) {
        switch (type) {
        case "task_added": return Theme.accent;
        case "task_completed": return Theme.success;
        case "goal_added": return Theme.primary;
        case "goal_completed": return Theme.warning;
        case "expense_added": return Theme.textSecondary;
        default: return Theme.textMuted;
        }
    }

    function activityLabel(type, title) {
        switch (type) {
        case "task_added": return "Added task \u201C" + title + "\u201D";
        case "task_completed": return "Completed task \u201C" + title + "\u201D";
        case "goal_added": return "Set goal \u201C" + title + "\u201D";
        case "goal_completed": return "Achieved goal \u201C" + title + "\u201D";
        case "expense_added": return "Logged expense \u201C" + title + "\u201D";
        default: return title;
        }
    }

    function levelColor(level) {
        switch (level) {
        case 0: return Theme.hoverBg;
        case 1: return Qt.rgba(0.243, 0.733, 0.620, 0.35); // accent, faint
        case 2: return Qt.rgba(0.243, 0.733, 0.620, 0.7);  // accent
        default: return Theme.primary;
        }
    }

    function translucent(c, alpha) {
        return Qt.rgba(c.r, c.g, c.b, alpha);
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        clip: true
        ScrollBar.vertical: AppScrollBar {}

        ColumnLayout {
            x: 24
            y: 20
            width: dashboard.width - 48
            spacing: 24

            // ---- Light greeting header (text only, no card — the page
            // title is already shown in the TopBar) ----
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    text: "Welcome back, " + revolif.currentUserName
                    font.pixelSize: Theme.fontSizeLg
                    font.bold: true
                    color: Theme.textPrimary
                }
                Label {
                    text: Qt.formatDate(new Date(), "dddd, MMMM d")
                    font.pixelSize: Theme.fontSizeSm
                    color: Theme.textSecondary
                }
            }

            // ============================================================
            //  "TODAY'S JOURNEY" HERO BANNER
            // ============================================================
            WaveBackground {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                radius: Theme.radiusXl
                clip: true

                ColumnLayout {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 36
                    anchors.topMargin: 36
                    anchors.bottomMargin: 36
                    width: 420
                    spacing: 10

                    Item { Layout.fillHeight: true }

                    Label {
                        text: "JOURNEY · DIRECTION · GROWTH"
                        font.pixelSize: Theme.fontSizeXs
                        font.bold: true
                        font.letterSpacing: 2
                        color: Theme.textPrimary
                    }

                    Label {
                        text: "Today's journey"
                        font.pixelSize: Theme.fontSizeXl
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Label {
                        text: "\u201CSmall steps today, greater tomorrow.\u201D"
                        font.pixelSize: Theme.fontSizeSm
                        font.italic: true
                        color: Theme.textSecondary
                    }

                    AnimatedButton {
                        Layout.topMargin: 8
                        text: "Let's focus \u2192"
                        primary: true
                        onClicked: { sidebar.currentPage = "tasks"; sidebar.navigate("tasks"); }
                    }

                    Item { Layout.fillHeight: true }
                }

                // Life Score ring — positioned independently with anchors so its
                // horizontal placement can be tuned directly via rightMargin/x,
                // instead of fighting a fillWidth sibling in a RowLayout.
                ColumnLayout {
                    id: lifeScoreBlock
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 24
                    spacing: 8

                    ProgressRing {
                        id: lifeScoreRing
                        Layout.alignment: Qt.AlignHCenter
                        width: 130
                        height: 130
                        strokeWidth: 10
                        trackColor: Qt.rgba(1, 1, 1, 0.4)
                        progressColor: Theme.textPrimary
                        progress: revolif.lifeScore / 100

                        Behavior on progress {
                            NumberAnimation { duration: 700; easing.type: Easing.OutCubic }
                        }
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Life Score"
                        font.pixelSize: Theme.fontSizeXs
                        font.bold: true
                        font.letterSpacing: 1
                        color: Theme.textPrimary
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: revolif.lifeScoreLabel
                        font.pixelSize: Theme.fontSizeXs
                        color: Theme.textSecondary
                    }
                }
            }

            // ============================================================
            //  TODAY'S SUMMARY CARDS — Tasks · Goals · Expenses
            // ============================================================
            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                // -- Tasks --
                Rectangle {
                    color: Theme.cardBg
                    radius: Theme.radiusLg
                    border.color: Theme.border
                    border.width: 1
                    
                    Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }
                    Layout.fillWidth: true
                    Layout.preferredHeight: dashboard.summaryCardHeight

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { sidebar.currentPage = "tasks"; sidebar.navigate("tasks"); }
                    }

                    ColumnLayout {
                        id: taskStatsCol
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle {
                                Layout.preferredWidth: 34
                                Layout.preferredHeight: 34
                                radius: 12
                                color: Qt.rgba(0.243, 0.733, 0.620, 0.15)
                                Label { anchors.centerIn: parent; text: "☑"; color: Theme.accent; font.pixelSize: 16 }
                            }
                            Item { Layout.fillWidth: true }
                        }

                        Label {
                            text: (dashData.pendingTasks !== undefined ? dashData.pendingTasks : 0)
                            font.pixelSize: 28
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Label {
                            text: "Pending Tasks"
                            font.pixelSize: Theme.fontSizeSm
                            color: Theme.textSecondary
                        }
                        Item { Layout.fillHeight: true }
                        Label {
                            text: (dashData.completedTasks || 0) + " completed" +
                                  ((dashData.overdueTasks || 0) > 0 ? "  ·  " + dashData.overdueTasks + " overdue" : "")
                            font.pixelSize: Theme.fontSizeXs
                            color: (dashData.overdueTasks || 0) > 0 ? Theme.danger : Theme.textMuted
                        }
                    }
                }

                // -- Goals --
                Rectangle {
                    color: Theme.cardBg
                    radius: Theme.radiusLg
                    border.color: Theme.border
                    border.width: 1
                    
                    Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }
                    Layout.fillWidth: true
                    Layout.preferredHeight: dashboard.summaryCardHeight

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { sidebar.currentPage = "goals"; sidebar.navigate("goals"); }
                    }

                    ColumnLayout {
                        id: goalStatsCol
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle {
                                Layout.preferredWidth: 34
                                Layout.preferredHeight: 34
                                radius: 12
                                color: Qt.rgba(0.0, 0.404, 0.310, 0.12)
                                Label { anchors.centerIn: parent; text: "◉"; color: Theme.primary; font.pixelSize: 16 }
                            }
                            Item { Layout.fillWidth: true }
                        }

                        Label {
                            text: (dashData.pendingGoals !== undefined ? dashData.pendingGoals : 0)
                            font.pixelSize: 28
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Label {
                            text: "Active Goals"
                            font.pixelSize: Theme.fontSizeSm
                            color: Theme.textSecondary
                        }
                        Item { Layout.fillHeight: true }
                        Label {
                            text: (dashData.completedGoals || 0) + " completed"
                            font.pixelSize: Theme.fontSizeXs
                            color: Theme.textMuted
                        }
                    }
                }

                // -- Expenses --
                Rectangle {
                    color: Theme.cardBg
                    radius: Theme.radiusLg
                    border.color: Theme.border
                    border.width: 1
                    
                    Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }
                    Layout.fillWidth: true
                    Layout.preferredHeight: dashboard.summaryCardHeight

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { sidebar.currentPage = "expenses"; sidebar.navigate("expenses"); }
                    }

                    ColumnLayout {
                        id: expenseStatsCol
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle {
                                Layout.preferredWidth: 34
                                Layout.preferredHeight: 34
                                radius: 12
                                color: Qt.rgba(0.878, 0.627, 0.188, 0.15)
                                Label { anchors.centerIn: parent; text: revolif.currencySymbol; color: Theme.warning; font.bold: true; font.pixelSize: 16 }
                            }
                            Item { Layout.fillWidth: true }
                        }

                        Label {
                            text: dashboard.formatCurrency(dashData.expensesThisMonth || 0)
                            font.pixelSize: 24
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Label {
                            text: "Spent This Month"
                            font.pixelSize: Theme.fontSizeSm
                            color: Theme.textSecondary
                        }
                        Item { Layout.fillHeight: true }
                        Label {
                            text: dashData.topCategory ? ("Top: " + dashData.topCategory) : "No expenses yet"
                            font.pixelSize: Theme.fontSizeXs
                            color: Theme.textMuted
                        }
                    }
                }
            }

            // ============================================================
            //  MONTHLY EXPENSE CHART  +  CONTRIBUTION / ACTIVITY CALENDAR
            // ============================================================
            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                // -- Small monthly expense bar chart --
                Rectangle {
                    color: Theme.cardBg
                    radius: Theme.radiusLg
                    border.color: Theme.border
                    border.width: 1
                    
                    Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }
                    clip: true
                    Layout.fillWidth: true
                    Layout.preferredHeight: 300

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 12

                        Label {
                            text: "Spending This Month"
                            font.bold: true
                            font.pixelSize: Theme.fontSizeBase
                            color: Theme.textPrimary
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 20

                            Repeater {
                                model: dashData.monthlyExpenseChart || []
                                delegate: ColumnLayout {
                                    Layout.preferredWidth: 36
                                    Layout.fillHeight: true
                                    spacing: 6

                                    property real maxAmount: {
                                        var m = 1;
                                        var chart = dashData.monthlyExpenseChart || [];
                                        for (var i = 0; i < chart.length; i++)
                                            if (chart[i].amount > m) m = chart[i].amount;
                                        return m;
                                    }

                                    Item { Layout.fillHeight: true }

                                    Rectangle {
                                        Layout.alignment: Qt.AlignHCenter
                                        Layout.preferredWidth: 22
                                        Layout.preferredHeight: Math.max(4, 120 * (modelData.amount / maxAmount))
                                        radius: 6
                                        color: modelData.amount > 0 ? Theme.accent : Theme.hoverBg

                                        ToolTip.visible: barHover.containsMouse
                                        ToolTip.text: dashboard.formatCurrency(modelData.amount)

                                        MouseArea {
                                            id: barHover
                                            anchors.fill: parent
                                            hoverEnabled: true
                                        }
                                    }

                                    Label {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: modelData.label
                                        font.pixelSize: Theme.fontSizeXs
                                        color: Theme.textSecondary
                                    }
                                }
                            }
                        }
                    }
                }

                // -- Real calendar --
                MiniCalendar {
                    Layout.preferredWidth: 260
                    Layout.preferredHeight: 300
                }

                // -- GitHub-style contribution / activity calendar --
                Rectangle {
                    color: Theme.cardBg
                    radius: Theme.radiusLg
                    border.color: Theme.border
                    border.width: 1
                    
                    Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }
                    clip: true
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 300

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 10

                        Label {
                            text: "Consistency This Month"
                            font.bold: true
                            font.pixelSize: Theme.fontSizeBase
                            color: Theme.textPrimary
                        }

                        GridLayout {
                            columns: 7
                            rowSpacing: 4
                            columnSpacing: 4
                            Layout.fillWidth: true

                            Repeater {
                                model: ["S", "M", "T", "W", "T", "F", "S"]
                                delegate: Label {
                                    text: modelData
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                    horizontalAlignment: Text.AlignHCenter
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 22
                                }
                            }

                            Repeater {
                                model: dashData.calendarFirstWeekday || 0
                                delegate: Item { Layout.fillWidth: true; Layout.minimumWidth: 22; Layout.preferredHeight: 22 }
                            }

                            Repeater {
                                model: dashData.contributionCalendar || []
                                delegate: Rectangle {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 22
                                    Layout.preferredHeight: 22
                                    radius: 5
                                    color: dashboard.levelColor(modelData.level)
                                    border.width: modelData.isToday ? 2 : 0
                                    border.color: Theme.primary

                                    ToolTip.visible: cellHover.containsMouse
                                    ToolTip.text: modelData.count + (modelData.count === 1 ? " activity" : " activities")

                                    MouseArea {
                                        id: cellHover
                                        anchors.fill: parent
                                        hoverEnabled: true
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            Layout.alignment: Qt.AlignRight
                            spacing: 4
                            Label { text: "Less"; font.pixelSize: 10; color: Theme.textMuted }
                            Repeater {
                                model: 4
                                delegate: Rectangle {
                                    width: 12; height: 12; radius: 3
                                    color: dashboard.levelColor(index)
                                }
                            }
                            Label { text: "More"; font.pixelSize: 10; color: Theme.textMuted }
                        }
                    }
                }
            }

            // ============================================================
            //  RECENT ACTIVITY TIMELINE  +  UPCOMING TASKS/EVENTS
            // ============================================================
            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                // -- Recent activity --
                Rectangle {
                    color: Theme.cardBg
                    radius: Theme.radiusLg
                    border.color: Theme.border
                    border.width: 1
                    
                    Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }
                    Layout.fillWidth: true
                    Layout.preferredHeight: 280

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 10

                        Label {
                            text: "Recent Activity"
                            font.bold: true
                            font.pixelSize: Theme.fontSizeBase
                            color: Theme.textPrimary
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 4
                            model: dashData.recentActivity || []

                            ScrollBar.vertical: AppScrollBar {}

                            delegate: RowLayout {
                                width: ListView.view.width
                                height: 44
                                spacing: 10

                                Rectangle {
                                    Layout.preferredWidth: 28
                                    Layout.preferredHeight: 28
                                    radius: 10
                                    color: dashboard.translucent(dashboard.activityColor(modelData.type), 0.15)
                                    Label {
                                        anchors.centerIn: parent
                                        text: dashboard.activityIcon(modelData.type)
                                        color: dashboard.activityColor(modelData.type)
                                        font.bold: true
                                        font.pixelSize: 12
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    Label {
                                        text: dashboard.activityLabel(modelData.type, modelData.title)
                                        font.pixelSize: Theme.fontSizeSm
                                        color: Theme.textPrimary
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        text: dashboard.relativeDay(modelData.date) + " · " + modelData.time
                                        font.pixelSize: Theme.fontSizeXs
                                        color: Theme.textMuted
                                    }
                                }
                            }

                            Label {
                                anchors.centerIn: parent
                                visible: (dashData.recentActivity || []).length === 0
                                text: "No activity yet — add a task, goal, or expense to get started."
                                font.pixelSize: Theme.fontSizeSm
                                color: Theme.textMuted
                                wrapMode: Text.WordWrap
                                width: parent.width - 20
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }
                }

                // -- Upcoming tasks/events --
                Rectangle {
                    color: Theme.cardBg
                    radius: Theme.radiusLg
                    border.color: Theme.border
                    border.width: 1
                    
                    Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }
                    Layout.fillWidth: true
                    Layout.preferredHeight: 280

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 10

                        Label {
                            text: "Upcoming"
                            font.bold: true
                            font.pixelSize: Theme.fontSizeBase
                            color: Theme.textPrimary
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 8
                            model: dashData.upcomingTasks || []

                            ScrollBar.vertical: AppScrollBar {}

                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 44
                                radius: Theme.radiusBase
                                color: Theme.hoverBg

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    Rectangle {
                                        Layout.preferredWidth: 18
                                        Layout.preferredHeight: 18
                                        radius: 6
                                        border.color: Theme.border
                                        border.width: 2
                                        color: "transparent"

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: revolif.completeTask(modelData.id)
                                        }
                                    }

                                    Label {
                                        text: modelData.title
                                        font.pixelSize: Theme.fontSizeBase
                                        color: Theme.textPrimary
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        text: modelData.deadline + " · " + modelData.time
                                        font.pixelSize: Theme.fontSizeSm
                                        color: Theme.textSecondary
                                    }
                                }
                            }

                            Label {
                                anchors.centerIn: parent
                                visible: (dashData.upcomingTasks || []).length === 0
                                text: "No upcoming tasks. You're all caught up!"
                                font.pixelSize: Theme.fontSizeSm
                                color: Theme.textMuted
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 8 }
        }
    }
}
