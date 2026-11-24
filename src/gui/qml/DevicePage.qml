pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import CkbNext

// One device's page, laid out like the Widgets one: profiles, modes and Save to hardware on
// the left, the device's sub-tabs on the right.
Pane {
    id: page

    required property DeviceController device

    // The sub-tabs this device has, in the Widgets order
    readonly property var tabs: {
        const list = [];
        if (device.hasLightTab)
            list.push(qsTranslate("KbWidget", "Lighting"));
        if (device.hasBindTab)
            list.push(qsTranslate("KbWidget", "Binding"));
        if (device.hasLegacyLightTab)
            list.push(qsTranslate("KbWidget", "Lighting"));
        if (device.hasKeyboardPerfTab || device.hasMousePerfTab)
            list.push(qsTranslate("KbWidget", "Performance"));
        list.push(qsTranslate("KbWidget", "Device"));
        return list;
    }

    function profileIcon(index: int): string {
        if (index >= device.profileNames.length)
            return "qrc:/img/icon_blank.png";
        if (index === device.hardwareProfileIndex)
            return "qrc:/img/icon_profile_hardware.png";
        return "qrc:/img/icon_profile.png";
    }

    RowLayout {
        anchors.fill: parent
        spacing: 12

        ColumnLayout {
            // A child that fills makes a nested layout fill too; this column keeps its width
            Layout.fillWidth: false
            Layout.preferredWidth: 168
            Layout.fillHeight: true

            ComboBox {
                id: profileBox

                Layout.fillWidth: true
                enabled: AppController.tabsEnabled
                // The profiles, then the entry that opens the profile manager
                model: page.device.profileNames.concat([qsTranslate("KbWidget", "Manage profiles...")])
                currentIndex: page.device.currentProfileIndex

                onActivated: (index) => {
                    if (!page.device.selectProfile(index))
                        console.warn("Device page: the profile manager isn't in the QML UI yet");
                    // Choosing an entry overrides the binding; follow the device again
                    currentIndex = Qt.binding(() => page.device.currentProfileIndex);
                }

                contentItem: RowLayout {
                    spacing: 6

                    Image {
                        Layout.leftMargin: 6
                        source: page.profileIcon(profileBox.currentIndex)
                        sourceSize: Qt.size(16, 16)
                    }
                    Label {
                        Layout.fillWidth: true
                        text: profileBox.displayText
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                delegate: ItemDelegate {
                    required property int index
                    required property string modelData

                    width: ListView.view.width
                    text: modelData
                    font.italic: index === page.device.profileNames.length
                    highlighted: profileBox.highlightedIndex === index
                    icon.source: page.profileIcon(index)
                    icon.color: "transparent"
                    icon.width: 16
                    icon.height: 16
                }
            }

            // Mode list, from step 2.5b
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // A disabled button gets no hover, so the reason it's disabled is shown from here
            Item {
                Layout.fillWidth: true
                implicitHeight: saveButton.implicitHeight
                ToolTip.visible: !page.device.canSaveToHardware && saveHover.hovered
                ToolTip.text: qsTranslate("KbWidget", "Saving to hardware is not supported on this device.")

                HoverHandler {
                    id: saveHover
                }
                Button {
                    id: saveButton

                    anchors.fill: parent
                    text: qsTranslate("KbWidget", "Save to hardware")
                    enabled: page.device.canSaveToHardware && AppController.tabsEnabled
                    onClicked: page.device.saveToHardware()
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            TabBar {
                id: subTabs

                Layout.fillWidth: true
                enabled: AppController.tabsEnabled

                Repeater {
                    model: page.tabs

                    TabButton {
                        required property string modelData

                        text: modelData
                        width: implicitWidth
                    }
                }
            }
            // Sub-tab pages, filled in Phases 3-5. Each page is shown by its own index, as a
            // Repeater's items needn't follow it in a StackLayout's child order.
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Repeater {
                    model: page.tabs

                    Item {
                        required property int index

                        anchors.fill: parent
                        visible: index === subTabs.currentIndex
                    }
                }
            }
        }
    }
}
