#include "settingswidget.h"
#include "ui_settingswidget.h"
#include "settingscontroller.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>

static bool updateRequestedByUser = false;

SettingsWidget::SettingsWidget(QWidget *parent) :
    QWidget(parent), devDetectProgress(nullptr),
    ui(new Ui::SettingsWidget)
{
    // Loads and applies the settings, so before the controls show them
    controller = new SettingsController(this);
    connect(controller, &SettingsController::reportFinished, this, &SettingsWidget::reportFinished);

    ui->setupUi(this);

    // Prepare extra settings
    extra = ui->extraWidget;
    extra->init(controller);

#ifndef Q_OS_MACOS
    ui->uninstallButton->hide();
#endif

    // Load modifier remap
    foreach(const QString& name, SettingsController::modifierNames()){
        ui->capsBox->addItem(name);
        ui->shiftBox->addItem(name);
        ui->ctrlBox->addItem(name);
        ui->altBox->addItem(name);
        ui->winBox->addItem(name);
    }
#ifdef Q_OS_MACOS
    controller->migrateCmdSwap();
#endif
    loadModifiers();

    // Read auto update settings
    ui->autoFWBox->setChecked(controller->autoFirmwareCheck());

    ui->label_2->setText(SettingsController::copyrightText());

#ifdef DISABLE_UPDATER
    ui->autoUpdBox->hide();
    ui->pushButton_2->hide();
#else
    ui->autoUpdBox->setChecked(controller->autoUpdateCheck());
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ui->hiDPIBox->setChecked(controller->hiDpi());
#else
    ui->hiDPIBox->hide();
#endif
}

SettingsWidget::~SettingsWidget(){
    delete ui;
}

void SettingsWidget::pollUpdates(){
    controller->pollUpdates();
}

void SettingsWidget::setStatus(const QString& text){
    ui->devicesLabel->setText(text);
}

void SettingsWidget::setVersion(const CkbVersionNumber& version){
    ui->versionLabel->setText(QString("ckb-next ") + version.toString());
}

void SettingsWidget::loadModifiers(){
    int idx = controller->modifierIndex("caps");
    if(idx >= 0)
        ui->capsBox->setCurrentIndex(idx);
    idx = controller->modifierIndex("lshift");
    if(idx >= 0)
        ui->shiftBox->setCurrentIndex(idx);
    idx = controller->modifierIndex("lctrl");
    if(idx >= 0)
        ui->ctrlBox->setCurrentIndex(idx);
    idx = controller->modifierIndex("lwin");
    if(idx >= 0)
        ui->winBox->setCurrentIndex(idx);
    idx = controller->modifierIndex("lalt");
    if(idx >= 0)
        ui->altBox->setCurrentIndex(idx);
}

void SettingsWidget::updateModifiers(){
    controller->setModifiers(ui->capsBox->currentIndex(), ui->shiftBox->currentIndex(), ui->ctrlBox->currentIndex(),
                             ui->altBox->currentIndex(), ui->winBox->currentIndex());
}

void SettingsWidget::on_pushButton_clicked(){
    qApp->quit();
}

void SettingsWidget::on_capsBox_activated(int index){
    updateModifiers();
}

void SettingsWidget::on_shiftBox_activated(int index){
    updateModifiers();
}

void SettingsWidget::on_ctrlBox_activated(int index){
    updateModifiers();
}

void SettingsWidget::on_altBox_activated(int index){
    updateModifiers();
}

void SettingsWidget::on_winBox_activated(int index){
    updateModifiers();
}

void SettingsWidget::on_autoFWBox_clicked(bool checked){
    controller->setAutoFirmwareCheck(checked);
}

void SettingsWidget::on_aboutQt_clicked(){
    QMessageBox::aboutQt(this);
}

void SettingsWidget::on_generateReportButton_clicked(){
    // Don't allow the script to run twice
    if(controller->reportRunning() || devDetectProgress)
        return;

    QMessageBox::information(this, tr("Generate report"), tr("This will collect software logs, as well as information about the Corsair devices in your system.\n\n"
                                                             "Make sure they are plugged in and click OK."));
    // Check if it was started successfully
    if(!controller->startReport()){
        QMessageBox::critical(this, tr("Error executing ckb-next-dev-detect"), tr("An error occurred while trying to execute ckb-next-dev-detect.\n"
                                                                                  "File not found or not executable."));
        return;
    }
    devDetectProgress = new QProgressDialog(tr("Generating Report"), "", 0, 0, this);
    devDetectProgress->setCancelButton(nullptr);
    devDetectProgress->open();
}

void SettingsWidget::reportFinished(bool ok, const QString& errorOutput, int exitCode){
    devDetectProgress->close();
    delete devDetectProgress;
    devDetectProgress = nullptr;
    if(!ok){
        QString errMsg(tr("An error occurred while trying to execute ckb-next-dev-detect.\n\n"));
        errMsg.append(errorOutput);
        errMsg.append("\n");
        errMsg.append(QString(tr("Return code %1")).arg(exitCode));
        QMessageBox::critical(this, tr("Error executing ckb-next-dev-detect"), errMsg);
        controller->finishReport(false);
        return;
    }

    QString newdir = QFileDialog::getExistingDirectory(this, tr("Select output directory"));
    // Cancelled
    if(newdir.isEmpty()){
        controller->finishReport(true);
        return;
    }
    if(controller->saveReport(newdir)){
        QMessageBox::information(this, tr("Report generated successfully"), tr("The report has been generated successfully."));
        controller->showReport();
    }
    else
        QMessageBox::critical(this, tr("Error writing report"), tr("Could not write report to the selected directory.\n"
                                                                   "Please pick a different one and try again."));
    controller->finishReport(true);
}

void SettingsWidget::on_autoUpdBox_clicked(bool checked)
{
    controller->setAutoUpdateCheck(checked);
}

void SettingsWidget::on_pushButton_2_clicked()
{
    emit checkForUpdates();
    ui->pushButton_2->setEnabled(false);
    ui->pushButton_2->setText(tr("Checking..."));
    updateRequestedByUser = true;
}

void SettingsWidget::enableUpdateButton(){
    ui->pushButton_2->setEnabled(true);
}

void SettingsWidget::setUpdateButtonText(const QString& text){
    if(updateRequestedByUser) {
        ui->pushButton_2->setText(text);
        updateRequestedByUser = false;
    }
}

void SettingsWidget::on_uninstallButton_clicked()
{
#ifdef Q_OS_MACOS
    if(QMessageBox::warning(this, tr("Uninstall ckb-next"), tr("WARNING: Clicking OK will uninstall ckb-next and any older versions of the software from your system.\n\n"
                                                             "Your settings and lighting profiles will be preserved."), QMessageBox::Cancel, QMessageBox::Ok) != QMessageBox::Ok)
        return;

    controller->uninstall();
#endif
}

void SettingsWidget::on_hiDPIBox_clicked(bool checked)
{
    controller->setHiDpi(checked);
    QMessageBox::warning(this, tr("Please restart ckb-next"), tr("Please click the Quit button and restart ckb-next for the changes to take effect."), QMessageBox::Ok);
}
