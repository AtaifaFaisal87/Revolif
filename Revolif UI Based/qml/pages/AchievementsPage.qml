import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../"

Rectangle {
    id: achievementsPage
    color: Theme.bg

    property int pinnedId: revolif.getDisplayedAchievementId()

    Connections {
        target: revolif
        function onCurrentUserChanged() { achievementsPage.pinnedId = revolif.getDisplayedAchievementId() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: "Click an unlocked achievement to pin it to your profile \u2014 click the pinned one again to unpin it."
            font.pixelSize: Theme.fontSizeSm
            color: Theme.textSecondary
        }

        GridView {
            id: achievementsGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cellWidth: 280
            cellHeight: 140
            model: []

            function refresh() { achievementsGrid.model = revolif.getAchievements() }

            Component.onCompleted: refresh()

            Connections {
                target: revolif
                function onStatsChanged() { achievementsGrid.refresh() }
            }

            delegate: Rectangle {
                width: 264
                height: 124
                radius: Theme.radiusLg
                color: modelData.unlocked ? Theme.cardBg : Theme.hoverBg
                border.color: modelData.id === achievementsPage.pinnedId ? Theme.warning : (modelData.unlocked ? Theme.accent : Theme.border)
                border.width: modelData.id === achievementsPage.pinnedId ? 3 : (modelData.unlocked ? 2 : 1)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8

                    RowLayout {
                        spacing: 8
                        Label {
                            text: modelData.unlocked ? "★" : "☆"
                            font.pixelSize: 24
                            color: modelData.unlocked ? Theme.warning : Theme.textMuted
                        }
                        Label {
                            text: modelData.name
                            font.bold: true
                            font.pixelSize: Theme.fontSizeBase
                            color: modelData.unlocked ? Theme.textPrimary : Theme.textMuted
                            Layout.fillWidth: true
                        }
                        Rectangle {
                            visible: modelData.id === achievementsPage.pinnedId
                            radius: Theme.radiusFull
                            color: Theme.warning
                            Layout.preferredWidth: pinnedLabel.implicitWidth + 14
                            Layout.preferredHeight: pinnedLabel.implicitHeight + 6

                            Label {
                                id: pinnedLabel
                                anchors.centerIn: parent
                                text: "📌 PINNED"
                                font.pixelSize: 10
                                font.bold: true
                                color: "white"
                            }
                        }
                    }

                    Label {
                        text: modelData.description
                        font.pixelSize: Theme.fontSizeSm
                        color: Theme.textSecondary
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "Complete " + modelData.requiredGoals + " goals"
                        font.pixelSize: 11
                        color: Theme.textMuted
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: modelData.unlocked ? Qt.PointingHandCursor : Qt.ArrowCursor
                    enabled: modelData.unlocked
                    onClicked: {
                        // Clicking the already-pinned card unpins it (-1 is
                        // the backend's "no featured achievement" sentinel,
                        // same as the console app's "Clear Selection").
                        if (modelData.id === achievementsPage.pinnedId)
                            revolif.setDisplayedAchievement(-1)
                        else
                            revolif.setDisplayedAchievement(modelData.id)
                    }
                }
            }
        }
    }
}
