#include <cmath>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>
#include "ckbsettings.h"
#include "gradientdialog.h"
#include "ui_gradientdialog.h"

GradientDialog::GradientDialog(QWidget *parent) :
    QDialog(parent),
    presets(new GradientPresetModel(this)),
    ui(new Ui::GradientDialog)
{
    ui->setupUi(this);
    connect(ui->widget, SIGNAL(currentChanged(QColor,bool,int)), this, SLOT(currentChanged(QColor,bool,int)));
    connect(ui->stopColor, SIGNAL(colorChanged(QColor)), this, SLOT(colorChanged(QColor)));

    // Built-in and stored presets
    presets->load();

    updatePresets();
}

QGradientStops GradientDialog::getGradient(const QGradientStops& prevGradient){
    ui->presetDelete->setEnabled(false);
    ui->presetSave->setEnabled(true);
    ui->widget->setStops(prevGradient);
    exec();
    if(result() != QDialog::Accepted)
        return prevGradient;
    return ui->widget->stops();
}

GradientDialog::~GradientDialog(){
    // Save presets
    presets->store();
    delete ui;
}

QIcon GradientDialog::makeIcon(const Preset& preset){
    // Paint gradient into a square
    int w = 60, h = 60;
    QImage image(w, h, QImage::Format_RGB888);
    QPainter painter(&image);
    // Draw background
    for(int x = 0; x < w; x += 30){
        for(int y = 0; y < h; y += 30){
            painter.fillRect(x, y, 15, 15, QColor(255, 255, 255));
            painter.fillRect(x + 15, y, 15, 15, QColor(192, 192, 192));
            painter.fillRect(x, y + 15, 15, 15, QColor(192, 192, 192));
            painter.fillRect(x + 15, y + 15, 15, 15, QColor(255, 255, 255));
        }
    }
    // Draw gradient
    painter.setPen(QColor(0, 0, 0));
    QLinearGradient gradient(1., 1., w - 1., h - 1.);
    gradient.setStops(preset.gradient);
    painter.setBrush(gradient);
    painter.drawRect(0, 0, w - 1, h - 1);
    return QPixmap::fromImage(image);
}

void GradientDialog::updatePresets(){
    ui->presetList->clear();
    for(int row = 0; row < presets->rowCount(); row++){
        const QString key = presets->keyAt(row);
        const Preset preset = presets->preset(key);
        QListWidgetItem* item = new QListWidgetItem(makeIcon(preset), preset.name, ui->presetList);
        if(currentPreset == key)
            item->setSelected(true);
        ui->presetList->addItem(item);
    }
}

void GradientDialog::currentChanged(const QColor& color, bool spontaneous, int position){
    int pCount = ui->widget->stopCount();
    if(pCount == 1)
        ui->stopBox->setTitle(tr("1 point"));
    else
        ui->stopBox->setTitle(tr("%1 points").arg(pCount));
    if(!color.isValid()){
        ui->stopBox->setEnabled(false);
        ui->stopPos->setValue(0);
        ui->stopColor->color(QColor(0, 0, 0));
        ui->stopOpacity->setValue(0);
    } else {
        ui->stopBox->setEnabled(true);
        ui->stopPos->setValue(position);
        ui->stopColor->color(color);
        ui->stopOpacity->setValue(round(color.alphaF() * 100.f));
    }
    // Un-set current preset
    if(spontaneous)
        setPreset("");
    setFocus();
}

void GradientDialog::setPreset(const QString &newPreset){
    if(currentPreset == newPreset)
        return;
    if(newPreset == ""){
        ui->presetList->clearSelection();
        ui->presetList->setCurrentItem(nullptr);
        Preset current = presets->preset(currentPreset);
        QString curName = ui->presetName->text().trimmed();
        if(curName == "" || (curName == current.name && current.builtIn))
            ui->presetName->setText(tr("Custom"));
        ui->presetDelete->setEnabled(false);
        ui->presetSave->setEnabled(true);
    } else {
        Preset current = presets->preset(newPreset);
        ui->presetName->setText(current.name);
        ui->presetDelete->setEnabled(!current.builtIn);
        ui->presetSave->setEnabled(false);
        ui->widget->setStops(current.gradient);
    }
    currentPreset = newPreset;
    setFocus();
}

void GradientDialog::on_presetList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous){
    setPreset(current ? current->text().toLower() : "");
}

void GradientDialog::on_presetName_textEdited(const QString &arg1){
    if(currentPreset != "" && arg1.trimmed() != presets->preset(currentPreset).name)
        setPreset("");
}

void GradientDialog::on_presetSave_clicked(){
    if(currentPreset != "")
        return;
    QString name = ui->presetName->text().trimmed();
    if(name == "")
        ui->presetName->setText(name = tr("Custom"));
    QString pName = GradientPresetModel::key(name);
    // Make sure not to overwrite a built-in preset
    Preset previous = presets->preset(pName);
    if(previous.builtIn){
        QMessageBox::warning(this, tr("Error"), tr("Can't overwrite a built-in preset. Please choose a different name."));
        setFocus();
        return;
    } else if(previous.name != ""){
        // Warn when overwriting an existing preset
        if(QMessageBox::warning(this, tr("Warning"), tr("Preset \"%1\" already exists. Replace?").arg(name), QMessageBox::StandardButtons(QMessageBox::Save | QMessageBox::Cancel)) != QMessageBox::Save){
            setFocus();
            return;
        }
    }
    currentPreset = presets->setPreset(name, ui->widget->stops());
    ui->presetDelete->setEnabled(true);
    ui->presetSave->setEnabled(false);
    updatePresets();
    setFocus();
}

void GradientDialog::on_presetDelete_clicked(){
    Preset current = presets->preset(currentPreset);
    if(current.name == "" || current.builtIn)
        return;
    if(QMessageBox::warning(this, tr("Warning"), tr("Delete preset \"%1\"?").arg(current.name), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No)) != QMessageBox::Yes){
        setFocus();
        return;
    }
    presets->removePreset(currentPreset);
    currentPreset = "";
    ui->presetDelete->setEnabled(false);
    ui->presetSave->setEnabled(true);
    updatePresets();
    setFocus();
}

void GradientDialog::on_stopPos_valueChanged(int arg1){
    int res = ui->widget->moveCurrent(arg1);
    if(res != arg1)
        ui->stopPos->setValue(res);
    setPreset("");
}

void GradientDialog::colorChanged(QColor color){
    color.setAlphaF(ui->stopOpacity->value() / 100.f);
    ui->widget->setCurrentColor(color);
    setPreset("");
}

void GradientDialog::on_stopOpacity_valueChanged(int arg1){
    colorChanged(ui->stopColor->color());
    setPreset("");
}
