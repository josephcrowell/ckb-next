pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import CkbNext

// The Settings tab, laid out like the Widgets one: version and device status at the top, the
// settings in a scrolling area, copyright, About Qt and Quit at the bottom.
Pane {
    id: page

    component SectionTitle: ColumnLayout {
        id: title

        required property string text

        Layout.fillWidth: true
        Layout.topMargin: 6
        spacing: 4

        Label {
            text: title.text
            font.bold: true
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: palette.mid
        }
    }

    // One modifier key's choice, starting from the stored remap
    component ModifierBox: ComboBox {
        required property string key

        Layout.preferredWidth: 140
        model: Settings.modifierNames
        Component.onCompleted: {
            const index = Settings.modifierIndex(key);
            if (index >= 0)
                currentIndex = index;
        }
    }

    // Every change saves all five, as in the Widgets tab
    function saveModifiers() {
        Settings.setModifiers(capsBox.currentIndex, shiftBox.currentIndex, ctrlBox.currentIndex, altBox.currentIndex, winBox.currentIndex);
    }

    component RemapLabel: Label {
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignRight
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        Label {
            text: AppController.versionText
            font.bold: true
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: page.palette.mid
        }
        // Device count and daemon warnings, in rich text
        Label {
            Layout.fillWidth: true
            Layout.topMargin: 6
            text: AppController.statusText || qsTranslate("SettingsWidget", "No devices connected")
            textFormat: Text.StyledText
            wrapMode: Text.Wrap
        }

        ScrollView {
            id: scroll

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 6
            contentWidth: availableWidth

            ColumnLayout {
                width: scroll.availableWidth
                spacing: 6

                SectionTitle {
                    text: qsTranslate("SettingsWidget", "Modifier keys")
                }
                // As the Widgets grid: the first labels keep their width, the others and a
                // spacer at the end share the rest
                GridLayout {
                    Layout.fillWidth: true
                    columns: 7
                    columnSpacing: 6

                    RemapLabel {
                        Layout.fillWidth: false
                        text: "Caps Lock:"
                    }
                    ModifierBox {
                        id: capsBox

                        key: "caps"
                        onActivated: page.saveModifiers()
                    }
                    RemapLabel {
                        text: "Shift:"
                    }
                    ModifierBox {
                        id: shiftBox

                        key: "lshift"
                        onActivated: page.saveModifiers()
                    }
                    Item {
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                    }
                    RemapLabel {
                        Layout.fillWidth: false
                        text: "Ctrl:"
                    }
                    ModifierBox {
                        id: ctrlBox

                        key: "lctrl"
                        onActivated: page.saveModifiers()
                    }
                    RemapLabel {
                        text: "Windows:"
                    }
                    ModifierBox {
                        id: winBox

                        key: "lwin"
                        onActivated: page.saveModifiers()
                    }
                    RemapLabel {
                        text: "Alt:"
                    }
                    ModifierBox {
                        id: altBox

                        key: "lalt"
                        onActivated: page.saveModifiers()
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                }
                Label {
                    text: qsTranslate("SettingsWidget", "These will override the keyboard profile. See \"Binding\" tab for more settings.")
                    font.italic: true
                }

                SectionTitle {
                    text: qsTranslate("SettingsWidget", "Application")
                }
                CheckBox {
                    text: qsTranslate("SettingsWidget", "Check for new firmware automatically")
                    checked: Settings.autoFirmwareCheck
                    onClicked: Settings.autoFirmwareCheck = checked
                    ToolTip.visible: hovered
                    ToolTip.delay: 700
                    ToolTip.text: qsTranslate("SettingsWidget", "You will be notified when new firmware versions are available. You'll have the option to install them immediately or wait until later.")
                }
                CheckBox {
                    visible: Settings.updaterAvailable
                    text: qsTranslate("SettingsWidget", "Check for ckb-next updates on startup")
                    checked: Settings.autoUpdateCheck
                    onClicked: Settings.autoUpdateCheck = checked
                }
                // Generate report (2.6c) and updates (2.6d) go here

                SectionTitle {
                    text: qsTranslate("ExtraSettingsWidget", "Behavior")
                }
                ColumnLayout {
                    Layout.leftMargin: 6
                    spacing: 6

                    CheckBox {
                        text: qsTranslate("ExtraSettingsWidget", "Hide tray icon")
                        checked: Settings.trayIconSuppressed
                        onClicked: Settings.trayIconSuppressed = checked
                        ToolTip.visible: hovered
                        ToolTip.delay: 700
                        ToolTip.text: qsTranslate("ExtraSettingsWidget", "The tray icon will not be displayed. The application will still run in the background; re-launch the app to see the GUI again.")
                    }
                    CheckBox {
                        text: qsTranslate("ExtraSettingsWidget", "Monochrome tray icon")
                        checked: Settings.monochromeTrayIcon
                        onClicked: Settings.monochromeTrayIcon = checked
                    }
                    CheckBox {
                        text: qsTranslate("ExtraSettingsWidget", "Set brightness per-mode")
                        checked: Settings.perModeBrightness
                        onClicked: Settings.perModeBrightness = checked
                        ToolTip.visible: hovered
                        ToolTip.delay: 700
                        ToolTip.text: qsTranslate("ExtraSettingsWidget", "By default, the same brightness level will be applied to all profiles and all devices. Enable this to store it with the lighting mode instead.")
                    }
                    Label {
                        visible: Qt.platform.os === "linux"
                        text: qsTranslate("ExtraSettingsWidget", "Enabling per-mode brightness will disable scrolling on the tray icon to change the brightness level")
                        font.italic: true
                        font.pointSize: Application.font.pointSize * 0.8
                    }
                    CheckBox {
                        text: qsTranslate("ExtraSettingsWidget", "Delay application start")
                        checked: Settings.startDelay
                        onClicked: Settings.startDelay = checked
                    }
                    CheckBox {
                        text: qsTranslate("ExtraSettingsWidget", "Disable animated preview when ckb-next is not in focus")
                        checked: Settings.disablePreviewOnFocusLoss
                        onClicked: Settings.disablePreviewOnFocusLoss = checked
                    }
                    CheckBox {
                        text: qsTranslate("ExtraSettingsWidget", "Disable Anti Aliasing (Requires restart)")
                        checked: Settings.msaaDisabled
                        onClicked: Settings.msaaDisabled = checked
                    }
                    // Linux only; a disabled control gets no hover, so the row shows why
                    RowLayout {
                        visible: Qt.platform.os === "linux"
                        spacing: 6
                        ToolTip.visible: !Settings.idleTimerSupported && idleHover.hovered
                        ToolTip.text: qsTranslate("ExtraSettingsWidget", "This feature is not supported under your current compositor")

                        HoverHandler {
                            id: idleHover
                        }
                        CheckBox {
                            id: idleBox

                            enabled: Settings.idleTimerSupported
                            text: qsTranslate("ExtraSettingsWidget", "Turn lights off when idle for")
                            checked: Settings.idleTimerSupported && Settings.idleTimerEnabled
                            onClicked: Settings.idleTimerEnabled = checked
                        }
                        SpinBox {
                            enabled: idleBox.enabled && idleBox.checked
                            editable: true
                            from: 1
                            to: 99
                            value: Settings.idleTimerDuration
                            textFromValue: (value) => value + qsTranslate("ExtraSettingsWidget", "m", "minutes")
                            valueFromText: (text) => parseInt(text)
                            onValueModified: Settings.idleTimerDuration = value
                        }
                    }
                }

                SectionTitle {
                    text: qsTranslate("ExtraSettingsWidget", "Animation scripts")
                }
                RowLayout {
                    Layout.leftMargin: 6
                    spacing: 6

                    Label {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 140
                        text: qsTranslate("ExtraSettingsWidget", "Location:")
                    }
                    Label {
                        Layout.alignment: Qt.AlignTop
                        Layout.fillWidth: true
                        text: Settings.animationPaths
                        elide: Text.ElideMiddle
                    }
                    ColumnLayout {
                        Layout.alignment: Qt.AlignTop
                        Layout.fillWidth: false

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: Settings.animationCountText
                        }
                        RowLayout {
                            Button {
                                text: qsTranslate("ExtraSettingsWidget", "Re-scan")
                                onClicked: Settings.rescanAnimations()
                            }
                            Button {
                                text: qsTranslate("ExtraSettingsWidget", "Details")
                                onClicked: detailsLoader.active = true
                            }
                        }
                    }
                }

                SectionTitle {
                    text: qsTranslate("ExtraSettingsWidget", "Hardware")
                }
                ColumnLayout {
                    Layout.leftMargin: 6
                    spacing: 6

                    RowLayout {
                        spacing: 6

                        Label {
                            Layout.preferredWidth: 140
                            text: qsTranslate("ExtraSettingsWidget", "Frame rate (FPS):")
                        }
                        SpinBox {
                            editable: true
                            from: 5
                            to: 60
                            value: Settings.frameRate
                            onValueModified: Settings.frameRate = value
                        }
                        Label {
                            visible: Settings.showFrameRateWarning
                            text: qsTranslate("ExtraSettingsWidget", "Warning: high frame rates may cause stability issues")
                        }
                    }
                    CheckBox {
                        text: qsTranslate("ExtraSettingsWidget", "Use spatial dithering to simulate extra color resolution")
                        checked: Settings.dither
                        onClicked: Settings.dither = checked
                        ToolTip.visible: hovered
                        ToolTip.delay: 700
                        ToolTip.text: qsTranslate("ExtraSettingsWidget", "May improve appearance on some keyboards.")
                    }
                }
            }
        }

        // A new window for each opening, so it lists the scripts of the latest scan
        Loader {
            id: detailsLoader

            active: false
            sourceComponent: AnimationDetails {
                visible: true
                onClosing: Qt.callLater(() => detailsLoader.active = false)
            }
        }

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 12

                Label {
                    text: Settings.copyrightText
                    textFormat: Text.StyledText
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                    HoverHandler {
                        cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }
                }
                Label {
                    text: "<a href=\"https://github.com/ckb-next/ckb-next\" style=\"text-decoration:none;\">https://github.com/ckb-next/ckb-next</a>"
                    textFormat: Text.StyledText
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                    HoverHandler {
                        cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }
                }
            }
            // A column of labels can't grow past its widest label, so a spacer pushes the
            // buttons right, as in the Widgets tab
            Item {
                Layout.fillWidth: true
            }
            // A child that fills makes a nested layout fill too; this column keeps its width
            ColumnLayout {
                Layout.alignment: Qt.AlignBottom
                Layout.fillWidth: false

                Button {
                    Layout.fillWidth: true
                    text: qsTranslate("SettingsWidget", "About Qt")
                    onClicked: AppController.aboutQt()
                }
                Button {
                    Layout.fillWidth: true
                    text: qsTranslate("SettingsWidget", "Quit")
                    onClicked: AppController.quit()
                }
            }
        }
    }
}
