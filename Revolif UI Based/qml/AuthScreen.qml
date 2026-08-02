import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"
import "JsHelpers.js" as JsHelpers

Rectangle {
    id: authScreen
    color: Theme.bg

    property bool isLogin: true

    readonly property var monthNames: ["January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"]
    readonly property var dayNumbers: Array.from({length: 31}, function(_, i) { return i + 1 })
    readonly property var yearNumbers: {
        var y = new Date().getFullYear();
        var a = [];
        for (var i = y - 5; i >= y - 100; i--) a.push(i);
        return a;
    }

    // Mirrors the backend's registerUser() checks, plus a calendar-date check
    // for the date of birth that the backend itself never actually performs
    // (its Date constructor accepts any day/month/year combination).
    function validateRegister() {
        if (nameField.text.trim().length === 0) return "Full name is required.";
        if (usernameField.text.trim().length === 0) return "Username is required.";

        var email = emailField.text.trim();
        if (email.length === 0 || email.indexOf("@") === -1 || email.indexOf(".") === -1)
            return "Enter a valid email address.";

        if (passwordField.text.length < 6) return "Password must be at least 6 characters.";

        var selectedDay = authScreen.dayNumbers[dobDay.currentIndex];
        var selectedMonth = dobMonth.currentIndex + 1;
        var selectedYear = Number(dobYear.currentText);
        var dateError = JsHelpers.validateCalendarDate(selectedDay, selectedMonth, selectedYear);
        if (dateError.length > 0) return dateError;

        return "";
    }

    // Login previously had no client-side check at all -- empty fields were
    // sent straight to the backend, which just reported "User not found".
    // This catches that earlier and gives a clearer message.
    function validateLogin() {
        if (usernameField.text.trim().length === 0) return "Username is required.";
        if (passwordField.text.length === 0) return "Password is required.";
        return "";
    }

    ScrollView {
        id: authScroll
        anchors.fill: parent
        contentWidth: availableWidth
        contentHeight: authArea.height
        clip: true
        ScrollBar.vertical: AppScrollBar {}

        Item {
            id: authArea
            width: authScroll.availableWidth
            height: Math.max(authScroll.availableHeight, authContent.implicitHeight + 80)

            ColumnLayout {
                        id: authContent
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: Math.max(40, (authArea.height - implicitHeight) / 2)
                        width: Math.min(420, authArea.width - 48)
                        spacing: 24

                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 120
                        height: 120
                        radius: width / 2
                        color: "#FFFFFF"
                        border.width: 1
                        border.color: Theme.isDark ? Qt.rgba(1, 1, 1, 0.14) : Theme.border
                        antialiasing: true
                        clip: true

                        Rectangle { z: -1; anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: Theme.shadowSoft; antialiasing: true }

                        Image {
                            anchors.fill: parent
                            source: "qrc:/assets/logo_circle_full.png"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: true
                        }
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "REVOLIF"
                        font.pixelSize: Theme.fontSize2xl
                        font.bold: true
                        color: Theme.primary
                        font.letterSpacing: 4
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Life, Beautifully Aligned."
                        font.pixelSize: Theme.fontSizeSm
                        color: Theme.textSecondary
                        font.letterSpacing: 2
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    color: Theme.cardBg
                    radius: Theme.radiusXl
                    Layout.preferredHeight: contentCol.implicitHeight + 64

                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.color: Theme.border
                        border.width: 1
                        radius: Theme.radiusXl
                    }

                    ColumnLayout {
                        id: contentCol
                        anchors.fill: parent
                        anchors.margins: 32
                        spacing: 16

                        RegularExpressionValidator {
                            id: usernameValidator
                            regularExpression: /^[A-Za-z0-9_]*$/
                        }

                        Label {
                            text: isLogin ? "Welcome Back" : "Create Account"
                            font.pixelSize: Theme.fontSizeXl
                            font.bold: true
                            color: Theme.textPrimary
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            visible: !isLogin
                            Label {
                                text: "Full Name"
                                font.pixelSize: Theme.fontSizeSm
                                color: Theme.textSecondary
                            }
                            AppTextField {
                                id: nameField
                                Layout.fillWidth: true
                                placeholderText: "Enter your full name"
                                // Letters and spaces only -- keystrokes that
                                // don't match are rejected before they ever
                                // reach the field, so digits/symbols simply
                                // can't be typed (or pasted) in here.
                                validator: RegularExpressionValidator { regularExpression: /^[A-Za-z ]*$/ }
                            }
                            Label {
                                text: "Letters and spaces only"
                                font.pixelSize: Theme.fontSizeXs
                                color: Theme.textMuted
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Label {
                                text: "Username"
                                font.pixelSize: Theme.fontSizeSm
                                color: Theme.textSecondary
                            }
                            AppTextField {
                                id: usernameField
                                Layout.fillWidth: true
                                placeholderText: "Enter username"
                                // Only enforced when creating a new account --
                                // left open during login so existing users
                                // whose usernames predate this rule aren't
                                // locked out.
                                validator: authScreen.isLogin ? null : usernameValidator
                            }
                            Label {
                                visible: !isLogin
                                text: "Letters, numbers, and underscores only \u2014 no spaces"
                                font.pixelSize: Theme.fontSizeXs
                                color: Theme.textMuted
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            visible: !isLogin
                            Label {
                                text: "Date of Birth"
                                font.pixelSize: Theme.fontSizeSm
                                color: Theme.textSecondary
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                AppComboBox {
                                    id: dobDay
                                    Layout.preferredWidth: 80
                                    model: authScreen.dayNumbers
                                }
                                AppComboBox {
                                    id: dobMonth
                                    Layout.fillWidth: true
                                    model: authScreen.monthNames
                                }
                                AppComboBox {
                                    id: dobYear
                                    Layout.preferredWidth: 130
                                    model: authScreen.yearNumbers
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            visible: !isLogin
                            Label {
                                text: "Email"
                                font.pixelSize: Theme.fontSizeSm
                                color: Theme.textSecondary
                            }
                            AppTextField {
                                id: emailField
                                Layout.fillWidth: true
                                placeholderText: "user@example.com"
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Label {
                                text: "Password"
                                font.pixelSize: Theme.fontSizeSm
                                color: Theme.textSecondary
                            }
                            AppTextField {
                                id: passwordField
                                Layout.fillWidth: true
                                placeholderText: "Enter password"
                                echoMode: TextInput.Password
                            }
                            Label {
                                visible: !isLogin
                                text: "At least 6 characters"
                                font.pixelSize: Theme.fontSizeXs
                                color: Theme.textMuted
                            }
                        }

                        AnimatedButton {
                            Layout.fillWidth: true
                            text: isLogin ? "Login" : "Register"
                            primary: true
                            onClicked: {
                                if (isLogin) {
                                    var loginErr = authScreen.validateLogin();
                                    if (loginErr.length > 0) {
                                        revolif.errorOccurred(loginErr);
                                        return;
                                    }
                                    revolif.login(usernameField.text, passwordField.text)
                                } else {
                                    var err = authScreen.validateRegister();
                                    if (err.length > 0) {
                                        revolif.errorOccurred(err);
                                        return;
                                    }
                                    var dobStr = String(dobDay.currentText).padStart(2, "0") + "/" +
                                                 String(dobMonth.currentIndex + 1).padStart(2, "0") + "/" +
                                                 dobYear.currentText;
                                    revolif.registerUser(nameField.text, usernameField.text, dobStr, emailField.text, passwordField.text)
                                }
                            }
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 4
                            Label {
                                text: isLogin ? "Don't have an account?" : "Already have an account?"
                                font.pixelSize: Theme.fontSizeSm
                                color: Theme.textSecondary
                            }
                            Label {
                                text: isLogin ? "Register" : "Login"
                                font.pixelSize: Theme.fontSizeSm
                                color: Theme.accent
                                font.bold: true
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: authScreen.isLogin = !authScreen.isLogin
                                }
                            }
                        }

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Admin login: admin / admin123"
                            font.pixelSize: 10
                            color: Theme.textMuted
                            visible: isLogin
                        }
                    }
        }
            }
        }
    }

    Connections {
        target: revolif
        function onAccountSuspended(username, canRestore) {
            suspendedDialog.username = username
            suspendedDialog.canRestore = canRestore
            suspendedDialog.open()
        }
    }

    Dialog {
        id: suspendedDialog
        modal: true
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: 380
        standardButtons: Dialog.NoButton

        background: Rectangle {
            color: Theme.cardBg
            radius: Theme.radiusLg
            border.color: Theme.border
            border.width: 1
        }
        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, Theme.isDark ? 0.55 : 0.35) }

        property string username: ""
        property bool canRestore: false

        // Same predefined reasons the console backend offers via
        // selectDeletionReason() / DELETION_REASONS, kept in this fixed
        // order so "Other" is always last. Selecting "Other" reveals a
        // free-text detail field, and the two are combined into
        // "Other: <detail>" on submit -- exactly like the console version --
        // so the Admin Dashboard's deletion-reasons chart tallies correctly
        // instead of everything falling into "Other".
        readonly property var reasonOptions: [
            "No longer using the application",
            "Switching to another app",
            "Privacy concerns",
            "Too many notifications",
            "Account issues/problems",
            "Other"
        ]
        property int selectedReasonIndex: 0
        property string otherDetail: ""

        onOpened: {
            selectedReasonIndex = 0
            otherDetail = ""
        }

        contentItem: ColumnLayout {
            spacing: 16

            Label {
                text: "Account Suspended"
                font.bold: true
                font.pixelSize: Theme.fontSizeLg
                color: Theme.danger
            }

            Label {
                text: suspendedDialog.canRestore
                      ? "Your account is deactivated. You can restore it now, or delete it permanently."
                      : "Your account was suspended by an admin and can't be self-restored. You can delete it permanently, or contact an admin."
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeBase
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: "Why are you deleting your account?"
                    font.pixelSize: Theme.fontSizeSm
                    color: Theme.textSecondary
                }

                AppComboBox {
                    id: reasonCombo
                    Layout.fillWidth: true
                    model: suspendedDialog.reasonOptions
                    currentIndex: suspendedDialog.selectedReasonIndex
                    onActivated: suspendedDialog.selectedReasonIndex = currentIndex
                }

                AppTextField {
                    Layout.fillWidth: true
                    visible: suspendedDialog.selectedReasonIndex === suspendedDialog.reasonOptions.length - 1
                    placeholderText: "Please specify..."
                    text: suspendedDialog.otherDetail
                    onTextChanged: suspendedDialog.otherDetail = text
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                AnimatedButton {
                    Layout.fillWidth: true
                    text: "Cancel"
                    primary: false
                    onClicked: suspendedDialog.close()
                }

                AnimatedButton {
                    Layout.fillWidth: true
                    text: "Restore Account"
                    primary: true
                    visible: suspendedDialog.canRestore
                    onClicked: {
                        revolif.restoreAccount(suspendedDialog.username, passwordField.text)
                        suspendedDialog.close()
                    }
                }

                AnimatedButton {
                    Layout.fillWidth: true
                    text: "Delete Permanently"
                    primary: !suspendedDialog.canRestore
                    onClicked: {
                        var reason = suspendedDialog.reasonOptions[suspendedDialog.selectedReasonIndex]
                        if (reason === "Other")
                            reason = "Other: " + suspendedDialog.otherDetail
                        revolif.permanentlyDeleteSuspendedAccount(suspendedDialog.username, passwordField.text, reason)
                        suspendedDialog.close()
                    }
                }
            }
        }
    }
}
