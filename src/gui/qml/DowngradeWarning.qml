pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Fusion

// Start-up warning shown when the settings were written by a newer ckb-next. It appears before
// any other window exists, so it is a top-level window of its own. C++ waits for done() and
// then reads confirmed.
Window {
    id: root

    required property string settingsPath
    // True when Ok was chosen
    property bool confirmed: false

    signal done()

    function finish(ok: bool) {
        confirmed = ok;
        done();
    }

    title: qsTranslate("QObject", "Downgrade Warning")
    flags: Qt.Dialog
    transientParent: null
    visible: true
    // A fixed size that fits the text and the buttons
    width: layout.implicitWidth + 2 * pane.padding
    height: layout.implicitHeight + 2 * pane.padding
    minimumWidth: layout.implicitWidth + 2 * pane.padding
    maximumWidth: layout.implicitWidth + 2 * pane.padding
    minimumHeight: layout.implicitHeight + 2 * pane.padding
    maximumHeight: layout.implicitHeight + 2 * pane.padding

    onClosing: finish(false)

    Shortcut {
        sequence: "Escape"
        onActivated: root.finish(false)
    }
    Shortcut {
        sequences: ["Return", "Enter"]
        onActivated: root.finish(true)
    }

    Pane {
        id: pane

        anchors.fill: parent
        padding: 12

        ColumnLayout {
            id: layout

            anchors.fill: parent
            spacing: 12

            Label {
                Layout.preferredWidth: 420
                textFormat: Text.RichText
                wrapMode: Text.Wrap
                text: qsTranslate("QObject", "Downgrading ckb-next will lead to profile data loss. It is recommended to click Cancel and update to the latest version.<br><br>If you wish to continue, back up the settings file located at<blockquote>%1</blockquote>and click OK.").arg(root.settingsPath)
            }

            DialogButtonBox {
                Layout.alignment: Qt.AlignRight
                padding: 0
                standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
                onAccepted: root.finish(true)
                onRejected: root.finish(false)
            }
        }
    }
}
