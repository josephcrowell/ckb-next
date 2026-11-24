pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Dialogs
import QtQuick.Layouts
import CkbNext

// The main window: one tab per connected device, then Settings. Closing it only hides it;
// the app keeps running until AppController.quit().
ApplicationWindow {
    id: root

    // Set from C++: true for --background and for a restored session
    required property bool startHidden

    function showWindow() {
        root.showNormal();
        root.raise();
        root.requestActivate();
    }

    function finishBackgroundWarning() {
        AppController.setBackgroundWarningShown();
        root.hide();
    }

    title: "ckb-next"
    width: 1000
    height: 750
    visible: !startHidden

    onClosing: (close) => {
        // Qt closes every window when the app quits; a hidden window has nothing to hide
        if (AppController.isQuitting() || !root.visible)
            return;
        close.accepted = false;
        // The window stays visible while the message shows, as the dialog is drawn in it
        if (AppController.backgroundWarningShown())
            root.hide();
        else
            backgroundWarning.open();
    }

    DeviceListModel {
        id: devices

        // A new device is shown if Settings was the current tab, as in the Widgets window
        property bool settingsWasCurrent: false

        onRowsAboutToBeInserted: settingsWasCurrent = tabBar.currentIndex === tabBar.count - 1
        onRowsInserted: (parent, first) => {
            if (settingsWasCurrent)
                Qt.callLater(tabBar.setCurrentIndex, first);
        }
    }

    Connections {
        target: AppController

        function onShowWindowRequested() {
            root.showWindow();
        }
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: AppController.quit()
    }

    MessageDialog {
        id: backgroundWarning

        title: "ckb-next"
        text: qsTranslate("MainWindow", "ckb-next will still run in the background.\nTo close it, choose Quit from the tray menu\nor click \"Quit\" on the Settings screen.")
        buttons: MessageDialog.Ok
        onAccepted: root.finishBackgroundWarning()
        onRejected: root.finishBackgroundWarning()
    }

    header: TabBar {
        id: tabBar

        enabled: AppController.tabsEnabled

        Repeater {
            model: devices

            TabButton {
                required property string name

                text: name
                width: implicitWidth
            }
        }
        TabButton {
            text: qsTranslate("MainWindow", "Settings")
            width: implicitWidth
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

        Repeater {
            model: devices

            // Device page, filled in from step 2.5
            Item {
                required property QtObject device
            }
        }
        // Settings page, filled in from step 2.6
        Item {
        }
    }
}
