#ifndef KPERFWIDGET_H
#define KPERFWIDGET_H

#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include "kbperf.h"
#include "kbprofile.h"
#include "colorbutton.h"

class PerfController;

namespace Ui {
class KPerfWidget;
}

class KPerfWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KPerfWidget(QWidget *parent = nullptr);
    ~KPerfWidget();

    void setPerf(KbPerf* newPerf, KbProfile* newProfile);

    static const int I_COUNT = KbPerf::I_COUNT;
    static const int HW_I_COUNT = KbPerf::HW_I_COUNT;
    typedef KbPerf::indicator indicator;
    typedef KbPerf::i_hw i_hw;

private:
    Ui::KPerfWidget *ui;

    PerfController* controller;

    struct IndicatorUi {
        QCheckBox* enable;
        QComboBox* hwEnable;
        ColorButton* color1, *color2, *color3;
    };
    IndicatorUi indicators[I_COUNT];
    QList<QWidget*> k95Widgets;

private slots:
    void uiUpdated(int index);
    void on_intensityBox_valueChanged(int arg1);
    void on_copyButton_clicked();
};

#endif // KPERFWIDGET_H
