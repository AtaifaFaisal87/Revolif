import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"

Rectangle {
    id: topBar
    color: "transparent"

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        ColumnLayout {
            spacing: 2
            Label {
                text: revolif.pageTitle
                font.pixelSize: Theme.fontSizeXl
                font.bold: true
                color: AdminTheme.textPrimaryLight
            }
            Label {
                text: "System management & oversight"
                font.pixelSize: Theme.fontSizeSm
                color: AdminTheme.textSecondaryLight
            }
        }

        Item { Layout.fillWidth: true }

        Rectangle {
            Layout.preferredWidth: badgeLabel.implicitWidth + 32
            Layout.preferredHeight: 36
            radius: Theme.radiusFull
            color: AdminTheme.surface
            border.color: AdminTheme.border
            border.width: 1

            RowLayout {
                anchors.centerIn: parent
                spacing: 8
                Rectangle {
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: 8
                    radius: 4
                    color: AdminTheme.primary
                }
                Label {
                    id: badgeLabel
                    text: "Live System"
                    font.pixelSize: Theme.fontSizeSm
                    font.bold: true
                    color: AdminTheme.textPrimaryDark
                }
            }
        }
    }
}
