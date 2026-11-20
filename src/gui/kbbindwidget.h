#ifndef KBBINDWIDGET_H
#define KBBINDWIDGET_H

#include <QWidget>
#include "kbbind.h"
#include "kbprofile.h"
#include "keywidget.h"

class BindController;

namespace Ui {
class KbBindWidget;
}

class KbBindWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KbBindWidget(QWidget *parent = nullptr);
    ~KbBindWidget();

    void setBind(KbBind* newBind, KbProfile* newProfile);
    void setControlsEnabled(const bool e);

private slots:
    void updateBind();
    void newLayout();
    void newSelection(const QStringList& selection);
    void updateSelDisplay();

    void on_resetButton_clicked();
    void on_copyButton_clicked();

private:
    Ui::KbBindWidget *ui;

    BindController* controller;
    KeyWidget* keyWidget;
};

#endif // KBBINDWIDGET_H
