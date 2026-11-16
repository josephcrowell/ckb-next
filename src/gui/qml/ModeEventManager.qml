pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Fusion
import QtQml.Models

Item {
    id: root

    property var model: null
    property string modeText: ""
    property bool ruleEnabled: false
    // Set by the dialog. Qt Quick takes the colour group from QWindow::isActive(), which is
    // always false for the offscreen window of a QQuickWidget.
    property bool windowActive: false

    signal accepted(enabled: bool)
    signal rejected()
    // Escape outside an editor: QDialog::reject(), which unlike Cancel keeps the edits
    signal escapePressed()

    // Values matched to the Qt Widgets dialog with the Fusion style
    readonly property int layoutMargin: 11
    readonly property int layoutSpacing: 6
    readonly property int spacerHeight: 5
    // Cell text margins; a column is its widest text plus both margins and the grid line
    readonly property int cellLeftMargin: 3
    readonly property int cellRightMargin: 3
    // Fusion's SH_Table_GridLineColor and scroll-area frame colours
    readonly property color gridColor: Qt.darker(palette.window, 1.2)
    readonly property color frameColor: Qt.lighter(Qt.darker(palette.window, 1.4), 1.08)
    SystemPalette {
        id: activeColors
        colorGroup: SystemPalette.Active
    }
    SystemPalette {
        id: inactiveColors
        colorGroup: SystemPalette.Inactive
    }
    // The scene always draws with the inactive group, so it gets the active colours while
    // the dialog is active.
    readonly property SystemPalette windowColors: windowActive ? activeColors : inactiveColors
    palette {
        inactive {
            alternateBase: root.windowColors.alternateBase
            base: root.windowColors.base
            button: root.windowColors.button
            buttonText: root.windowColors.buttonText
            dark: root.windowColors.dark
            highlight: root.windowColors.highlight
            highlightedText: root.windowColors.highlightedText
            light: root.windowColors.light
            mid: root.windowColors.mid
            midlight: root.windowColors.midlight
            placeholderText: root.windowColors.placeholderText
            shadow: root.windowColors.shadow
            text: root.windowColors.text
            window: root.windowColors.window
            windowText: root.windowColors.windowText
        }
    }

    // Like QAbstractItemView: the active selection colours only while the view itself has the
    // focus in an active window. An open editor has the focus, not the view.
    property bool editorFocused: false
    // The open edit delegate, if any
    property var openEditor: null
    readonly property int popupRowHeight: 26

    // TableView is a focus scope, so focusing it would give the focus straight back to an open
    // editor. Commit and close the editor first, as QAbstractItemView does on a press elsewhere.
    function focusTable() {
        if (openEditor) {
            openEditor.commitValue()
            table.closeEditor()
        }
        table.forceActiveFocus(Qt.MouseFocusReason)
    }
    readonly property SystemPalette selectionColors: table.activeFocus && !editorFocused
                                                     && windowActive
                                                     ? activeColors : inactiveColors

    function selectRow(r) {
        if (root.model && r >= 0 && r < root.model.rowCount())
            selModel.setCurrentIndex(root.model.index(r, 0),
                                     ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows)
    }
    function selectFirst() {
        selectRow(0)
    }

    onModelChanged: Qt.callLater(selectFirst)

    // QDialog key handling: Return and Enter click the default button (OK)
    Keys.onReturnPressed: okButton.clicked()
    Keys.onEnterPressed: okButton.clicked()
    Keys.onEscapePressed: root.escapePressed()

    // QToolButton without autoRaise: the panel is always drawn
    component FramedToolButton: ToolButton {
        leftPadding: 7
        rightPadding: 7
        topPadding: 4
        bottomPadding: 3
        background.visible: true
    }

    // The scene clears to white; paint the window colour like the Widgets dialog
    Rectangle {
        anchors.fill: parent
        color: palette.window
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.layoutMargin
        spacing: root.layoutSpacing

        Label {
            text: root.modeText
        }

        // The Widgets spacer rows add their height plus one layout spacing
        Item {
            id: tableWrapper
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.verticalStretchFactor: 1
            Layout.topMargin: root.spacerHeight

            Rectangle {
                id: frame
                anchors.fill: parent
                color: palette.base
                border.width: 1
                border.color: root.frameColor
            }

            // A press on the empty part of the view focuses it, which closes and commits an
            // open editor, like QAbstractItemView. Presses on cells are handled in the delegate.
            TapHandler {
                onPressedChanged: {
                    if (!pressed)
                        return
                    const p = tableWrapper.mapToItem(table.contentItem, point.position)
                    if (table.cellAtPosition(p).x < 0)
                        root.focusTable()
                }
            }

            TableView {
                id: table
                objectName: "ruleTable"
                anchors.fill: parent
                anchors.margins: frame.border.width
                clip: true
                activeFocusOnTab: true
                model: root.model
                interactive: false
                boundsBehavior: Flickable.StopAtBounds

                property real rowHeight: 30

                selectionModel: ItemSelectionModel {
                    id: selModel
                    model: root.model
                    // Like QTableView with SelectRows, the current row is the selected row
                    onCurrentChanged: (current) => {
                        if (current.valid && !isRowSelected(current.row, current.parent))
                            select(current, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows)
                    }
                }
                selectionBehavior: TableView.SelectRows
                selectionMode: TableView.SingleSelection
                // Taps are handled in the delegate: TableView's own tap handling clears the selection
                pointerNavigationEnabled: false
                editTriggers: TableView.AnyKeyPressed
                // TableView keeps these keys; without an open editor they belong to the dialog.
                // (TableView handles them itself in an open editor, before these handlers.)
                // Return and Enter aren't edit keys in QAbstractItemView.
                Keys.onEscapePressed: root.escapePressed()
                Keys.onReturnPressed: okButton.clicked()
                Keys.onEnterPressed: okButton.clicked()

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                WheelHandler {
                    onWheel: (event) => {
                        const maxY = Math.max(0, table.contentHeight - table.height)
                        const step = event.angleDelta.y / 120 * Application.styleHints.wheelScrollLines
                                   * table.rowHeight
                        table.contentY = Math.max(0, Math.min(maxY, table.contentY - step))
                    }
                }

                function fittedWidth(column) {
                    if (column === 4 && table.rows <= 1)
                        return 0
                    if (!table.isColumnLoaded(column))
                        return -1
                    return Math.max(100, table.implicitColumnWidth(column))
                }

                columnWidthProvider: function(column) {
                    if (column !== 2)
                        return fittedWidth(column)
                    let others = 0
                    for (const c of [0, 1, 3, 4]) {
                        const w = fittedWidth(c)
                        others += w < 0 ? 100 : w
                    }
                    return Math.max(100, table.width - others)
                }

                onWidthChanged: forceLayout()

                Connections {
                    target: root.model
                    function onRowsInserted() { table.forceLayout() }
                    function onRowsRemoved() { table.forceLayout() }
                    function onRowsMoved() { table.forceLayout() }
                    function onModelReset() { table.forceLayout() }
                    function onDataChanged() { table.forceLayout() }
                }

                delegate: Rectangle {
                    id: cell

                    required property var model
                    required property int row
                    required property int column
                    required property bool selected
                    required property bool current
                    required property bool editing

                    readonly property bool cellEnabled: table.rows > 0
                                                        && root.model.isCellEnabled(row, column)

                    // QFontMetrics gives whole-pixel text widths
                    implicitWidth: Math.round(cellText.implicitWidth) + root.cellLeftMargin
                                   + root.cellRightMargin + 1
                    implicitHeight: table.rowHeight
                    // QTableView doesn't draw the selection on disabled cells
                    color: selected && cellEnabled ? root.selectionColors.highlight : palette.base

                    Label {
                        id: cellText
                        anchors.fill: parent
                        anchors.leftMargin: root.cellLeftMargin
                        anchors.rightMargin: root.cellRightMargin + 1
                        // QTableView lays the text out in the cell minus its grid line
                        anchors.bottomMargin: 1
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        visible: !cell.editing
                        text: cell.model.display
                        color: !cell.cellEnabled ? palette.disabled.text
                               : cell.selected ? root.selectionColors.highlightedText : palette.text
                    }

                    // QAbstractItemView takes the focus and selects on press
                    TapHandler {
                        onPressedChanged: {
                            // Presses inside an open editor belong to the editor
                            if (!pressed || cell.editing)
                                return
                            root.focusTable()
                            selModel.setCurrentIndex(root.model.index(cell.row, cell.column),
                                                     ItemSelectionModel.ClearAndSelect
                                                     | ItemSelectionModel.Rows)
                        }
                        onDoubleTapped: {
                            // Enabled cells are the editable ones (KbWindowInfoModel::flags)
                            if (cell.cellEnabled)
                                table.edit(root.model.index(cell.row, cell.column))
                        }
                    }

                    // Grid lines
                    Rectangle {
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 1
                        color: root.gridColor
                    }
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: root.gridColor
                    }

                    DragHandler {
                        id: rowDrag
                        target: null
                        xAxis.enabled: false
                        yAxis.enabled: true

                        property int gap: -1

                        // Like QAbstractItemView: the nearer edge of the row under the cursor;
                        // below the last row there is no drop target.
                        function updateGap() {
                            const p = cell.mapToItem(table.contentItem, rowDrag.centroid.position)
                            gap = p.y < 0 || p.y >= table.rows * table.rowHeight
                                  ? -1 : Math.round(p.y / table.rowHeight)
                            dropIndicator.visible = gap >= 0
                            dropIndicator.y = table.y + gap * table.rowHeight - table.contentY
                            const d = Qt.point(rowDrag.centroid.position.x - rowDrag.centroid.pressPosition.x,
                                               rowDrag.centroid.position.y - rowDrag.centroid.pressPosition.y)
                            dragGhost.x = table.x + d.x
                            dragGhost.y = table.y + cell.y - table.contentY + d.y
                        }

                        onCentroidChanged: if (active) updateGap()
                        onActiveChanged: {
                            if (active) {
                                root.selectRow(cell.row)
                                dragGhost.sourceRect = Qt.rect(0, cell.y, table.contentWidth, table.rowHeight)
                                dragGhost.scheduleUpdate()
                                dragGhost.visible = true
                                updateGap()
                                return
                            }
                            dropIndicator.visible = false
                            dragGhost.visible = false
                            const from = cell.row
                            const to = gap
                            gap = -1
                            if (to < 0 || to === from || to === from + 1)
                                return
                            root.model.moveItem(from, to)
                            root.selectRow(to > from ? to - 1 : to)
                        }
                    }

                    TableView.editDelegate: FocusScope {
                        id: editor

                        required property var model
                        required property int column

                        width: parent.width
                        height: parent.height

                        onActiveFocusChanged: root.editorFocused = activeFocus
                        Component.onCompleted: root.openEditor = editor
                        Component.onDestruction: {
                            root.editorFocused = false
                            if (root.openEditor === editor)
                                root.openEditor = null
                        }

                        function commitValue() {
                            if (!editorLoader.item)
                                return
                            if (editor.column === 2)
                                editor.model.edit = (editorLoader.item as TextField).text
                            else
                                editor.model.edit = (editorLoader.item as ComboBox).currentIndex
                        }

                        // Like QStyledItemDelegate: Return/Enter and focus loss commit (TableView
                        // emits commit for both), Escape closes the editor without writing.
                        TableView.onCommit: commitValue()

                        Loader {
                            id: editorLoader
                            anchors.fill: parent
                            focus: true
                            sourceComponent: editor.column === 2 ? textEditor : comboEditor
                        }

                        Component {
                            id: comboEditor
                            ComboBox {
                                id: combo
                                focus: true
                                model: root.model.choices(editor.column)
                                currentIndex: editor.model.edit
                                // QComboBox (Fusion) opens its popup with the current item over the box
                                popup.y: -currentIndex * root.popupRowHeight
                                // QComboBox popup rows: no bold current item, Widgets row height
                                delegate: MenuItem {
                                    required property int index
                                    required property string modelData
                                    width: ListView.view.width
                                    text: modelData
                                    topPadding: 4
                                    bottomPadding: 4
                                    highlighted: combo.highlightedIndex === index
                                    hoverEnabled: combo.hoverEnabled
                                }
                            }
                        }

                        Component {
                            id: textEditor
                            TextField {
                                focus: true
                                // Item-view editors are frameless QLineEdits
                                background: Rectangle {
                                    color: palette.base
                                }
                                leftPadding: 2
                                rightPadding: 2
                                topPadding: 0
                                bottomPadding: 0
                                verticalAlignment: TextInput.AlignVCenter
                                text: editor.model.edit
                                Component.onCompleted: selectAll()
                            }
                        }
                    }
                }
            }

            // QCommonStyle PE_IndicatorItemViewItemDrop: a 1 px line in the text colour
            Rectangle {
                id: dropIndicator
                visible: false
                x: table.x
                width: table.width
                height: 1
                color: palette.text
            }

            // The dragged row's image under the cursor, like QAbstractItemView::startDrag
            ShaderEffectSource {
                id: dragGhost
                visible: false
                sourceItem: table.contentItem
                live: false
                width: sourceRect.width
                height: sourceRect.height
            }
        }

        RowLayout {
            spacing: 3

            CheckBox {
                id: enabledBox
                text: qsTranslate("KbModeEventMgr", "Enabled")
                checked: root.ruleEnabled
                leftPadding: 0
                rightPadding: 5
                spacing: 5
                topPadding: 2
                bottomPadding: 3
            }
            Item {
                Layout.fillWidth: true
            }
            FramedToolButton {
                text: qsTranslate("KbModeEventMgr", "Add")
                onClicked: {
                    const r = root.model.addItem()
                    table.forceLayout()
                    table.positionViewAtRow(r, TableView.Contain)
                    table.edit(root.model.index(r, 2))
                }
            }
            FramedToolButton {
                text: qsTranslate("KbModeEventMgr", "Remove")
                onClicked: {
                    if (selModel.hasSelection)
                        root.model.removeItem(selModel.selectedIndexes[0].row)
                }
            }
            FramedToolButton {
                text: qsTranslate("KbModeEventMgr", "Clear")
                onClicked: {
                    root.model.clear()
                    okButton.forceActiveFocus()
                }
            }
        }

        Label {
            Layout.topMargin: root.spacerHeight
            text: qsTranslate("KbModeEventMgr",
                              "Tip: Run xprop or xwininfo in a terminal and click on a window to find out its title and other values.")
        }

        RowLayout {
            spacing: root.layoutSpacing

            Item {
                Layout.fillWidth: true
            }
            Button {
                text: qsTranslate("KbModeEventMgr", "Cancel")
                onClicked: root.rejected()
            }
            Button {
                id: okButton
                // QDialog gives the default button the focus when it opens
                focus: true
                // OK is always the dialog's default button, which Fusion draws highlighted
                highlighted: true
                text: qsTranslate("KbModeEventMgr", "OK")
                onClicked: root.accepted(enabledBox.checked)
            }
        }
    }
}
