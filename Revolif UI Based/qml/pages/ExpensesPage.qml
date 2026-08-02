import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../dialogs"
import "../"

Rectangle {
    id: expensesPage
    color: Theme.bg

    readonly property var categoryColors: ({
        "Food": "#3EBB9E",
        "Transport": "#00674F",
        "Education": "#0A3C30",
        "Shopping": "#E0A030",
        "Bills": "#D9534F",
        "Entertainment": "#73E6CB",
        "Health": "#6B7B77",
        "Other": "#A9B6B2"
    })
    readonly property var fallbackColors: ["#3EBB9E", "#00674F", "#0A3C30", "#E0A030",
        "#D9534F", "#73E6CB", "#6B7B77", "#A9B6B2"]

    property var categoryTotals: ({})

    function colorForCategory(name, index) {
        return expensesPage.categoryColors[name] || expensesPage.fallbackColors[index % expensesPage.fallbackColors.length];
    }

    // Sorted [{ name, value, color }] built from revolif.getSpendingByCategory(),
    // highest spend first, so both the donut and its legend read the same way.
    readonly property var chartSegments: {
        var keys = Object.keys(expensesPage.categoryTotals);
        var list = [];
        for (var i = 0; i < keys.length; i++) {
            list.push({ name: keys[i], value: expensesPage.categoryTotals[keys[i]] });
        }
        list.sort(function(a, b) { return b.value - a.value; });
        for (var j = 0; j < list.length; j++) {
            list[j].color = expensesPage.colorForCategory(list[j].name, j);
        }
        return list;
    }

    readonly property real categoryGrandTotal: {
        var t = 0;
        for (var i = 0; i < chartSegments.length; i++) t += chartSegments[i].value;
        return t;
    }

    function refreshBreakdown() {
        expensesPage.categoryTotals = revolif.getSpendingByCategory();
    }

    Component.onCompleted: refreshBreakdown()

    Connections {
        target: revolif
        function onStatsChanged() { expensesPage.refreshBreakdown() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 96
                radius: Theme.radiusLg
                color: Theme.cardBg
                border.color: Theme.border
                border.width: 1
                
                Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: 14
                        color: Theme.accentLight
                        Label { anchors.centerIn: parent; text: revolif.currencySymbol; color: Theme.primary; font.bold: true; font.pixelSize: 18 }
                    }

                    ColumnLayout {
                        spacing: 2
                        Label {
                            text: revolif.currencySymbol + revolif.totalExpenses.toFixed(2)
                            font.pixelSize: Theme.fontSizeXl
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Label {
                            text: "Total Money Spent"
                            font.pixelSize: Theme.fontSizeSm
                            color: Theme.textSecondary
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: breakdownContent.implicitHeight + 40
            radius: Theme.radiusLg
            color: Theme.cardBg
            border.color: Theme.border
            border.width: 1
            
            Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }
            visible: expensesPage.chartSegments.length > 0

            ColumnLayout {
                id: breakdownContent
                anchors.fill: parent
                anchors.margins: 20
                spacing: 14

                Label {
                    text: "Category Breakdown"
                    font.bold: true
                    font.pixelSize: Theme.fontSizeBase
                    color: Theme.textPrimary
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 28

                    CategoryDonutChart {
                        Layout.preferredWidth: 180
                        Layout.preferredHeight: 180
                        segments: expensesPage.chartSegments
                    }

                    ColumnLayout {
                        // A fixed-ish preferred width instead of fillWidth: with
                        // fillWidth this column used to stretch across the whole
                        // remaining card, dragging the % / amount labels away
                        // from the name+dot over to the far right edge. Capping
                        // the width keeps each row's contents together.
                        Layout.preferredWidth: 260
                        Layout.maximumWidth: 320
                        spacing: 10

                        Repeater {
                            model: expensesPage.chartSegments

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                Rectangle {
                                    Layout.preferredWidth: 12
                                    Layout.preferredHeight: 12
                                    radius: 4
                                    color: modelData.color
                                }

                                Label {
                                    text: modelData.name
                                    font.pixelSize: Theme.fontSizeSm
                                    color: Theme.textPrimary
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    elide: Text.ElideRight
                                }

                                Label {
                                    text: (expensesPage.categoryGrandTotal > 0
                                           ? Math.round((modelData.value / expensesPage.categoryGrandTotal) * 100)
                                           : 0) + "%"
                                    font.pixelSize: Theme.fontSizeSm
                                    font.bold: true
                                    color: Theme.textSecondary
                                    Layout.preferredWidth: 34
                                    horizontalAlignment: Text.AlignRight
                                }

                                Label {
                                    text: revolif.currencySymbol + modelData.value.toFixed(2)
                                    font.pixelSize: Theme.fontSizeSm
                                    color: Theme.textMuted
                                    Layout.preferredWidth: 70
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                        }
                    }

                    // Absorbs the rest of the card's width so the legend
                    // column above stays a compact, readable block next to
                    // the donut instead of stretching edge-to-edge.
                    Item { Layout.fillWidth: true }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Item { Layout.fillWidth: true }

            AnimatedButton {
                text: "+ Add Expense"
                primary: true
                onClicked: addExpenseDialog.open()
            }
        }

        ListView {
            id: expenseListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12
            clip: true
            model: []

            function refresh() { expenseListView.model = revolif.getExpenses() }

            Component.onCompleted: refresh()

            Connections {
                target: revolif
                function onStatsChanged() { expenseListView.refresh() }
            }

            delegate: Rectangle {
                width: ListView.view.width
                height: 70
                radius: Theme.radiusLg
                color: Theme.cardBg
                border.color: Theme.border
                border.width: 1
                
                Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 16

                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: 10
                        color: Theme.accentLight

                        Label {
                            anchors.centerIn: parent
                            text: revolif.currencySymbol
                            font.bold: true
                            color: Theme.primary
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        Layout.fillWidth: true
                        // Without this, a long category string (e.g. "Shopping ·
                        // 02/08/2026") forces this column wider than its fair
                        // share of the row, which shoves the amount/trash icon
                        // sideways and breaks alignment between rows.
                        Layout.minimumWidth: 0

                        Label {
                            text: modelData.title
                            font.bold: true
                            font.pixelSize: Theme.fontSizeBase
                            color: Theme.textPrimary
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            elide: Text.ElideRight
                        }

                        Label {
                            text: modelData.category + " · " + modelData.date
                            font.pixelSize: Theme.fontSizeSm
                            color: Theme.textSecondary
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            elide: Text.ElideRight
                        }
                    }

                    Label {
                        text: revolif.currencySymbol + modelData.amount.toFixed(2)
                        font.bold: true
                        font.pixelSize: Theme.fontSizeBase
                        color: Theme.textPrimary
                        // Fixed width + right alignment so every row's amount
                        // lands in the same column, regardless of how long
                        // the title/category text next to it happens to be.
                        Layout.preferredWidth: 110
                        Layout.alignment: Qt.AlignVCenter
                        horizontalAlignment: Text.AlignRight
                    }

                    // Fixed-width action column so the view/edit/delete
                    // icons land in the same spot on every row.
                    Row {
                        Layout.preferredWidth: 76
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 14

                        ActionIcon {
                            kind: "eye"
                            size: 18
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: expenseDetailsDialog.openFor(modelData)
                            }
                        }

                        ActionIcon {
                            kind: "edit"
                            size: 18
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    expenseDetailsDialog.openFor(modelData);
                                    expenseDetailsDialog.mode = "edit";
                                }
                            }
                        }

                        ActionIcon {
                            kind: "trash"
                            size: 18
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    deleteConfirm.expenseId = modelData.id
                                    deleteConfirm.message = "Delete expense \"" + modelData.title + "\"? This cannot be undone."
                                    deleteConfirm.open()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    AddExpenseDialog { id: addExpenseDialog }

    ExpenseDetailsDialog { id: expenseDetailsDialog }

    ConfirmDialog {
        id: deleteConfirm
        property int expenseId: -1
        confirmText: "Delete"
        onConfirmed: revolif.deleteExpense(expenseId)
    }
}
