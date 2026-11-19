#include <QPainter>
#include "gradientbutton.h"
#include "gradientdialog.h"
#include "gradientmodel.h"

GradientButton::GradientButton(QWidget* parent, bool allowAlpha) :
    QPushButton(parent), _alpha(allowAlpha)
{
    connect(this, SIGNAL(clicked()), this, SLOT(pickGradient()));
    setAutoDefault(false);
    setDefault(false);
    fromString("");
    setText("");
}

void GradientButton::updateImage(){
    const int w = 130, h = 16;
    QImage image(w, h, QImage::Format_RGB888);
    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    painter.fillRect(0, 0, w, h, QColor(0, 0, 0));
    if(_alpha){
        for(int x = 1; x < w - 1; x += h){
            painter.fillRect(x, 1, h / 2, h / 2 - 1, QColor(255, 255, 255));
            painter.fillRect(x + h / 2, 1, h / 2, h / 2 - 1, QColor(192, 192, 192));
            painter.fillRect(x, h / 2, h / 2, h / 2 - 1, QColor(192, 192, 192));
            painter.fillRect(x + h / 2, h / 2, h / 2, h / 2 - 1, QColor(255, 255, 255));
        }
    }
    QLinearGradient gradient(1., 0., w - 1, 0.);
    gradient.setStops(_stops);
    painter.fillRect(1, 1, w - 2, h - 2, QBrush(gradient));
    setIconSize(QSize(w, h));
    setIcon(QIcon(QPixmap::fromImage(image)));
}

void GradientButton::fromString(const QString& string){
    _stops = GradientModel::fromString(string, _alpha);
    updateImage();
}

QString GradientButton::toString() const {
    return GradientModel::toString(_stops);
}

void GradientButton::pickGradient(){
    GradientDialog dialog(this);
    _stops = dialog.getGradient(_stops);
    updateImage();
    emit gradientChanged();
}

