#include "extrasettingswidget.h"
#include "ui_extrasettingswidget.h"
#include "animdetailsdialog.h"
#include "settingscontroller.h"

ExtraSettingsWidget::ExtraSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExtraSettingsWidget)
{
    ui->setupUi(this);
}

void ExtraSettingsWidget::init(SettingsController* settingsController){
    // Set first: some controls report their stored values to it as they are set
    controller = settingsController;

    const int rate = controller->frameRate();
    ui->fpsBox->setValue(rate);
    ui->fpsWarnLabel->setVisible(SettingsController::frameRateWarning(rate));

    ui->brightnessBox->setChecked(controller->perModeBrightness());
    ui->ditherBox->setChecked(controller->dither());

#if defined(Q_OS_MACOS) && defined(OS_MAC_LEGACY)
    ui->mAccelBox->setChecked(controller->mouseAccelDisabled());
    ui->sAccelBox->setChecked(controller->scrollAccelDisabled());
    ui->sSpeedBox->setValue(controller->scrollSpeed());
    if(!controller->scrollAccelDisabled())
        ui->sSpeedWidget->hide();
#else
    // Hide all OSX options on Linux
    ui->osxLabel->hide();
    ui->osxLine->hide();
    ui->mAccelBox->hide();
    ui->sAccelBox->hide();
    ui->sSpeedWidget->hide();
#endif

    ui->trayBox->setChecked(controller->trayIconSuppressed());

    // Update animation info
    ui->animPathLabel->setText(SettingsController::animationPaths());
    on_animScanButton_clicked();

    ui->startDelayBox->setChecked(controller->startDelay());
    ui->previewBox->setChecked(controller->disablePreviewOnFocusLoss());

#ifdef Q_OS_LINUX
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    ui->timerMinBox->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
#endif
    if(!SettingsController::idleTimerSupported()){
        QString notSupported(tr("This feature is not supported under your current compositor"));
        ui->timerBox->setToolTip(notSupported);
        ui->timerMinBox->setToolTip(notSupported);
        ui->timerBox->setEnabled(false);
        ui->timerMinBox->setEnabled(false);
    } else {
        ui->timerBox->setChecked(controller->idleTimerEnabled());
        ui->timerMinBox->setEnabled(ui->timerBox->isChecked());
    }
    ui->timerMinBox->setValue(controller->idleTimerDuration());
#else
    ui->scrollWarningLabel->hide();
    ui->timerBox->hide();
    ui->timerMinBox->hide();
#endif
    ui->monochromeBox->setChecked(controller->monochromeTrayIcon());
    ui->msaaBox->setChecked(controller->msaaDisabled());
}

ExtraSettingsWidget::~ExtraSettingsWidget(){
    delete ui;
}

void ExtraSettingsWidget::on_trayBox_clicked(bool checked){
    controller->setTrayIconSuppressed(checked);
}

void ExtraSettingsWidget::on_brightnessBox_clicked(bool checked){
    controller->setPerModeBrightness(checked);
}

void ExtraSettingsWidget::on_animScanButton_clicked(){
    ui->animCountLabel->setText(controller->rescanAnimations());
}

void ExtraSettingsWidget::on_fpsBox_valueChanged(int arg1){
    controller->setFrameRate(arg1);
    // Show warning label if FPS is above 30
    ui->fpsWarnLabel->setVisible(SettingsController::frameRateWarning(arg1));
}

void ExtraSettingsWidget::on_ditherBox_clicked(bool checked){
    controller->setDither(checked);
}

void ExtraSettingsWidget::on_mAccelBox_clicked(bool checked){
#if defined(Q_OS_MACOS) && defined(OS_MAC_LEGACY)
    controller->setMouseAccelDisabled(checked);
#else
    Q_UNUSED(checked);
#endif
}

void ExtraSettingsWidget::on_sAccelBox_clicked(bool checked){
#if defined(Q_OS_MACOS) && defined(OS_MAC_LEGACY)
    controller->setScrollAccelDisabled(checked);
#endif
    ui->sSpeedWidget->setVisible(checked);
}

void ExtraSettingsWidget::on_sSpeedBox_valueChanged(int arg1){
#if defined(Q_OS_MACOS) && defined(OS_MAC_LEGACY)
    controller->setScrollSpeed(arg1);
#else
    Q_UNUSED(arg1);
#endif
}

void ExtraSettingsWidget::on_startDelayBox_clicked(bool checked){
    controller->setStartDelay(checked);
}

void ExtraSettingsWidget::on_previewBox_clicked(bool checked){
    controller->setDisablePreviewOnFocusLoss(checked);
}

void ExtraSettingsWidget::on_detailsBtn_clicked(){
    AnimDetailsDialog* dlg = new AnimDetailsDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    dlg->exec();
}

void ExtraSettingsWidget::on_timerBox_clicked(bool checked){
#ifdef Q_OS_LINUX
    ui->timerMinBox->setEnabled(checked);
    controller->setIdleTimerEnabled(checked);
#endif
}

void ExtraSettingsWidget::on_timerMinBox_editingFinished(){
#ifdef Q_OS_LINUX
    controller->setIdleTimerDuration(ui->timerMinBox->value());
#endif
}

void ExtraSettingsWidget::on_monochromeBox_toggled(bool checked){
    controller->setMonochromeTrayIcon(checked);
}

void ExtraSettingsWidget::on_msaaBox_toggled(bool checked){
    controller->setMsaaDisabled(checked);
}
