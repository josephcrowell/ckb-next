#include <cmath>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QUrl>
#include <QTimer>
#include "firmwarecontroller.h"
#include "fwupgradedialog.h"
#include "kbwidget.h"
#include "kblightwidget.h"
#include "kbprofiledialog.h"
#include "ui_kbwidget.h"
#include "ui_kblightwidget.h"
#ifdef USE_QML_EVENT_MGR
#include "kbmodeeventmgr.h"
#endif
#include "mainwindow.h"
#include <QDebug>
#include <QItemSelectionModel>
#include "modelisttablemodel.h"

KbWidget::KbWidget(QWidget *parent, Kb *_device, XWindowDetector* windowDetector) :
    QWidget(parent),
    device(_device),
    ui(new Ui::KbWidget)
{
    ui->setupUi(this);
    Q_ASSERT(ui->pollRateBox->count() == Kb::POLLRATE_COUNT);
    controller = new DeviceController(device, windowDetector, this);
    ui->modesList->setDevice(device, controller->modes());
    connect(controller, &DeviceController::profilesChanged, this, &KbWidget::updateProfileList);
    connect(controller, &DeviceController::profileSwitched, ui->profileBox, &QComboBox::setCurrentIndex);
    connect(device, &Kb::modeChanged, this, &KbWidget::modeChanged);
    connect(controller, &DeviceController::infoUpdated, this, &KbWidget::devUpdate);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(ui->batteryTrayBox, &QCheckBox::checkStateChanged, this, &KbWidget::batteryTrayBox_checkStateChanged);
#else // QT_VERSION < 6.7.0
    connect(ui->batteryTrayBox, &QCheckBox::stateChanged, this, &KbWidget::batteryTrayBox_stateChanged);
#endif
    connect(ui->modesList->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &KbWidget::currentSelectionChanged);

    // Remove the Legacy Lighting tab on anything other than the K95L
    if(device->model() != KeyMap::K95L)
        ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->legacyLightTab));

    // Remove the Lighting and Performance tabs from non-RGB keyboards
    if(!device->features.contains("rgb")){
        if(device->model() != KeyMap::M95){
            ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->mPerfTab));
            ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->lightTab));
        }
        ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->kPerfTab));
    } else {
        // Remove mouse Performance tab from non-mice
        if(!device->isMouse())
            ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->mPerfTab));
        // Remove keyboard Performance tab from non-keyboards
        if(!device->isKeyboard())
            ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->kPerfTab));
    }

    // If we have an M95, set the performance and lighting tabs as such
    if(device->model() == KeyMap::M95){
        ui->mPerfWidget->setLegacyM95();
        ui->lightWidget->setLegacyM95();
    }

    // If we have a DARK CORE or Dark Core SE, then change the performance tab accordingly
    if(device->model() == KeyMap::DARKCORE)
        ui->mPerfWidget->setDarkCore();

    // If the device is supports it, show the battery
    if(controller->hasBattery()){
        connect(controller, &DeviceController::batteryChanged, this, &KbWidget::updateBattery);
        // The device's battery tray icon, shown or hidden through the check box below
        batteryIcon = new BatteryStatusTrayIcon(device->usbModel, this);
        connect(device, &Kb::batteryChanged, batteryIcon, &BatteryStatusTrayIcon::setBattery);
        connect(device, &Kb::showBatteryIndicatorChanged, batteryIcon, &BatteryStatusTrayIcon::setVisible);
    } else {
        ui->batteryLabel->hide();
        ui->batteryStatusLabel->hide();
        ui->batteryTrayBox->hide();
    }

    // Hide poll rate and FW update as appropriate
    if(!controller->hasPollRate()){
        ui->pollRateBox->hide();
        ui->pollLabel2->hide();
        ui->horizontalLayout_2->removeItem(ui->horizontalSpacer_4);
        delete ui->horizontalSpacer_4;
        ui->horizontalSpacer_4 = nullptr;
    }
    if(!controller->hasFirmwareUpdates()){
        ui->fwUpdButton->hide();
        ui->fwUpdLabel->hide();
        ui->fwUpdLayout->removeItem(ui->fwUpdLayout->itemAt(1));
    }
    // Remove unsupported pollrates (the fastest ones, at the top)
    // Block signals so that the pollrate doesn't change
    bool block = ui->pollRateBox->blockSignals(true);
    while(ui->pollRateBox->count() > controller->pollRates().count())
        ui->pollRateBox->removeItem(0);
    ui->pollRateBox->blockSignals(block);

    // Remove binding tab if the device doesn't support it
    if(!device->features.contains("bind")){
        ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->bindTab));
    }
    // Set monochrome mode according to hardware
    if(device->monochrome)
        ui->lightWidget->setMonochrome();
    // Disable Save to hardware button for unsupported devices
    if(!device->hwload){
        ui->hwSaveButton->setDisabled(true);
        ui->hwSaveButton->setToolTip(QString(tr("Saving to hardware is not supported on this device.")));
    }
    // Read and set the device layout
    controller->setupLayout();
    const QList<QPair<int, QString>>& layouts = controller->layouts();
    if(!layouts.isEmpty()){
        // Clear the "Default" value
        ui->layoutBox->clear();

        // Enable the ComboBox only if there is more than one supported layout
        if(layouts.count() > 1)
            ui->layoutBox->setEnabled(true);

        for(int i = 0; i < layouts.count(); i++)
            ui->layoutBox->addItem(layouts[i].second, layouts[i].first);

        ui->layoutBox->setCurrentIndex(controller->layoutIndex());
    }

    // Set max DPI for mice
    if(device->isMouse())
        ui->mPerfWidget->setMaxDpi(device->getMaxDpi());

    if(!controller->canSetPollRate()){
        ui->pollRateBox->setEnabled(false);
        ui->pollRateBox->setToolTip(tr("This device does not support setting the poll rate through software."));
    }
}

KbWidget::~KbWidget(){
    delete ui;
}

void KbWidget::showDeviceTab(){
    ui->tabWidget->setCurrentIndex(ui->tabWidget->indexOf(ui->devTab));
}

void KbWidget::updateProfileList(){
    // Clear profile list and rebuild
    const QStringList names = controller->profileNames();
    const int hwIndex = controller->hardwareProfileIndex(), currentIndex = controller->currentProfileIndex();
    ui->profileBox->clear();
    for(int i = 0; i < names.count(); i++){
        ui->profileBox->addItem((i == hwIndex) ? QIcon(":/img/icon_profile_hardware.png") : QIcon(":/img/icon_profile.png"),
                                names.at(i));
        if(i == currentIndex)
            ui->profileBox->setCurrentIndex(i);
    }
    ui->profileBox->addItem(QIcon(":/img/icon_blank.png"), tr("Manage profiles..."));
    QFont font = ui->profileBox->font();
    font.setItalic(true);
    ui->profileBox->setItemData(ui->profileBox->count() - 1, font, Qt::FontRole);
}

void KbWidget::on_profileBox_activated(int index){
    if(index < 0)
        return;
    if(!controller->selectProfile(index)){
        // "Manage profiles" option
        KbProfileDialog dialog(this);
        dialog.exec();
        updateProfileList();
        return;
    }
    // Focus the mode list to highlight the whole row properly
    ui->modesList->setFocus();
}

void KbWidget::modeChanged(){
    int index = device->currentProfile()->indexOf(device->currentMode());
    if(index < 0)
        return;
    // Update tabs
    ui->lightWidget->setLight(device->currentLight());
    ui->bindWidget->setBind(device->currentBind(), device->currentProfile());
    ui->kPerfWidget->setPerf(device->currentPerf(), device->currentProfile());
    ui->mPerfWidget->setPerf(device->currentPerf(), device->currentProfile());
    // Update selection
    ui->modesList->setCurrentIndex(ui->modesList->model()->index(index, 0));
}

void KbWidget::currentSelectionChanged(const QModelIndex& current, const QModelIndex& previous){
    const int row = controller->selectMode(current.row());
    if(row >= 0)
        ui->modesList->edit(ui->modesList->model()->index(row, ModeListTableModel::COL_MODE_NAME));
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
#define CHECK_VALUE_UNCHECKED Qt::CheckState::Unchecked
void KbWidget::batteryTrayBox_checkStateChanged(Qt::CheckState state){
#else // QT_VERSION < 6.7.0
#define CHECK_VALUE_UNCHECKED 0
void KbWidget::batteryTrayBox_stateChanged(int state){
#endif
    controller->setShowBatteryIndicator(state != CHECK_VALUE_UNCHECKED);
}

void KbWidget::on_modesList_customContextMenuRequested(const QPoint& pos){
    QModelIndex idx = ui->modesList->indexAt(pos);
#if defined(USE_XCB_EWMH) && defined(USE_QML_EVENT_MGR)
    KbProfile* currentProfile = device->currentProfile();
#endif
    if(!idx.isValid() || !controller->isCurrentModeRow(idx.row()))
        return;

    const int row = idx.row();

    QMenu menu(this);
    QAction* rename = new QAction(tr("Rename..."), this);
    QAction* duplicate = new QAction(tr("Duplicate"), this);
    QAction* del = new QAction(tr("Delete"), this);
    bool canDelete = controller->canDeleteMode();
    if(!canDelete)
        // Can't delete modes if they're required by hardware
        del->setEnabled(false);
    QAction* moveup = new QAction(tr("Move Up"), this);
#if defined(USE_XCB_EWMH) && defined(USE_QML_EVENT_MGR)
    QAction* focusevts = new QAction(tr("Manage Events"), this);
#endif
    if(!controller->canMoveModeUp(row))
        moveup->setEnabled(false);
    QAction* movedown = new QAction(tr("Move Down"), this);
    if(!controller->canMoveModeDown(row))
        movedown->setEnabled(false);
    menu.addAction(rename);
    menu.addAction(duplicate);
    menu.addAction(del);
#if defined(USE_XCB_EWMH) && defined(USE_QML_EVENT_MGR)
    menu.addSeparator();
    menu.addAction(focusevts);
    menu.addSeparator();
#endif
    menu.addAction(moveup);
    menu.addAction(movedown);
    ui->modesList->setIgnoreFocusLoss(true);
    QAction* result = menu.exec(QCursor::pos());
    ui->modesList->setIgnoreFocusLoss(false);
    if(result == rename){
        ui->modesList->edit(ui->modesList->model()->index(idx.row(), ModeListTableModel::COL_MODE_NAME, idx.parent()));
    } else if(result == duplicate){
        controller->duplicateMode(row);
    } else if(result == del){
        if(!canDelete)
            return;
        if(QMessageBox::question(this, tr("Delete mode"), tr("Are you sure you want to delete this mode?")) != QMessageBox::Yes)
            return;
        controller->deleteMode(row);
    } else if(result == moveup){
        controller->moveModeUp(row);
        // Update UI
        modeChanged();
    } else if(result == movedown){
        controller->moveModeDown(row);
        // Update UI
        modeChanged();
    }
#if defined(USE_XCB_EWMH) && defined(USE_QML_EVENT_MGR)
     else if(result == focusevts) {
        openEventMgr(currentProfile->currentMode());
    }
#endif
}

void KbWidget::devUpdate(){
    // Update device tab
    ui->devLabel->setText(controller->modelName());
    ui->serialLabel->setText(controller->serial());
    ui->fwLabel->setText(controller->firmwareVersion());
    ui->bldValLabel->setText(controller->bootloaderVersion());
    // Not all WL devices have a radio BLD so these must be kept separate
    if(!controller->hasRadioFirmware()){
        ui->wlLabel->setVisible(false);
        ui->wlValLabel->setVisible(false);
    } else {
        ui->wlValLabel->setText(controller->radioFirmwareVersion());
    }
    if(!controller->hasRadioBootloader()){
        ui->wlBldLabel->setVisible(false);
        ui->wlBldValLabel->setVisible(false);
    } else {
        ui->wlBldValLabel->setText(controller->radioBootloaderVersion());
    }
    // This is needed so that the currentIndexChanged event doesn't fire
    // If it does, we'll end up with an always greyed out box when pollrate != 1
    bool block = ui->pollRateBox->blockSignals(true);
    ui->pollRateBox->setCurrentIndex(controller->pollRateIndex());
    ui->pollRateBox->blockSignals(block);
    ui->batteryTrayBox->setChecked(controller->showBatteryIndicator());
}

void KbWidget::updateBattery(uint battery, BatteryStatus charging){
    QString label = QString("%1 (%2%), %3")
                    .arg(BatteryStatusTrayIcon::BATTERY_VALUES[BatteryStatusTrayIcon::getBatteryString(battery)],
                    QString::number(battery),
                    BatteryStatusTrayIcon::BATTERY_STATUS_VALUES[charging]);
    ui->batteryStatusLabel->setText(label);
}

void KbWidget::on_hwSaveButton_clicked(){
    controller->saveToHardware();
}

void KbWidget::on_tabWidget_currentChanged(int index){
    if(!device)
        return;
    if(index == ui->tabWidget->count() - 1){
        // Device tab
        updateFwButton();
    }
}

void KbWidget::updateFwButton(){
    ui->fwUpdButton->setText(controller->firmwareButtonText());
}

void KbWidget::setTabBarEnabled(const bool e){
    ui->tabWidget->tabBar()->setEnabled(e);
    ui->profileBox->setEnabled(e);
    ui->modesList->setEnabled(e);
    ui->hwSaveButton->setEnabled(e);
    ui->bindWidget->setControlsEnabled(e);
}

void KbWidget::on_fwUpdButton_clicked(){
    // If alt is pressed, ignore upgrades and go straight to the manual prompt
    if(!(qApp->keyboardModifiers() & Qt::AltModifier)){
        // Check version numbers
        if(!controller->firmwareListDownloaded()){
            ui->fwUpdButton->setText(tr("Checking..."));
            ui->fwUpdButton->setEnabled(false);
        }
        CkbVersionNumber newVersion;
        const DeviceController::FirmwareCheck check = controller->checkFirmware(newVersion);
        ui->fwUpdButton->setEnabled(true);
        updateFwButton();
        if(check == DeviceController::FIRMWARE_CHECK_FAILED){
            if(QMessageBox::question(this, tr("Firmware update"), tr("<center>There was a problem getting the status for this device.<br />Would you like to select a file manually?</center>")) != QMessageBox::Yes)
                return;
            // "Yes" -> fall through to browse file
        } else if(check == DeviceController::FIRMWARE_NEEDS_NEWER_CKB){
            QMessageBox::information(this, tr("Firmware update"), tr("<center>There is a new firmware available for this device (v%1).<br />However, it requires a newer version of ckb-next.<br />Please upgrade ckb-next and try again.</center>").arg(newVersion.toString()));
            return;
        } else if(check == DeviceController::FIRMWARE_UP_TO_DATE){
            if(QMessageBox::question(this, tr("Firmware update"), tr("<center>Your firmware is already up to date.<br />Would you like to select a file manually?</center>")) != QMessageBox::Yes)
                return;
            // "Yes" -> fall through to browse file
        } else {
            // Automatic upgrade. Fetch file from web.
            // FwUpgradeDialog can't be parented to KbWidget because KbWidget may be deleted before the dialog exits
            FwUpgradeDialog dialog(parentWidget(), newVersion, QByteArray(), device);
            dialog.exec();
            return;
        }
    }
    // Browse for file
    QString path = QFileDialog::getOpenFileName(this, tr("Select firmware file"), QStandardPaths::writableLocation(QStandardPaths::DownloadLocation), tr("Firmware blobs (*.bin)"));
    if(path.isEmpty())
        return;
    QByteArray blob;
    if(!FirmwareController::readFile(path, blob)){
        QMessageBox::warning(parentWidget(), tr("Error"), tr("<center>File could not be read.</center>"));
        return;
    }
    FwUpgradeDialog dialog(parentWidget(), CkbVersionNumber(), blob, device);
    dialog.exec();
}

void KbWidget::on_layoutBox_activated(int index){
    // Can't use currentIndexChanged as it fires when the GUI is first drawn
    // before the layout has been initialised
    controller->setLayout(index);
}

void KbWidget::on_pollRateBox_currentIndexChanged(int arg1) {
    if(arg1 == -1)
        return;
    ui->pollRateBox->setEnabled(false);
    controller->setPollRate(arg1);
}

void KbWidget::openEventMgr(KbMode* mode) {
#ifdef USE_QML_EVENT_MGR
    KbModeEventMgr* mgr = new KbModeEventMgr(this, mode->winInfo(), mode->name());
    if(!mgr->isValid()) {
        qWarning() << "Mode event manager failed to load";
        delete mgr;
        return;
    }
    // We set this attribute so that we don't have to free it
    mgr->setAttribute(Qt::WA_DeleteOnClose);
    mgr->show();
#else
    Q_UNUSED(mode);
#endif
}

void KbWidget::on_modesList_doubleClicked(const QModelIndex& index) {
    if(index.column() != ModeListTableModel::COL_EVENT_ICON)
        return;
    controller->enableModeEvents();
    openEventMgr(controller->currentMode());
}

void KbWidget::on_modesList_clicked(const QModelIndex& index) {
    if(index.column() != ModeListTableModel::COL_EVENT_ICON)
        return;
    if(controller->toggleModeEvents(index.row()))
        openEventMgr(controller->currentMode());
}
