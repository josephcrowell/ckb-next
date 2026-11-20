#include <QMessageBox>
#include <QTimer>
#include "fwupgradedialog.h"
#include "firmwarecontroller.h"
#include "ui_fwupgradedialog.h"

FwUpgradeDialog::FwUpgradeDialog(QWidget* parent, CkbVersionNumber newV, const QByteArray& fwBlob, Kb* device) :
    QDialog(parent),
    ui(new Ui::FwUpgradeDialog),
    controller(new FirmwareController(device, fwBlob, this)), evLoop(nullptr), exitSuccess(true)
{
    ui->setupUi(this);
    ui->curLabel->setText(controller->currentVersion().toString());
    ui->newLabel->setText(newV.toString());
    ui->devLabel->setText(controller->deviceName());

    connect(controller, &FirmwareController::progress, this, &FwUpgradeDialog::fwUpdateProgress);
    connect(controller, &FirmwareController::finished, this, &FwUpgradeDialog::fwUpdateFinished);
}

FwUpgradeDialog::~FwUpgradeDialog(){
    delete ui;
}

void FwUpgradeDialog::closeEvent(QCloseEvent* event){
    event->ignore();
}

int FwUpgradeDialog::exec(){
    if(!controller->needsDownload()){
        // If a blob was already specified, check its version and validity
        CkbVersionNumber newV = controller->blobVersion();
        if(newV.isNull()){
            QMessageBox::warning(parentWidget(), tr("Error"), tr("<center>Not a valid firmware for this device.</center>"));
            return QDialog::Rejected;
        }
        ui->newLabel->setText(newV.toString());
    } else {
        // Download a new blob file
        ui->progressBar->show();
        ui->cancelButton->setEnabled(false);
        ui->actionButton->setEnabled(false);
        show();
        // This can take a while
        controller->download();
        // Check validity
        if(controller->blobVersion().isNull()){
            hide();
            QMessageBox::warning(parentWidget(), tr("Error"), tr("<center>There was a problem with the downloaded file.<br />Please try again later.</center>"));
            return QDialog::Rejected;
        }
    }
    // Save temporary file
    if(!controller->saveBlob()){
        hide();
        QMessageBox::warning(parentWidget(), tr("Error"), tr("<center>Unable to save temporary file.</center>"));
        return QDialog::Rejected;
    }
    // Set up UI
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(1);
    ui->progressBar->setTextVisible(false);
    ui->statusLabel->setText(tr("Ready to install new firmware.<br /><br /><b>Disclaimer:</b> ckb-next is not endorsed by Corsair.<br /><br />This is <i>unlikely</i> to cause any damage, however the developers of this software do not accept any responsibility in such an event."));
    ui->cancelButton->setEnabled(true);
    ui->actionButton->setEnabled(true);
    show();
    // Run modal event loop
    evLoop = new QEventLoop(this);
    evLoop->exec();
    delete evLoop;
    hide();
    return exitSuccess ? QDialog::Accepted : QDialog::Rejected;
}

void FwUpgradeDialog::fwUpdateProgress(int current, int total){
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(current);
}

void FwUpgradeDialog::fwUpdateFinished(bool succeeded){
    if(succeeded)
        ui->statusLabel->setText(tr("Update successful!"));
    else
        ui->statusLabel->setText(tr("Update failed."));
    ui->actionButton->setText(tr("OK"));
    ui->actionButton->setEnabled(true);
    ui->progressBar->setMaximum(1);
    ui->progressBar->setValue(1);
    // Exit after 10s
    if(evLoop)
        QTimer::singleShot(10000, evLoop, SLOT(quit()));
}

void FwUpgradeDialog::on_cancelButton_clicked(){
    exitSuccess = false;
    if(evLoop)
        evLoop->quit();
}

void FwUpgradeDialog::on_actionButton_clicked(){
    if(controller->canStart()){
        // Start upgrade
        ui->progressBar->show();
        ui->progressBar->setValue(0);
        ui->progressBar->setMaximum(0);
        ui->progressBar->setTextVisible(true);
        ui->cancelButton->hide();
        ui->actionButton->setEnabled(false);
        ui->actionButton->setText(tr("Please wait"));
        ui->statusLabel->setText(tr("Installing firmware..."));
        controller->start();
    } else {
        // Finished, close dialog.
        if(evLoop)
            evLoop->quit();
    }
}
