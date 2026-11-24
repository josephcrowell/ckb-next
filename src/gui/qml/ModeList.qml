pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Dialogs
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

    // The current mode's menu, as in the Widgets list
    Menu {
        id: menu

        property int row: -1
        property bool canDelete: false
        property bool canMoveUp: false
        property bool canMoveDown: false

        MenuItem {
            text: qsTranslate("KbWidget", "Rename...")
            onTriggered: list.editingRow = menu.row
        }
        MenuItem {
            text: qsTranslate("KbWidget", "Duplicate")
            onTriggered: frame.device.duplicateMode(menu.row)
        }
        MenuItem {
            // Modes the hardware needs can't be deleted
            text: qsTranslate("KbWidget", "Delete")
            enabled: menu.canDelete
            onTriggered: deleteDialog.open()
        }
        MenuSeparator {
            id: eventsTop
        }
        MenuItem {
            id: manageEvents

            text: qsTranslate("KbWidget", "Manage Events")
            onTriggered: list.openEventManager()
        }
        MenuSeparator {
            id: eventsBottom
        }
        MenuItem {
            text: qsTranslate("KbWidget", "Move Up")
            enabled: menu.canMoveUp
            onTriggered: frame.device.moveModeUp(menu.row)
        }
        MenuItem {
            text: qsTranslate("KbWidget", "Move Down")
            enabled: menu.canMoveDown
            onTriggered: frame.device.moveModeDown(menu.row)
        }

        // A hidden item still takes space in a Menu, so the events entries are taken out
        Component.onCompleted: {
            if (frame.device.canManageModeEvents)
                return;
            removeItem(eventsBottom);
            removeItem(manageEvents);
            removeItem(eventsTop);
        }
    }

    // A new dialog for each question: Qt 6.11's non-native MessageDialog adds its buttons
    // again each time it is reopened
    Loader {
        id: deleteDialog

        function open() {
            active = true;
        }
        function finish() {
            active = false;
        }

        active: false

        sourceComponent: MessageDialog {
            title: qsTranslate("KbWidget", "Delete mode")
            text: qsTranslate("KbWidget", "Are you sure you want to delete this mode?")
            buttons: MessageDialog.Yes | MessageDialog.No
            Component.onCompleted: open()
            onVisibleChanged: {
                if (!visible)
                    Qt.callLater(deleteDialog.finish);
            }
            onButtonClicked: (button) => {
                // The current mode may have changed (window rules) while the question was open
                if (button === MessageDialog.Yes && frame.device.isCurrentModeRow(menu.row) && frame.device.canDeleteMode())
                    frame.device.deleteMode(menu.row);
            }
        }
    }

    ListView {
        id: list

        // Row whose name is being edited, or -1
        property int editingRow: -1
        // Row being dragged, or -1, and the gap it would drop into (row it goes before)
        property int dragRow: -1
        property int dropGap: -1

        function openMenu(row: int) {
            if (!frame.device.isCurrentModeRow(row))
                return;
            menu.row = row;
            menu.canDelete = frame.device.canDeleteMode();
            menu.canMoveUp = frame.device.canMoveModeUp(row);
            menu.canMoveDown = frame.device.canMoveModeDown(row);
            menu.popup();
        }

        function dragTo(row: int, y: real) {
            dragRow = row;
            // Not past "New mode...", the last row
            dropGap = Math.max(0, Math.min(count - 1, Math.round(y / 28)));
        }

        function finishDrag() {
            if (dragRow < 0)
                return;
            const from = dragRow;
            const to = dropGap > from ? dropGap - 1 : dropGap;
            dragRow = -1;
            frame.device.moveMode(from, to);
        }

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
            // No sync on rowsMoved: the view moves its current row with the item afterwards,
            // which would move a synced row a second time
        }

        // Where a dragged mode would go, as the Widgets drop indicator
        Rectangle {
            visible: list.dragRow >= 0
            z: 2
            width: list.width
            height: 2
            y: list.dropGap * 28 - 1
            color: frame.palette.text
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

                    // Press selects, right press opens the menu, a drag moves the mode
                    MouseArea {
                        property real pressY: 0

                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onPressed: (mouse) => {
                            // A right click doesn't add a mode
                            if (mouse.button === Qt.RightButton && row.isNewModeRow)
                                return;
                            list.selectRow(row.index);
                            if (mouse.button === Qt.RightButton)
                                list.openMenu(row.index);
                            pressY = mouse.y;
                        }
                        onPositionChanged: (mouse) => {
                            if (row.isNewModeRow || !(pressedButtons & Qt.LeftButton))
                                return;
                            if (list.dragRow < 0 && Math.abs(mouse.y - pressY) < Application.styleHints.startDragDistance)
                                return;
                            list.dragTo(row.index, mapToItem(list.contentItem, mouse.x, mouse.y).y);
                        }
                        onReleased: list.finishDrag()
                        onCanceled: list.dragRow = -1
                        onDoubleClicked: (mouse) => {
                            if (!row.isNewModeRow && mouse.button === Qt.LeftButton)
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
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onPressed: (mouse) => {
                            if (mouse.button === Qt.RightButton && row.isNewModeRow)
                                return;
                            list.selectRow(row.index);
                            if (mouse.button === Qt.RightButton)
                                list.openMenu(row.index);
                        }
                        onClicked: (mouse) => {
                            if (mouse.button !== Qt.LeftButton)
                                return;
                            if (!row.isNewModeRow && frame.device.toggleModeEvents(row.index))
                                list.openEventManager();
                        }
                        onDoubleClicked: (mouse) => {
                            if (row.isNewModeRow || mouse.button !== Qt.LeftButton)
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
