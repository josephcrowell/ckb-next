#include "kperfwidget.h"
#include "ui_kperfwidget.h"
#include "modeselectdialog.h"
#include "perfcontroller.h"
#include "compat/qoverloadlegacy.h"

///
/// \brief KPerfWidget::KPerfWidget sets up the UI for Keyboard Performace panel
/// \param parent
///
KPerfWidget::KPerfWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KPerfWidget), controller(new PerfController(this))
{
    ui->setupUi(this);
    // Set up indicators
    indicators[0].enable= nullptr; indicators[0].hwEnable = ui->numBox; indicators[0].color1 = ui->numColorOn; indicators[0].color2 = ui->numColorOff; indicators[0].color3= nullptr;
    indicators[1].enable= nullptr; indicators[1].hwEnable = ui->capsBox; indicators[1].color1 = ui->capsColorOn; indicators[1].color2 = ui->capsColorOff; indicators[1].color3= nullptr;
    indicators[2].enable= nullptr; indicators[2].hwEnable = ui->scrollBox; indicators[2].color1 = ui->scrollColorOn; indicators[2].color2 = ui->scrollColorOff; indicators[2].color3= nullptr;
    indicators[3].enable = ui->modeBox; indicators[3].hwEnable= nullptr; indicators[3].color1 = ui->modeColorOn; indicators[3].color2 = ui->modeColorOff; indicators[3].color3= nullptr;
    indicators[4].enable = ui->macroBox; indicators[4].hwEnable= nullptr; indicators[4].color1 = ui->macroColorOn; indicators[4].color2 = ui->macroColorOff; indicators[4].color3= nullptr;
    indicators[5].enable = ui->lightBox; indicators[5].hwEnable= nullptr; indicators[5].color1 = ui->lightColor1; indicators[5].color2 = ui->lightColor2; indicators[5].color3 = ui->lightColor3;
    indicators[6].enable = ui->lockBox; indicators[6].hwEnable= nullptr; indicators[6].color1 = ui->lockColorOn; indicators[6].color2 = ui->lockColorOff; indicators[6].color3= nullptr;
    indicators[7].enable = ui->muteBox; indicators[7].hwEnable= nullptr; indicators[7].color1 = ui->muteColorOn; indicators[7].color2 = ui->muteColorOff; indicators[7].color3 = ui->muteColorNA;
    for(int i = 0; i < I_COUNT; i++){
        indicators[i].color1->setLabel(false);
        indicators[i].color1->bigIcons(true);
        indicators[i].color1->allowAlpha(true);
        indicators[i].color2->setLabel(false);
        indicators[i].color2->bigIcons(true);
        indicators[i].color2->allowAlpha(true);
        if(indicators[i].color3){
            indicators[i].color3->setLabel(false);
            indicators[i].color3->bigIcons(true);
            indicators[i].color3->allowAlpha(true);
        }
        // Map signals
        if(indicators[i].enable){
            connect(indicators[i].enable, &QCheckBox::clicked, [=] () {
                emit uiUpdated(i);
            });
        }
        if(indicators[i].hwEnable){
            connect(indicators[i].hwEnable, OVERLOAD_PTR(int, QComboBox, activated), [=] () {
                emit uiUpdated(i);
            });
        }
        connect(indicators[i].color1, &ColorButton::colorChanged, [=] () {
            emit uiUpdated(i);
        });
        connect(indicators[i].color2, &ColorButton::colorChanged, [=] () {
            emit uiUpdated(i);
        });
        if(indicators[i].color3){
            connect(indicators[i].color3, &ColorButton::colorChanged, [=] () {
                emit uiUpdated(i);
            });
        }
    }

    if (PerfController::muteDeviceSupported()){
        ui->muteDev->setEnabled(true);
        connect(ui->muteDev, OVERLOAD_PTR(int, QComboBox, currentIndexChanged), controller, &PerfController::setMuteDevice);
    }

    k95Widgets << ui->modeBox << ui->modeColorOn << ui->modeColorOff << ui->macroBox << ui->macroColorOn << ui->macroColorOff << ui->k95Label1 << ui->k95Label2 << ui->k95Label3 << ui->k95Label4 << ui->k95Label5 << ui->k95Label6 << ui->k95Line << ui->k95Spacer;
}

///
/// \brief KPerfWidget::~KPerfWidget nothing unusual - just delete the ui object
///
KPerfWidget::~KPerfWidget(){
    delete ui;
}

void KPerfWidget::setPerf(KbPerf* newPerf, KbProfile* newProfile){
    controller->setPerf(newPerf, newProfile);
    // Set intensity
    ui->intensityBox->setValue(controller->intensity());
    // Set hardware indicator values
    for(int i = 0; i < HW_I_COUNT; i++){
        const PerfController::Indicator ind = controller->indicator(i);
        indicators[i].hwEnable->setCurrentIndex((int)ind.mode);
        indicators[i].color1->color(ind.color1);
        indicators[i].color2->color(ind.color2);
        // RGB or BOTH
        if(ind.software){
            indicators[i].color1->setEnabled(true);
            indicators[i].color2->setEnabled(true);
        } else {
            indicators[i].color1->setEnabled(false);
            indicators[i].color2->setEnabled(false);
        }
    }
    // Set software indicators
    for(int i = HW_I_COUNT; i < I_COUNT; i++){
        const PerfController::Indicator ind = controller->indicator(i);
        const bool software = ind.software;
        indicators[i].enable->setChecked(software);
        indicators[i].color1->color(ind.color1);
        indicators[i].color2->color(ind.color2);
        if(indicators[i].color3) indicators[i].color3->color(ind.color3);
        if(software){
            indicators[i].color1->setEnabled(true);
            indicators[i].color2->setEnabled(true);
            if(indicators[i].color3) indicators[i].color3->setEnabled(true);
        } else {
            indicators[i].color1->setEnabled(false);
            indicators[i].color2->setEnabled(false);
            if(indicators[i].color3) indicators[i].color3->setEnabled(false);
        }
    }
    // Set mute device
    ui->muteDev->setCurrentIndex(controller->muteDevice());
    // Hide K95 indicators on non-K95s
    if(controller->hasK95Indicators()){
        foreach(QWidget* w, k95Widgets)
            w->show();
    } else {
        foreach(QWidget* w, k95Widgets)
            w->hide();
        if(!controller->hasMuteIndicator()) { // hide indicators unsupported by STRAFE
            ui->muteBox->hide();
            ui->muteColorOn->hide();
            ui->muteColorOff->hide();
            ui->muteColorNA->hide();
            ui->label_12->hide();
            ui->label_14->hide();
            ui->label_17->hide();
       }
    }
}

void KPerfWidget::uiUpdated(int index){
    if(!controller->perf())
        return;
    const QColor c1 = indicators[index].color1->color();
    const QColor c2 = indicators[index].color2->color();
    const QColor c3 = indicators[index].color3 ? indicators[index].color3->color() : QColor();
    const bool enabled = indicators[index].enable ? indicators[index].enable->isChecked() : false;
    const int hwMode = indicators[index].hwEnable ? indicators[index].hwEnable->currentIndex() : 0;
    const bool software = controller->setIndicator(index, enabled, hwMode, c1, c2, c3);
    // Enable/disable color buttons based on whether or not the indicator is turned on
    if(software){
        indicators[index].color1->setEnabled(true);
        indicators[index].color2->setEnabled(true);
        if(indicators[index].color3) indicators[index].color3->setEnabled(true);
    } else {
        indicators[index].color1->setEnabled(false);
        indicators[index].color2->setEnabled(false);
        if(indicators[index].color3) indicators[index].color3->setEnabled(false);
    }
}

void KPerfWidget::on_intensityBox_valueChanged(int arg1){
    controller->setIntensity(arg1);
}

void KPerfWidget::on_copyButton_clicked(){
    KbProfile* profile = controller->profile();
    ModeSelectDialog dialog(this, profile->currentMode(), profile->modes(), tr("Copy performance settings to:"));
    if(dialog.exec() != QDialog::Accepted)
        return;
    controller->copyToModes(dialog.selection());
}
