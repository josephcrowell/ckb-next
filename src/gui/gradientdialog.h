#ifndef GRADIENTDIALOG_H
#define GRADIENTDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <QMap>
#include "gradientmodel.h"

namespace Ui {
class GradientDialog;
}

class GradientDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GradientDialog(QWidget *parent = nullptr);
    ~GradientDialog();

    QGradientStops getGradient(const QGradientStops& prevGradient);
private:
    typedef GradientPresetModel::Preset Preset;
    // Preset gradients, sorted by lower case name
    GradientPresetModel*    presets;
    QString                 currentPreset;

    QIcon       makeIcon(const Preset& preset);
    void        updatePresets();
    void        setPreset(const QString& newPreset);

    Ui::GradientDialog *ui;

private slots:
    void currentChanged(const QColor& color, bool spontaneous, int position);
    void colorChanged(QColor color);

    void on_presetList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void on_presetName_textEdited(const QString &arg1);
    void on_presetSave_clicked();
    void on_presetDelete_clicked();
    void on_stopPos_valueChanged(int arg1);
    void on_stopOpacity_valueChanged(int arg1);
};

#endif // GRADIENTDIALOG_H
