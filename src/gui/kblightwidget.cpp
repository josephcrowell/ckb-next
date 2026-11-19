#include "animadddialog.h"
#include "kblightwidget.h"
#include "ui_kblightwidget.h"
#include "mainwindow.h"
#include "keywidgetlayout.h"
#include "lightcontroller.h"

KbLightWidget::KbLightWidget(QWidget *parent) :
    QWidget(parent), controller(new LightController(this)),
    ui(new Ui::KbLightWidget), keyWidget(new KeyWidget(this))
{
    ui->setupUi(this);
    ui->animWidget->setController(controller);
    if(AnimScript::count() == 0)
        ui->animButton->setEnabled(false);

    connect(ui->bgButton, &ColorButton::colorChanged, controller, &LightController::setColor);
    connect(keyWidget, SIGNAL(selectionChanged(QStringList)), this, SLOT(newSelection(QStringList)));
    connect(keyWidget, &KeyWidget::sidelightToggled, controller, &LightController::toggleSidelight); // click on a toggle button, like sidelight
    connect(ui->animWidget, SIGNAL(animChanged(KbAnim*)), this, SLOT(changeAnim(KbAnim*)));
    connect(ui->animWidget, SIGNAL(didUpdateSelection(QStringList)), this, SLOT(changeAnimKeys(QStringList)));
    connect(keyWidget, &KeyWidget::M95LightToggled, controller, &LightController::toggleM95Light);
    connect(controller, &LightController::lightUpdated, this, &KbLightWidget::updateLight);
    connect(controller, &LightController::colorsChanged, this, &KbLightWidget::updateColors);
    connect(controller, &LightController::previewFrame, keyWidget, &KeyWidget::displayColorMap);

    // Restore "show animated" setting
    ui->showAnimBox->setChecked(controller->showAnimated());

    // Make a new KeyWidgetLayout that preserves the aspect ratio and add it
    KeyWidgetLayout* l = new KeyWidgetLayout();
    l->addWidget(keyWidget);
    ui->verticalLayout->removeWidget(ui->controlWidget);
    l->addWidget(ui->controlWidget);

    keyWidget->setFocusPolicy(Qt::ClickFocus);
    ui->verticalLayout->insertLayout(2, l);
    ui->verticalLayout->setStretchFactor(l, 2);
    ui->verticalLayout->setStretchFactor(ui->animWidget, 1);
}

KbLightWidget::~KbLightWidget(){
    delete ui;
}

void KbLightWidget::setLight(KbLight* newLight){
    keyWidget->clearSelection();
    keyWidget->clearAnimation();
    ui->animWidget->clearSelection();
    if(controller->light() == newLight)
        return;
    controller->setLight(newLight);
    updateLight();
    keyWidget->setAnimation(QStringList());
    ui->animWidget->refreshList();
}

void KbLightWidget::setMonochrome(){
    keyWidget->monochrome(true);
}

void KbLightWidget::on_showAnimBox_clicked(bool checked){
    controller->setShowAnimated(checked);
}

void KbLightWidget::updateLight(){
    KbLight* light = controller->light();
    keyWidget->map(light->map());
    keyWidget->colorMap(light->colorMap());
    ui->brightnessBox->setCurrentIndex(light->dimming());
}

void KbLightWidget::updateColors(){
    keyWidget->colorMap(controller->light()->colorMap());
}

void KbLightWidget::newSelection(const QStringList& selection){
    if(!controller->setSelection(selection))
        return;
    ui->animWidget->setSelectedKeys(selection);
    ui->bgButton->color(controller->selectedColor());
    ui->selLabel->setText(controller->selectionText());
}

void KbLightWidget::changeAnim(KbAnim *newAnim){
    if(newAnim)
        keyWidget->setSelection(newAnim->keys());
    else
        keyWidget->clearSelection();
    keyWidget->setAnimationToSelection();
}

void KbLightWidget::changeAnimKeys(const QStringList& keys){
    keyWidget->setAnimation(keys);
}

void KbLightWidget::on_brightnessBox_activated(int index){
    controller->setBrightness(index);
}

void KbLightWidget::on_bgButton_clicked(){
    if(controller->selection().isEmpty())
        keyWidget->selectAll();
}

void KbLightWidget::on_animButton_clicked(){
    if(AnimScript::count() == 0)
        return;
    if(controller->selection().isEmpty())
        keyWidget->selectAll();
    AnimAddDialog dialog(this, controller->light(), controller->selection());
    dialog.exec();
    if(dialog.result() != QDialog::Accepted)
        return;
    KbAnim* animation = controller->addAnimation(dialog.chosenScript(), dialog.chosenPreset(), controller->selection());
    ui->animWidget->addAnim(animation);
    controller->restartAnimation();
}

void KbLightWidget::setLegacyM95(){
    ui->animButton->setEnabled(false);
    ui->bgButton->setEnabled(false);
}
