pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import Qt.labs.qmlmodels
import CkbNext

// The loaded animation scripts, as the Widgets "Loaded Animations" dialog: one row per script,
// columns sized to their contents, the description taking the rest.
Window {
    id: window

    title: qsTranslate("AnimDetailsDialog", "Loaded Animations")
    width: 750
    height: 550
    minimumWidth: 750
    minimumHeight: 350
    modality: Qt.ApplicationModal
    color: palette.window

    Shortcut {
        sequences: [StandardKey.Cancel]
        onActivated: window.close()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 9
        spacing: 6

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 1
            background: Rectangle {
                color: window.palette.base
                border.color: window.palette.mid
            }

            HorizontalHeaderView {
                id: header

                anchors.left: table.left
                anchors.top: parent.top
                syncView: table
                clip: true
                resizableColumns: true
                model: [qsTranslate("AnimDetailsDialog", "Name"), qsTranslate("AnimDetailsDialog", "Path"),
                    qsTranslate("AnimDetailsDialog", "Version"), qsTranslate("AnimDetailsDialog", "Author"),
                    qsTranslate("AnimDetailsDialog", "Year"), qsTranslate("AnimDetailsDialog", "License"),
                    qsTranslate("AnimDetailsDialog", "Description")]
            }

            TableView {
                id: table

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: header.bottom
                anchors.bottom: parent.bottom
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                // Each column as wide as its widest cell or title; the last one fills the rest
                columnWidthProvider: (column) => {
                    const fit = Math.max(implicitColumnWidth(column), header.implicitColumnWidth(column));
                    if (column < columns - 1)
                        return fit;
                    let used = 0;
                    for (let c = 0; c < columns - 1; c++)
                        used += columnWidth(c);
                    return Math.max(fit, width - used);
                }
                onWidthChanged: forceLayout()
                ScrollBar.horizontal: ScrollBar {
                }
                ScrollBar.vertical: ScrollBar {
                }

                model: TableModel {
                    TableModelColumn {
                        display: "name"
                    }
                    TableModelColumn {
                        display: "path"
                    }
                    TableModelColumn {
                        display: "version"
                    }
                    TableModelColumn {
                        display: "author"
                    }
                    TableModelColumn {
                        display: "year"
                    }
                    TableModelColumn {
                        display: "license"
                    }
                    TableModelColumn {
                        display: "description"
                    }

                    // TableModel takes only a JavaScript array, not the QVariantList wrapper
                    rows: Array.from(Settings.animationDetails())
                }

                delegate: Rectangle {
                    id: cellBox

                    required property string display
                    required property int row

                    implicitWidth: cell.implicitWidth + 12
                    implicitHeight: cell.implicitHeight + 8
                    // Alternating rows, as the Widgets table
                    color: row % 2 ? window.palette.alternateBase : window.palette.base

                    Label {
                        id: cell

                        anchors.verticalCenter: parent.verticalCenter
                        x: 6
                        text: cellBox.display
                    }
                }
            }
        }

        DialogButtonBox {
            Layout.fillWidth: true
            standardButtons: DialogButtonBox.Ok
            onAccepted: window.close()
        }
    }
}
