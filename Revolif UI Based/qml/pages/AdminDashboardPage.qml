import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../dialogs"
import "../"

Rectangle {
    id: adminDashboardPage
    color: "transparent"

    // ---------------------------------------------------------------
    // Data
    // ---------------------------------------------------------------
    property var stats: ({})
    property var rawUsers: []
    property var deletedUsers: []
    property var achievements: []
    property var activity: []
    property var deletionStats: []

    property string searchText: ""
    property string activeFilter: "all"

    function refreshAll() {
        stats = revolif.getSystemStatistics()
        rawUsers = revolif.getAllUsers()
        deletedUsers = revolif.getPermanentlyDeletedUsers()
        achievements = revolif.getAchievements()
        activity = revolif.getRecentAdminActivity()
        deletionStats = revolif.getDeletionReasonStats()
    }

    function totalDeletions() {
        var t = 0
        for (var i = 0; i < deletionStats.length; i++) t += deletionStats[i].count
        return t
    }

    // One fixed color per reason, same idea as the console chart's
    // REASON_COLORS -- cycles if there are ever more than 6 reasons.
    function reasonColor(index) {
        var colors = ["#F87171", "#10B981", "#FBBF24", "#60A5FA", "#C084FC", "#22D3EE"]
        return colors[index % colors.length]
    }

    Component.onCompleted: refreshAll()

    Connections {
        target: revolif
        function onStatsChanged() { refreshAll() }
    }

    function statusOf(u) {
        if (u.deleted) return "deleted"
        return u.active ? "active" : "suspended"
    }

    function statusLabel(status) {
        if (status === "active") return "Active"
        if (status === "suspended") return "Suspended"
        return "Deleted"
    }

    function statusColor(status) {
        if (status === "active") return "#10B981"
        if (status === "suspended") return "#F59E0B"
        return "#EF4444"
    }

    function combinedUsers() {
        var list = []
        for (var i = 0; i < rawUsers.length; i++) {
            var u = rawUsers[i]
            list.push({
                uid: u.uid, name: u.name, username: u.username, email: u.email,
                regDate: u.regDate, active: u.active, deleted: false
            })
        }
        for (var j = 0; j < deletedUsers.length; j++) {
            var d = deletedUsers[j]
            list.push({
                uid: d.uid, name: d.name && d.name.length > 0 ? d.name : d.username,
                username: d.username, email: "", regDate: "", active: false, deleted: true,
                deletionDate: d.date, reason: d.reason
            })
        }
        return list
    }

    function filteredUsers() {
        var all = combinedUsers()
        var q = searchText.trim().toLowerCase()
        var result = []
        for (var i = 0; i < all.length; i++) {
            var u = all[i]
            var status = statusOf(u)
            if (activeFilter !== "all" && status !== activeFilter) continue
            if (q.length > 0) {
                var hay = (u.name + " " + u.username + " " + u.email + " " + u.uid).toLowerCase()
                if (hay.indexOf(q) === -1) continue
            }
            result.push(u)
        }
        return result
    }

    // ---------------------------------------------------------------
    // Layout
    // ---------------------------------------------------------------
    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical: AppScrollBar {}

        ColumnLayout {
            x: 28
            y: 12
            width: adminDashboardPage.width - 56
            spacing: 32

            // ============================================================
            // SYSTEM OVERVIEW
            // ============================================================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 14

                Label {
                    text: "SYSTEM OVERVIEW"
                    font.pixelSize: 13
                    font.bold: true
                    font.letterSpacing: 2
                    color: AdminTheme.primaryHover
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    Repeater {
                        model: [
                            { label: "Total Registered Users", value: adminDashboardPage.stats.totalUsers || 0, icon: "◈" },
                            { label: "Active Users", value: adminDashboardPage.stats.activeUsers || 0, icon: "●" },
                            { label: "Suspended Users", value: adminDashboardPage.stats.suspendedUsers || 0, icon: "◐" },
                            { label: "Permanently Deleted", value: adminDashboardPage.stats.permanentlyDeletedUsers || 0, icon: "✕" },
                            { label: "Total Achievements", value: adminDashboardPage.stats.totalAchievements || 0, icon: "★" }
                        ]

                        delegate: GlassPanel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 140
                            clip: true

                            ColumnLayout {
                                id: statCardContent
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 18
                                spacing: 8

                                Label {
                                    text: modelData.icon
                                    font.pixelSize: 18
                                    color: Theme.accentLight
                                }
                                Label {
                                    text: modelData.value
                                    font.pixelSize: 30
                                    font.bold: true
                                    color: "white"
                                }
                                Label {
                                    text: modelData.label
                                    font.pixelSize: 12
                                    color: Qt.rgba(1, 1, 1, 0.6)
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }

            // ============================================================
            // USER MANAGEMENT
            // ============================================================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 14

                Label {
                    text: "USER MANAGEMENT"
                    font.pixelSize: 13
                    font.bold: true
                    font.letterSpacing: 2
                    color: AdminTheme.primaryHover
                }

                GlassPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: userMgmtContent.implicitHeight + 40

                    ColumnLayout {
                        id: userMgmtContent
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 16

                        // ---- Search bar ----
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 52
                            radius: Theme.radiusBase
                            color: "#1F2937"
                            border.color: searchField.activeFocus ? "#10B981" : "#374151"
                            border.width: searchField.activeFocus ? 2 : 1
                            Behavior on border.color { ColorAnimation { duration: 120 } }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 16
                                anchors.rightMargin: 16
                                spacing: 10

                                Label {
                                    text: "⌕"
                                    font.pixelSize: 16
                                    color: Qt.rgba(1, 1, 1, 0.5)
                                }
                                TextField {
                                    id: searchField
                                    Layout.fillWidth: true
                                    placeholderText: "Search by name, username, email, or user ID..."
                                    color: "white"
                                    placeholderTextColor: Qt.rgba(1, 1, 1, 0.4)
                                    background: null
                                    onTextChanged: adminDashboardPage.searchText = text
                                }
                            }
                        }

                        // ---- Filter tabs ----
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Repeater {
                                model: [
                                    { key: "all", label: "All Users" },
                                    { key: "active", label: "Active Users" },
                                    { key: "suspended", label: "Suspended Users" },
                                    { key: "deleted", label: "Permanently Deleted Users" }
                                ]

                                delegate: Rectangle {
                                    Layout.preferredHeight: 34
                                    Layout.preferredWidth: tabLabel.implicitWidth + 28
                                    radius: Theme.radiusFull
                                    color: adminDashboardPage.activeFilter === modelData.key ? Theme.accent : Qt.rgba(1, 1, 1, 0.06)
                                    border.color: adminDashboardPage.activeFilter === modelData.key ? Theme.accent : Qt.rgba(1, 1, 1, 0.14)
                                    border.width: 1

                                    Label {
                                        id: tabLabel
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font.pixelSize: 12
                                        font.bold: true
                                        color: adminDashboardPage.activeFilter === modelData.key ? Theme.primaryDark : Qt.rgba(1, 1, 1, 0.7)
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: adminDashboardPage.activeFilter = modelData.key
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }

                        // ---- Table header ----
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 4
                            spacing: 12
                            Label { text: "USER ID"; Layout.preferredWidth: 60; font.pixelSize: 11; font.bold: true; color: Qt.rgba(1,1,1,0.4) }
                            Label { text: "NAME"; Layout.preferredWidth: 140; font.pixelSize: 11; font.bold: true; color: Qt.rgba(1,1,1,0.4) }
                            Label { text: "USERNAME"; Layout.preferredWidth: 110; font.pixelSize: 11; font.bold: true; color: Qt.rgba(1,1,1,0.4) }
                            Label { text: "EMAIL"; Layout.fillWidth: true; font.pixelSize: 11; font.bold: true; color: Qt.rgba(1,1,1,0.4) }
                            Label { text: "REGISTERED"; Layout.preferredWidth: 100; font.pixelSize: 11; font.bold: true; color: Qt.rgba(1,1,1,0.4) }
                            Label { text: "STATUS"; Layout.preferredWidth: 90; font.pixelSize: 11; font.bold: true; color: Qt.rgba(1,1,1,0.4) }
                            Label { text: "ACTIONS"; Layout.preferredWidth: 130; font.pixelSize: 11; font.bold: true; color: Qt.rgba(1,1,1,0.4) }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Qt.rgba(1,1,1,0.1) }

                        // ---- Table rows ----
                        Repeater {
                            model: adminDashboardPage.filteredUsers()

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 56
                                radius: Theme.radiusSm
                                color: rowMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.07)
                                       : (index % 2 === 0 ? Qt.rgba(1, 1, 1, 0.025) : "transparent")
                                Behavior on color { ColorAnimation { duration: 100 } }

                                MouseArea { id: rowMouse; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 4
                                    anchors.rightMargin: 4
                                    spacing: 12

                                    Label { text: "#" + modelData.uid; Layout.preferredWidth: 60; font.pixelSize: 12; color: Qt.rgba(1,1,1,0.55) }
                                    Label { text: modelData.name; Layout.preferredWidth: 140; font.pixelSize: 13; font.bold: true; color: "white"; elide: Text.ElideRight }
                                    Label { text: modelData.username; Layout.preferredWidth: 110; font.pixelSize: 12; color: Qt.rgba(1,1,1,0.6); elide: Text.ElideRight }
                                    Label { text: modelData.email.length > 0 ? modelData.email : "—"; Layout.fillWidth: true; font.pixelSize: 12; color: Qt.rgba(1,1,1,0.6); elide: Text.ElideRight }
                                    Label { text: modelData.deleted ? modelData.deletionDate : modelData.regDate; Layout.preferredWidth: 100; font.pixelSize: 12; color: Qt.rgba(1,1,1,0.55) }

                                    Rectangle {
                                        Layout.preferredWidth: 90
                                        Layout.preferredHeight: 26
                                        radius: Theme.radiusFull
                                        color: Qt.rgba(adminDashboardPage.statusColor(adminDashboardPage.statusOf(modelData)).r,
                                                       adminDashboardPage.statusColor(adminDashboardPage.statusOf(modelData)).g,
                                                       adminDashboardPage.statusColor(adminDashboardPage.statusOf(modelData)).b, 0.16)
                                        Label {
                                            anchors.centerIn: parent
                                            text: adminDashboardPage.statusLabel(adminDashboardPage.statusOf(modelData))
                                            font.pixelSize: 11
                                            font.bold: true
                                            color: adminDashboardPage.statusColor(adminDashboardPage.statusOf(modelData))
                                        }
                                    }

                                    RowLayout {
                                        Layout.preferredWidth: 130
                                        spacing: 8

                                        AnimatedButton {
                                            text: "Suspend"
                                            primary: false
                                            dark: true
                                            horizontalPadding: 10
                                            implicitHeight: 32
                                            visible: !modelData.deleted && modelData.active
                                            onClicked: revolif.suspendUser(modelData.username)
                                        }
                                        AnimatedButton {
                                            text: "Unsuspend"
                                            primary: true
                                            dark: true
                                            horizontalPadding: 10
                                            implicitHeight: 32
                                            visible: !modelData.deleted && !modelData.active
                                            onClicked: revolif.unsuspendUser(modelData.username)
                                        }
                                        Label {
                                            text: "Reason: " + (modelData.reason || "N/A")
                                            visible: modelData.deleted
                                            font.pixelSize: 11
                                            color: Qt.rgba(1,1,1,0.45)
                                            elide: Text.ElideRight
                                            Layout.preferredWidth: 130
                                        }
                                    }
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.topMargin: 8
                            visible: adminDashboardPage.filteredUsers().length === 0
                            text: "No users match this search or filter."
                            font.pixelSize: 13
                            color: Qt.rgba(1, 1, 1, 0.4)
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }

            // ============================================================
            // ACHIEVEMENT MANAGEMENT
            // ============================================================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "ACHIEVEMENT MANAGEMENT"
                        font.pixelSize: 13
                        font.bold: true
                        font.letterSpacing: 2
                        color: AdminTheme.primaryHover
                    }
                    Item { Layout.fillWidth: true }
                    AnimatedButton {
                        text: "+ Add Achievement"
                        primary: true
                        dark: true
                        onClicked: achievementDialog.openForAdd()
                    }
                }

                GlassPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: achievementFlow.implicitHeight + 40

                    Flow {
                        id: achievementFlow
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 20
                        spacing: 14

                        Repeater {
                            model: adminDashboardPage.achievements

                            delegate: Rectangle {
                                width: 260
                                height: 150
                                radius: Theme.radiusBase
                                color: Qt.rgba(1, 1, 1, 0.05)
                                border.color: Qt.rgba(1, 1, 1, 0.14)
                                border.width: 1

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 6

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label {
                                            text: modelData.name
                                            font.bold: true
                                            font.pixelSize: 14
                                            color: "white"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                        Rectangle {
                                            visible: modelData.isDefault
                                            Layout.preferredWidth: defLabel.implicitWidth + 14
                                            Layout.preferredHeight: 20
                                            radius: Theme.radiusFull
                                            color: Qt.rgba(1, 1, 1, 0.1)
                                            Label {
                                                id: defLabel
                                                anchors.centerIn: parent
                                                text: "Default"
                                                font.pixelSize: 10
                                                color: Qt.rgba(1, 1, 1, 0.6)
                                            }
                                        }
                                    }

                                    Label {
                                        text: modelData.description
                                        font.pixelSize: 12
                                        color: Qt.rgba(1, 1, 1, 0.55)
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        elide: Text.ElideRight
                                        maximumLineCount: 3
                                    }

                                    Label {
                                        text: "Requires " + modelData.requiredGoals + " goals"
                                        font.pixelSize: 11
                                        color: Theme.accentLight
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8
                                        visible: !modelData.isDefault

                                        AnimatedButton {
                                            text: "Edit"
                                            primary: false
                                            dark: true
                                            horizontalPadding: 12
                                            implicitHeight: 32
                                            onClicked: achievementDialog.openForEdit(modelData.id, modelData.name, modelData.description, modelData.requiredGoals)
                                        }
                                        AnimatedButton {
                                            text: "Delete"
                                            primary: false
                                            dark: true
                                            horizontalPadding: 12
                                            implicitHeight: 32
                                            onClicked: {
                                                deleteAchievementConfirm.achievementId = modelData.id
                                                deleteAchievementConfirm.message = "Delete achievement \"" + modelData.name + "\"? This cannot be undone."
                                                deleteAchievementConfirm.open()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: adminDashboardPage.achievements.length === 0
                        text: "No achievements yet."
                        font.pixelSize: 13
                        color: Qt.rgba(1, 1, 1, 0.4)
                    }
                }
            }

            // ============================================================
            // ACCOUNT DELETION REASONS
            // ============================================================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 14

                Label {
                    text: "ACCOUNT DELETION REASONS"
                    font.pixelSize: 13
                    font.bold: true
                    font.letterSpacing: 2
                    color: AdminTheme.primaryHover
                }

                GlassPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: reasonsChart.implicitHeight + 40

                    ColumnLayout {
                        id: reasonsChart
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 14

                        Label {
                            visible: adminDashboardPage.totalDeletions() === 0
                            text: "No permanently deleted accounts on record."
                            font.pixelSize: 13
                            color: Qt.rgba(1, 1, 1, 0.4)
                        }

                        Repeater {
                            model: adminDashboardPage.totalDeletions() > 0 ? adminDashboardPage.deletionStats : []

                            delegate: RowLayout {
                                Layout.fillWidth: true
                                spacing: 14

                                Label {
                                    text: modelData.reason
                                    Layout.preferredWidth: 190
                                    font.pixelSize: 12
                                    color: Qt.rgba(1, 1, 1, 0.75)
                                    wrapMode: Text.Wrap
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 14
                                    radius: 7
                                    color: Qt.rgba(1, 1, 1, 0.06)

                                    Rectangle {
                                        height: parent.height
                                        width: parent.width * (modelData.percent / 100)
                                        radius: 7
                                        color: adminDashboardPage.reasonColor(index)
                                        Behavior on width { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
                                    }
                                }

                                Label {
                                    text: modelData.count + " (" + modelData.percent.toFixed(1) + "%)"
                                    Layout.preferredWidth: 90
                                    horizontalAlignment: Text.AlignRight
                                    font.pixelSize: 11
                                    color: Qt.rgba(1, 1, 1, 0.55)
                                }
                            }
                        }
                    }
                }
            }

            // ============================================================
            // USER FEEDBACK & DELETION REPORTS
            // ============================================================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 14

                Label {
                    text: "USER FEEDBACK & DELETION REPORTS"
                    font.pixelSize: 13
                    font.bold: true
                    font.letterSpacing: 2
                    color: AdminTheme.primaryHover
                }

                GlassPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: feedbackColumn.implicitHeight + 40

                    ColumnLayout {
                        id: feedbackColumn
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 10

                        Label {
                            visible: adminDashboardPage.deletedUsers.length === 0
                            text: "No account deletions reported yet."
                            font.pixelSize: 13
                            color: Qt.rgba(1, 1, 1, 0.4)
                        }

                        Repeater {
                            model: adminDashboardPage.deletedUsers

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: reportRow.implicitHeight + 24
                                radius: Theme.radiusSm
                                color: Qt.rgba(1, 1, 1, 0.04)
                                border.color: Qt.rgba(1, 1, 1, 0.1)
                                border.width: 1

                                RowLayout {
                                    id: reportRow
                                    anchors.fill: parent
                                    anchors.margins: 14
                                    spacing: 16

                                    ColumnLayout {
                                        Layout.preferredWidth: 160
                                        spacing: 2
                                        Label {
                                            text: modelData.name && modelData.name.length > 0 ? modelData.name : modelData.username
                                            font.bold: true
                                            font.pixelSize: 13
                                            color: "white"
                                        }
                                        Label {
                                            text: "@" + modelData.username
                                            font.pixelSize: 11
                                            color: Qt.rgba(1, 1, 1, 0.45)
                                        }
                                    }

                                    Label {
                                        text: modelData.date
                                        Layout.preferredWidth: 90
                                        font.pixelSize: 12
                                        color: Qt.rgba(1, 1, 1, 0.55)
                                    }

                                    Label {
                                        text: modelData.reason && modelData.reason.length > 0 ? modelData.reason : "No reason provided."
                                        Layout.fillWidth: true
                                        font.pixelSize: 12
                                        color: Qt.rgba(1, 1, 1, 0.7)
                                        wrapMode: Text.Wrap
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ============================================================
            // RECENT ACTIVITY
            // ============================================================
            ColumnLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: 28
                spacing: 14

                Label {
                    text: "RECENT ACTIVITY"
                    font.pixelSize: 13
                    font.bold: true
                    font.letterSpacing: 2
                    color: AdminTheme.primaryHover
                }

                GlassPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: activityColumn.implicitHeight + 40

                    ColumnLayout {
                        id: activityColumn
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 4

                        Label {
                            visible: adminDashboardPage.activity.length === 0
                            text: "No recent activity this session."
                            font.pixelSize: 13
                            color: Qt.rgba(1, 1, 1, 0.4)
                        }

                        Repeater {
                            model: adminDashboardPage.activity

                            delegate: RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 36
                                spacing: 12

                                Rectangle {
                                    Layout.preferredWidth: 8
                                    Layout.preferredHeight: 8
                                    radius: 4
                                    Layout.alignment: Qt.AlignVCenter
                                    color: modelData.type === "deletion" ? "#EF4444"
                                           : modelData.type === "suspension" ? "#F59E0B"
                                           : "#10B981"
                                }

                                Label {
                                    text: modelData.detail
                                    Layout.fillWidth: true
                                    font.pixelSize: 13
                                    color: Qt.rgba(1, 1, 1, 0.8)
                                    elide: Text.ElideRight
                                }

                                Label {
                                    text: modelData.timestamp
                                    font.pixelSize: 11
                                    color: Qt.rgba(1, 1, 1, 0.4)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    AddAchievementDialog { id: achievementDialog }

    ConfirmDialog {
        id: deleteAchievementConfirm
        property int achievementId: -1
        confirmText: "Delete"
        onConfirmed: revolif.removeAchievement(achievementId)
    }
}
