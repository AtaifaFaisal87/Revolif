import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../"

Rectangle {
    id: profilePage
    color: Theme.bg

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        clip: true
        ScrollBar.vertical: AppScrollBar {}

        ColumnLayout {
            x: 20
            y: 16
            width: profilePage.width - 40
            spacing: 24

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: content.implicitHeight + 48
                radius: Theme.radiusLg
                color: Theme.cardBg
                border.color: Theme.border
                border.width: 1
                
                Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft }

                ColumnLayout {
                    id: content
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16

                    RowLayout {
                        spacing: 24

                        Rectangle {
                            Layout.preferredWidth: 80
                            Layout.preferredHeight: 80
                            radius: 40
                            color: Theme.primary

                            Label {
                                anchors.centerIn: parent
                                text: revolif.currentUserName.charAt(0).toUpperCase()
                                font.pixelSize: 32
                                font.bold: true
                                color: "white"
                            }
                        }

                        ColumnLayout {
                            spacing: 4

                            Label {
                                text: revolif.currentUserName
                                font.pixelSize: Theme.fontSizeXl
                                font.bold: true
                                color: Theme.textPrimary
                            }

                            Label {
                                text: revolif.currentUserEmail
                                font.pixelSize: Theme.fontSizeBase
                                color: Theme.textSecondary
                            }

                            // Featured achievement badge -- reflects whatever
                            // was pinned on the Achievements page. Hidden
                            // entirely when nothing's pinned rather than
                            // showing an empty pill.
                            RowLayout {
                                spacing: 6
                                visible: revolif.featuredAchievementName.length > 0
                                Layout.topMargin: 4

                                Rectangle {
                                    radius: Theme.radiusFull
                                    color: Theme.accentLight
                                    Layout.preferredHeight: badgeRow.implicitHeight + 10
                                    Layout.preferredWidth: badgeRow.implicitWidth + 20

                                    RowLayout {
                                        id: badgeRow
                                        anchors.centerIn: parent
                                        spacing: 6

                                        Label { text: "\u2605"; font.pixelSize: 13; color: Theme.primary }
                                        Label {
                                            text: revolif.featuredAchievementName
                                            font.pixelSize: Theme.fontSizeXs
                                            font.bold: true
                                            color: Theme.primary
                                        }
                                    }
                                }

                                Label {
                                    text: "\u2715"
                                    font.pixelSize: 12
                                    color: Theme.textMuted
                                    MouseArea {
                                        anchors.fill: parent
                                        anchors.margins: -6
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: revolif.setDisplayedAchievement(-1)
                                    }
                                }
                            }
                        }
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: 24
                        rowSpacing: 16
                        Layout.fillWidth: true

                        Repeater {
                            id: profileRepeater
                            model: []

                            function refresh() {
                                var d = revolif.getProfileData();
                                profileRepeater.model = [
                                    { label: "Username", value: d.username || "", key: "" },
                                    { label: "Name", value: d.name || "", key: "name", editable: true },
                                    { label: "Email", value: d.email || "", key: "email", editable: true },
                                    { label: "Date of Birth", value: d.dob || "", key: "" },
                                    { label: "Member Since", value: d.registrationDate || "", key: "" },
                                    { label: "Login Streak", value: (d.streak || 0) + " days (Best: " + (d.bestStreak || 0) + ")", key: "" }
                                ];
                            }

                            Component.onCompleted: refresh()

                            Connections {
                                target: revolif
                                function onCurrentUserChanged() { profileRepeater.refresh() }
                            }

                            delegate: ColumnLayout {
                                spacing: 4

                                Label {
                                    text: modelData.label
                                    font.pixelSize: Theme.fontSizeSm
                                    color: Theme.textSecondary
                                }

                                RowLayout {
                                    spacing: 8

                                    TextField {
                                        id: fieldInput
                                        Layout.fillWidth: true
                                        text: modelData.value
                                        enabled: modelData.editable === true
                                        implicitHeight: Theme.inputHeight
                                        leftPadding: 16
                                        rightPadding: 16
                                        verticalAlignment: TextInput.AlignVCenter
                                        background: Rectangle {
                                            radius: Theme.inputRadius
                                            color: fieldInput.enabled ? (fieldInput.activeFocus ? Theme.inputBgFocus : Theme.inputBg) : "transparent"
                                            border.color: fieldInput.enabled ? (fieldInput.activeFocus ? Theme.inputBorderFocus : Theme.inputBorder) : "transparent"
                                            border.width: fieldInput.activeFocus ? 2 : 1
                                        }
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeBase
                                    }

                                    AnimatedButton {
                                        text: "Save"
                                        visible: modelData.editable === true
                                        primary: true
                                        onClicked: revolif.updateProfile(modelData.key, fieldInput.text)
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.border
                    }

                    Label {
                        text: "Change Password"
                        font.bold: true
                        font.pixelSize: Theme.fontSizeLg
                        color: Theme.textPrimary
                    }

                    RowLayout {
                        spacing: 12

                        AppTextField {
                            id: oldPass
                            Layout.fillWidth: true
                            placeholderText: "Current password"
                            echoMode: TextInput.Password
                        }

                        AppTextField {
                            id: newPass
                            Layout.fillWidth: true
                            placeholderText: "New password"
                            echoMode: TextInput.Password
                        }

                        AnimatedButton {
                            text: "Change"
                            primary: true
                            onClicked: {
                                if (oldPass.text.length === 0) {
                                    revolif.errorOccurred("Current password is required.");
                                    return;
                                }
                                if (newPass.text.length < 6) {
                                    revolif.errorOccurred("New password must be at least 6 characters.");
                                    return;
                                }
                                var ok = revolif.changePassword(oldPass.text, newPass.text)
                                if (ok) {
                                    oldPass.text = "";
                                    newPass.text = "";
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.border
                    }

                    RowLayout {
                        spacing: 12

                        AnimatedButton {
                            text: "Generate Monthly Report"
                            primary: true
                            onClicked: revolif.generateMonthlyReport()
                        }

                        AnimatedButton {
                            text: "Open Reports Folder"
                            primary: false
                            onClicked: revolif.openReportsFolder()
                        }
                    }

                    Label {
                        text: "Saved as a .txt file in Documents > Revolif Reports."
                        font.pixelSize: 11
                        color: Theme.textMuted
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.border
                    }

                    Label {
                        text: "Delete Account"
                        font.bold: true
                        font.pixelSize: Theme.fontSizeLg
                        color: Theme.danger
                    }

                    Label {
                        text: "This deactivates your account. You'll need to log back in to permanently delete it or restore it."
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        font.pixelSize: Theme.fontSizeSm
                        color: Theme.textSecondary
                    }

                    RowLayout {
                        spacing: 12

                        TextField {
                            id: deleteAccountPass
                            placeholderText: "Enter your password to confirm"
                            echoMode: TextInput.Password
                            Layout.preferredWidth: 260
                            implicitHeight: Theme.inputHeight
                            leftPadding: 16
                            rightPadding: 16
                            verticalAlignment: TextInput.AlignVCenter
                            color: Theme.textPrimary
                            placeholderTextColor: Theme.inputPlaceholder
                            background: Rectangle {
                                radius: Theme.inputRadius
                                color: deleteAccountPass.activeFocus ? Theme.inputBgFocus : Theme.inputBg
                                border.color: Theme.danger
                                border.width: deleteAccountPass.activeFocus ? 2 : 1
                            }
                        }

                        AnimatedButton {
                            text: "Delete My Account"
                            primary: false
                            enabled: deleteAccountPass.text.length > 0
                            onClicked: deleteAccountConfirm.open()
                        }
                    }

                    ConfirmDialog {
                        id: deleteAccountConfirm
                        message: "Are you sure you want to delete your account? You can restore it by logging back in before it's permanently removed."
                        confirmText: "Delete My Account"
                        onConfirmed: {
                            revolif.deleteMyAccount(deleteAccountPass.text)
                            deleteAccountPass.text = ""
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.border
                    }

                    AnimatedButton {
                        Layout.preferredWidth: 140
                        text: "Logout"
                        primary: false
                        onClicked: revolif.logout()
                    }
                }
            }

            Item { Layout.preferredHeight: 4 }
        }
    }
}
