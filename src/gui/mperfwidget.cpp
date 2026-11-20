#include "mperfwidget.h"
#include "ui_mperfwidget.h"
#include "modeselectdialog.h"
#include "perfcontroller.h"
#include "compat/qoverloadlegacy.h"

MPerfWidget::MPerfWidget(QWidget *parent) :
    QWidget(parent), ui(new Ui::MPerfWidget), controller(new PerfController(this)), colorLink(false), isSetting(false) {
// Re-declare checkStateChanged() signal if Qt Version is below 6.7.0.
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    void (QCheckBox::*checkStateChanged)(Qt::CheckState) = &QCheckBox::checkStateChanged;
#else // QT_VERSION < 6.7.0
    void (QCheckBox::*checkStateChanged)(int) = &QCheckBox::stateChanged;
#endif

    ui->setupUi(this);
    ui->xyBox->setChecked(!controller->xyLinked());
    // Set up DPI stages
    stages[0].indicator = ui->iButton0; stages[0].xSlider = ui->xSlider0; stages[0].ySlider = ui->ySlider0; stages[0].xBox = ui->xBox0; stages[0].yBox = ui->yBox0; stages[0].enableCheck = nullptr;
    stages[1].indicator = ui->iButton1; stages[1].xSlider = ui->xSlider1; stages[1].ySlider = ui->ySlider1; stages[1].xBox = ui->xBox1; stages[1].yBox = ui->yBox1; stages[1].enableCheck = ui->eBox1;
    stages[2].indicator = ui->iButton2; stages[2].xSlider = ui->xSlider2; stages[2].ySlider = ui->ySlider2; stages[2].xBox = ui->xBox2; stages[2].yBox = ui->yBox2; stages[2].enableCheck = ui->eBox2;
    stages[3].indicator = ui->iButton3; stages[3].xSlider = ui->xSlider3; stages[3].ySlider = ui->ySlider3; stages[3].xBox = ui->xBox3; stages[3].yBox = ui->yBox3; stages[3].enableCheck = ui->eBox3;
    stages[4].indicator = ui->iButton4; stages[4].xSlider = ui->xSlider4; stages[4].ySlider = ui->ySlider4; stages[4].xBox = ui->xBox4; stages[4].yBox = ui->yBox4; stages[4].enableCheck = ui->eBox4;
    stages[5].indicator = ui->iButton5; stages[5].xSlider = ui->xSlider5; stages[5].ySlider = ui->ySlider5; stages[5].xBox = ui->xBox5; stages[5].yBox = ui->yBox5; stages[5].enableCheck = ui->eBox5;
    stages[0].indicatorLabel = ui->iLabel0;
    stages[1].indicatorLabel = ui->iLabel1;
    stages[2].indicatorLabel = ui->iLabel2;
    stages[3].indicatorLabel = ui->iLabel3;
    stages[4].indicatorLabel = ui->iLabel4;
    stages[5].indicatorLabel = ui->iLabel5;

    ui->iButtonO->setLabel(false);
    ui->iButtonO->bigIcons(true);
    ui->iButtonO->allowAlpha(true);

    for(int i = 0; i < DPI_COUNT; i++){
        stages[i].indicator->setLabel(false);
        stages[i].indicator->bigIcons(true);
        stages[i].indicator->allowAlpha(true);
        // Disable tracking so that the slider only sends the value when the user has stopped moving it
        stages[i].xSlider->setTracking(false);
        stages[i].ySlider->setTracking(false);
        // Map signals
        connect(stages[i].indicator, &ColorButton::clicked, [=] (){
            emit colorClicked(i);
        });
        connect(stages[i].indicator, &ColorButton::colorChanged, [=] () {
            emit colorChanged(i);
        });
        // valueChanged is used to update the settings
        // sliderMoved is used to update the spinbox in realtime
        connect(stages[i].xSlider, &QSlider::valueChanged, [=] () {
            emit sliderXValueChanged(i);
        });
        connect(stages[i].ySlider, &QSlider::valueChanged, [=] () {
            emit sliderYValueChanged(i);
        });
        connect(stages[i].xSlider, &QSlider::sliderMoved, [=] () {
            emit sliderXMoved(i);
        });
        connect(stages[i].ySlider, &QSlider::sliderMoved, [=] () {
            emit sliderYMoved(i);
        });
        connect(stages[i].xBox, OVERLOAD_PTR(int, QSpinBox, valueChanged), [=] () {
            emit boxXChanged(i);
        });
        connect(stages[i].yBox, OVERLOAD_PTR(int, QSpinBox, valueChanged), [=] () {
            emit boxYChanged(i);
        });
        if(stages[i].enableCheck)
            // Sniper has no enable
            connect(stages[i].enableCheck, checkStateChanged, [=] () {
                emit enableChanged(i);
            });
        // Hide indicator arrows
        stages[i].indicatorLabel->setVisible(false);
    }
    ui->iLabelO->setVisible(false);
    connect(controller, &PerfController::dpiChanged, this, &MPerfWidget::highlightDpiBox);
}

MPerfWidget::~MPerfWidget(){
    delete ui;
}

void MPerfWidget::setPerf(KbPerf *newPerf, KbProfile *newProfile){
    controller->setPerf(newPerf, newProfile);
    KbPerf* perf = newPerf;
    ui->spinBox->setValue(controller->intensity());
    for(int i = 0; i < DPI_COUNT; i++){
        stages[i].indicator->color(perf->dpiColor(i));
        bool oldLink = controller->xyLinked();
        // Don't force X/Y to the same value if they were set differently in the past
        controller->setXyLinked(false);
        stages[i].xBox->setValue(perf->dpi(i).x());
        stages[i].yBox->setValue(perf->dpi(i).y());
        controller->setXyLinked(oldLink);
        if(stages[i].enableCheck)
            stages[i].enableCheck->setChecked(perf->dpiEnabled(i));
    }
    ui->iButtonO->color(perf->dpiColor(KbPerf::OTHER));
    ui->aSnapBox->setChecked(controller->angleSnap());
    ui->lHeightBox->setCurrentIndex(controller->liftHeightIndex());
    ui->indicBox->setChecked(controller->dpiIndicator());
    highlightDpiBox(controller->dpiIndex());
}

void MPerfWidget::colorClicked(int index){
    // Set all colors at once if Alt is held down
    colorLink = !!(qApp->keyboardModifiers() & Qt::AltModifier);
}

void MPerfWidget::colorChanged(int index){
    QColor color = stages[index].indicator->color();
    if(colorLink){
        // Alt was held down - set all
        for(int i = 0; i < DPI_COUNT; i++)
            stages[i].indicator->color(color);
        ui->iButtonO->color(color);
    }
    controller->setDpiColor(index, color, colorLink);
    colorLink = false;
}

static inline int calculateSliderDpi(QSlider* slider){
    return PerfController::sliderToDpi(slider->sliderPosition(), slider->minimum(), slider->maximum());
}


// Potential recursion in sliderMoved/boxMoved due to rounding
#define SET_START   if(isSetting) return; isSetting = true
#define SET_END     isSetting = false



void MPerfWidget::sliderXValueChanged(int index){
    SET_START;
    QSlider* slider = stages[index].xSlider;
    int value = calculateSliderDpi(slider);
    controller->setDpiX(index, value);
    stages[index].xBox->setValue(value);
    if(controller->xyLinked()){
        stages[index].ySlider->setValue(slider->sliderPosition());
        stages[index].yBox->setValue(value);
    }
    SET_END;
}

void MPerfWidget::sliderXMoved(int index){
    SET_START;
    QSlider* slider = stages[index].xSlider;
    int value = calculateSliderDpi(slider);
    stages[index].xBox->setValue(value);
    if(controller->xyLinked()){
        stages[index].ySlider->setValue(slider->sliderPosition());
        stages[index].yBox->setValue(value);
    }
    SET_END;
}

void MPerfWidget::sliderYValueChanged(int index){
    SET_START;
    QSlider* slider = stages[index].ySlider;
    int value = calculateSliderDpi(slider);
    controller->setDpiY(index, value);
    stages[index].yBox->setValue(value);
    if(controller->xyLinked()){
        stages[index].xSlider->setValue(slider->sliderPosition());
        stages[index].xBox->setValue(value);
    }
    SET_END;
}

void MPerfWidget::sliderYMoved(int index){
    SET_START;
    QSlider* slider = stages[index].ySlider;
    int value = calculateSliderDpi(slider);
    stages[index].yBox->setValue(value);
    if(controller->xyLinked()){
        stages[index].xSlider->setValue(slider->sliderPosition());
        stages[index].xBox->setValue(value);
    }
    SET_END;
}

void MPerfWidget::setLegacyM95(){
    ui->eBox4->setChecked(false);
    ui->eBox5->setChecked(false);
    QWidget* w[] =  {
        ui->indicBox,
        ui->spinBox,

        // Hide colour buttons
        ui->iButton0,
        ui->iButton1,
        ui->iButton2,
        ui->iButton3,
        ui->iButton4,
        ui->iButton5,
        ui->iButtonO,

        // Hide stages 4, 5, and other completely
        ui->label_3,
        ui->label_15,

        // Hide checkboxes as we can't disable DPI stages
        ui->eBox1,
        ui->eBox2,
        ui->eBox3,
        ui->eBox4,
        ui->eBox5,
        ui->label_13,
        ui->label_8,
        ui->xSlider4,
        ui->xSlider5,
        ui->ySlider4,
        ui->ySlider5,
        ui->xBox4,
        ui->xBox5,
        ui->yBox5,
        ui->yBox4
    };
    for(size_t i = 0; i < sizeof(w) / sizeof(QWidget*); i++)
        w[i]->hide();
}

void MPerfWidget::setDarkCore(){
    controller->setDarkCore();

    // Remove lift height elements, which do not work
    ui->lHeightBox->removeItem(1);
    ui->lHeightBox->removeItem(2);

    // Disable DPI profile 4 and 5
    ui->eBox4->setChecked(false);
    ui->eBox5->setChecked(false);

    // Hide DPI profiles 4 and 5
    QWidget* w[] = {
        ui->iButton4,
        ui->iButton5,
        ui->eBox4,
        ui->eBox5,
        ui->label_13,
        ui->label_8,
        ui->xSlider4,
        ui->xSlider5,
        ui->ySlider4,
        ui->ySlider5,
        ui->xBox4,
        ui->xBox5,
        ui->yBox5,
        ui->yBox4
    };

    for(size_t i = 0; i < sizeof(w) / sizeof(QWidget*); i++)
        w[i]->hide();
}

void MPerfWidget::boxXChanged(int index){
    SET_START;
    QSpinBox* box = stages[index].xBox;
    QSlider* slider = stages[index].xSlider;
    int value = box->value();
    slider->setValue(PerfController::dpiToSlider(value, slider->minimum(), slider->maximum()));
    controller->setDpiX(index, value);
    SET_END;
    if(controller->xyLinked())
        stages[index].yBox->setValue(value);
}

void MPerfWidget::boxYChanged(int index){
    SET_START;
    QSpinBox* box = stages[index].yBox;
    QSlider* slider = stages[index].ySlider;
    int value = box->value();
    slider->setValue(PerfController::dpiToSlider(value, slider->minimum(), slider->maximum()));
    controller->setDpiY(index, value);
    SET_END;
    if(controller->xyLinked())
        stages[index].xBox->setValue(value);
}

void MPerfWidget::enableChanged(int index){
    controller->setDpiEnabled(index, stages[index].enableCheck->isChecked());
}

void MPerfWidget::on_xyBox_clicked(bool checked){
    controller->setXyLinked(!checked);
    controller->saveXyLinked();
    // FIXME: update other MPerfWidgets, since this is a global setting
}

void MPerfWidget::on_indicBox_clicked(bool checked){
    controller->setDpiIndicator(checked);
}

void MPerfWidget::on_aSnapBox_clicked(bool checked){
    controller->setAngleSnap(checked);
}

void MPerfWidget::on_lHeightBox_activated(int index){
    controller->setLiftHeightIndex(index);
}

void MPerfWidget::on_copyButton_clicked(){
    KbProfile* profile = controller->profile();
    ModeSelectDialog dialog(this, profile->currentMode(), profile->modes(), tr("Copy performance settings to:"));
    if(dialog.exec() != QDialog::Accepted)
        return;
    controller->copyToModes(dialog.selection());
}

void MPerfWidget::on_spinBox_valueChanged(int arg1){
    controller->setIntensity(arg1);
}

void MPerfWidget::highlightDpiBox(int index){
    for(int i = 0; i < DPI_COUNT; i++){
        stages[i].indicatorLabel->setVisible(false);
    }
    ui->iLabelO->setVisible(false);

    if(index > DPI_COUNT - 1){
        ui->iLabelO->setVisible(true);
        return;
    }

    stages[index].indicatorLabel->setVisible(true);
}

void MPerfWidget::setMaxDpi(ushort dpi){
    for(int i = 0; i < DPI_COUNT; i++){
        stages[i].xBox->setMaximum(dpi);
        stages[i].yBox->setMaximum(dpi);
        stages[i].xSlider->setMaximum(dpi);
        stages[i].ySlider->setMaximum(dpi);
    }
}
