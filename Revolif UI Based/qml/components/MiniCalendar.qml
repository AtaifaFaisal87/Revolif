import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"

Rectangle {
    id: miniCal
    color: Theme.cardBg
    radius: Theme.radiusLg
    border.color: Theme.border
    border.width: 1
    clip: true

    property date currentDate: new Date()
    readonly property int viewYear: currentDate.getFullYear()
    readonly property int viewMonth: currentDate.getMonth()
    readonly property int daysInMonth: new Date(viewYear, viewMonth + 1, 0).getDate()
    readonly property int firstWeekday: new Date(viewYear, viewMonth, 1).getDay()
    readonly property date today: new Date()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: Qt.formatDate(miniCal.currentDate, "MMMM yyyy")
                font.bold: true
                font.pixelSize: Theme.fontSizeSm
                color: Theme.textPrimary
            }
            Item { Layout.fillWidth: true }
        }

        GridLayout {
            columns: 7
            rowSpacing: 3
            columnSpacing: 3
            Layout.fillWidth: true

            Repeater {
                model: ["S","M","T","W","T","F","S"]
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
                model: miniCal.firstWeekday
                delegate: Item { Layout.fillWidth: true; Layout.minimumWidth: 22; Layout.preferredHeight: 22 }
            }

            Repeater {
                model: miniCal.daysInMonth
                delegate: Rectangle {
                    readonly property int dayNum: index + 1
                    readonly property bool isToday: dayNum === miniCal.today.getDate() &&
                                                     miniCal.viewMonth === miniCal.today.getMonth() &&
                                                     miniCal.viewYear === miniCal.today.getFullYear()

                    Layout.fillWidth: true
                    Layout.minimumWidth: 22
                    Layout.preferredHeight: 22
                    radius: 6
                    color: isToday ? Theme.primary : "transparent"

                    Label {
                        anchors.centerIn: parent
                        text: dayNum
                        font.pixelSize: 11
                        color: isToday ? Theme.textOnPrimary : Theme.textPrimary
                    }
                }
            }
        }
    }
}
