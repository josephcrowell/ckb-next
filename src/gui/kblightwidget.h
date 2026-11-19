#ifndef KBLIGHTWIDGET_H
#define KBLIGHTWIDGET_H

#include <QWidget>
#include <QColor>
#include <QFile>
#include <QResizeEvent>
#include "kblight.h"
#include "keywidget.h"

class LightController;

namespace Ui {
class KbLightWidget;
}

class KbLightWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KbLightWidget(QWidget *parent = nullptr);
    ~KbLightWidget();

    void setLight(KbLight* newLight);
    void setMonochrome();
    void setLegacyM95();

private slots:
    void updateLight();
    void updateColors();
    void newSelection(const QStringList& selection);
    void changeAnim(KbAnim* newAnim);
    void changeAnimKeys(const QStringList& keys);

    void on_brightnessBox_activated(int index);
    void on_animButton_clicked();

    void on_bgButton_clicked();

    void on_showAnimBox_clicked(bool checked);

private:
    LightController* controller;

    Ui::KbLightWidget *ui;
    KeyWidget* keyWidget;
};

#endif // KBLIGHTWIDGET_H
