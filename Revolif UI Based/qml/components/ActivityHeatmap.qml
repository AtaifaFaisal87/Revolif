import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"

// LeetCode / GitHub-style contribution grid. Feed it the QVariantList
// returned by revolif.getActivityHeatmap(weeks) — a flat list of
// { date, count, level, weekday } running from the oldest day to today.
Item {
    id: root
    property var days: []          // flat list, oldest -> newest
    property int weeks: 18
    property int cellSize: 13
    property int cellSpacing: 3
    property int hoveredIndex: -1

    implicitHeight: gridColumn.implicitHeight
    implicitWidth: gridColumn.implicitWidth

    readonly property var levelColors: [
        Theme.borderLight,   // 0 - no activity
        "#C9EAE0",
        "#8FD9C4",
        "#4FC2A0",
        Theme.primary        // 4 - busiest days
    ]

    Column {
        id: gridColumn
        spacing: 10

        Row {
            spacing: root.cellSpacing
            Repeater {
                model: root.weeks

                Column {
                    spacing: root.cellSpacing
                    property int weekIndex: index

                    Repeater {
                        model: 7
                        delegate: Rectangle {
                            id: cell
                            property int dayIndex: weekIndex * 7 + index
                            property var dayData: dayIndex < root.days.length ? root.days[dayIndex] : null

                            width: root.cellSize
                            height: root.cellSize
                            radius: 3
                            color: dayData ? root.levelColors[dayData.level] : "transparent"
                            border.width: root.hoveredIndex === dayIndex ? 1 : 0
                            border.color: Theme.primary

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                enabled: cell.dayData !== null
                                onEntered: root.hoveredIndex = cell.dayIndex
                                onExited: if (root.hoveredIndex === cell.dayIndex) root.hoveredIndex = -1

                                ToolTip.visible: containsMouse && cell.dayData
                                ToolTip.delay: 150
                                ToolTip.text: cell.dayData
                                    ? (cell.dayData.count + (cell.dayData.count === 1 ? " activity on " : " activities on ") + cell.dayData.date)
                                    : ""
                            }
                        }
                    }
                }
            }
        }

        Row {
            spacing: 6
            Label {
                text: "Less"
                color: Theme.textMuted
                font.pixelSize: Theme.fontSizeXs
                font.family: Theme.fontFamily
                anchors.verticalCenter: parent.verticalCenter
            }
            Repeater {
                model: root.levelColors
                delegate: Rectangle {
                    width: root.cellSize
                    height: root.cellSize
                    radius: 3
                    color: modelData
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            Label {
                text: "More"
                color: Theme.textMuted
                font.pixelSize: Theme.fontSizeXs
                font.family: Theme.fontFamily
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
