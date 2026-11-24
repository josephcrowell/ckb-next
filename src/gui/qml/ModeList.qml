pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import CkbNext

// The device's modes, like the Widgets mode list: mode icon, name and (with window detection)
// the window rules' lightning, then a "New mode..." row. Choosing a row makes its mode current;
// the "New mode..." row adds a mode and starts renaming it.
Frame {
    id: frame

    required property DeviceController device

    function focusList() {
        list.forceActiveFocus();
    }

    padding: 1
    background: Rectangle {
        color: frame.palette.base
        border.color: frame.palette.mid
    }

    ListView {
        id: list

        // Row whose name is being edited, or -1
        property int editingRow: -1

        function syncCurrent() {
            currentIndex = frame.device.currentModeIndex;
            if (currentIndex >= 0)
                positionViewAtIndex(currentIndex, ListView.Contain);
        }

        function selectRow(row: int) {
            if (row < 0 || row >= count)
                return;
            const added = frame.device.selectMode(row);
            forceActiveFocus();
            if (added >= 0)
                editingRow = added;
        }

        function finishRename(row: int, name: string) {
            if (editingRow !== row)
                return;
            editingRow = -1;
            frame.device.renameMode(row, name);
            forceActiveFocus();
        }

        function cancelRename() {
            editingRow = -1;
            forceActiveFocus();
        }

        function openEventManager() {
            console.warn("Mode list: the event manager isn't in the QML UI yet");
        }

        anchors.fill: parent
        clip: true
        focus: true
        keyNavigationEnabled: false
        boundsBehavior: Flickable.StopAtBounds
        model: frame.device.modeModel
        ScrollBar.vertical: ScrollBar {
        }

        Keys.onUpPressed: selectRow(currentIndex - 1)
        Keys.onDownPressed: selectRow(currentIndex + 1)
        Keys.onPressed: (event) => {
            if (event.key !== Qt.Key_F2 || currentIndex < 0 || currentIndex >= count - 1)
                return;
            editingRow = currentIndex;
            event.accepted = true;
        }

        Component.onCompleted: syncCurrent()

        Connections {
            target: frame.device

            function onCurrentModeChanged() {
                list.syncCurrent();
            }
        }
        // A new profile, or modes moved, keep the current mode but not its row
        Connections {
            target: frame.device.modeModel

            function onModelReset() {
                list.editingRow = -1;
                list.syncCurrent();
            }
            function onLayoutChanged() {
                list.syncCurrent();
            }
        }

        delegate: Rectangle {
            id: row

            required property int index
            required property string modeName
            required property url modeIcon
            required property bool eventsEnabled
            required property bool isNewModeRow

            readonly property bool current: ListView.isCurrentItem
            readonly property color textColor: current ? list.palette.highlightedText : list.palette.text

            width: list.width
            height: 28
            color: current ? list.palette.highlight : rowHover.hovered ? list.palette.window : "transparent"

            HoverHandler {
                id: rowHover
            }

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    MouseArea {
                        anchors.fill: parent
                        onPressed: list.selectRow(row.index)
                        onDoubleClicked: {
                            if (!row.isNewModeRow)
                                list.editingRow = row.index;
                        }
                    }
                    Image {
                        id: modeImage

                        anchors.left: parent.left
                        anchors.leftMargin: 2
                        anchors.verticalCenter: parent.verticalCenter
                        source: row.modeIcon
                        sourceSize: Qt.size(24, 24)
                    }
                    Label {
                        anchors.left: modeImage.right
                        anchors.right: parent.right
                        anchors.leftMargin: 4
                        anchors.verticalCenter: parent.verticalCenter
                        visible: list.editingRow !== row.index
                        text: row.modeName
                        font.italic: row.isNewModeRow
                        color: row.textColor
                        elide: Text.ElideRight
                    }
                    Loader {
                        anchors.left: modeImage.right
                        anchors.right: parent.right
                        anchors.leftMargin: 2
                        anchors.verticalCenter: parent.verticalCenter
                        active: list.editingRow === row.index

                        sourceComponent: TextField {
                            text: row.modeName
                            Component.onCompleted: {
                                selectAll();
                                forceActiveFocus();
                            }
                            // Enter or leaving the field keeps the name, as in the Widgets list
                            onEditingFinished: list.finishRename(row.index, text)
                            Keys.onEscapePressed: list.cancelRename()
                        }
                    }
                }
                // Window rules: a click turns them on or off (opening the editor when there
                // are none yet), a double click opens the editor
                Item {
                    Layout.preferredWidth: 32
                    Layout.fillHeight: true
                    visible: frame.device.canManageModeEvents

                    MouseArea {
                        anchors.fill: parent
                        onPressed: list.selectRow(row.index)
                        onClicked: {
                            if (!row.isNewModeRow && frame.device.toggleModeEvents(row.index))
                                list.openEventManager();
                        }
                        onDoubleClicked: {
                            if (row.isNewModeRow)
                                return;
                            frame.device.enableModeEvents();
                            list.openEventManager();
                        }
                    }
                    Image {
                        anchors.centerIn: parent
                        visible: !row.isNewModeRow
                        source: row.eventsEnabled ? "qrc:/img/lightning.svg" : "qrc:/img/lightning_disabled.svg"
                        // The Widgets list draws the 24 px icon this high
                        sourceSize.height: 18
                    }
                }
            }
        }
    }
}
