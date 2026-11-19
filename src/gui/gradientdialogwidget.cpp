#include <cmath>

#include <QtGlobal>
#include <QPainter>
#include <QPropertyAnimation>
#include <QVector>

#include "gradientdialogwidget.h"

GradientDialogWidget::GradientDialogWidget(QWidget *parent) :
    QWidget(parent), model(new GradientModel(this)), selectedPos(-1)
{
    setMouseTracking(true);
    connect(model, &GradientModel::currentChanged, this, &GradientDialogWidget::currentChanged);
    connect(model, &GradientModel::stopsChanged, this, QOverload<>::of(&QWidget::update));
}

QRect GradientDialogWidget::fillRect(){
    int x = 8, y = 1;
    int w = (width() - x * 2) / 16 * 16, h = (height() - 24) / 16 * 16;
    return QRect(x, y, w, h);
}

QRect GradientDialogWidget::stopRect(int pos){
    QRect fill = fillRect();
    int sx = round(pos / 100. * fill.width()) + fill.x() - 6;
    int sw = 12, sh = 18;
    int sy = height() - sh - 2;
    return QRect(sx, sy, sw, sh);
}

int GradientDialogWidget::selectedStop(){
    QRect rect = fillRect();
    int res = round((selectedPos - rect.x()) * 100. / rect.width());
    if(res < 0 || res > 100)
        return -1;
    return res;
}

void GradientDialogWidget::paintEvent(QPaintEvent*){
    QPainter painter(this);
    QRect fill = fillRect();
    int x = fill.x(), y = fill.y(), w = fill.width(), h = fill.height();

    // Draw border around gradient
    painter.setPen(palette().color(QPalette::Dark));
    painter.drawLine(x - 1, y - 1, x + w, y - 1);
    painter.drawLine(x - 1, y - 1, x - 1, y + h);
    painter.setPen(palette().color(QPalette::Light));
    painter.drawLine(x - 1, y + h, x + w, y + h);
    painter.drawLine(x + w, y - 1, x + w, y + h);
    // Draw background grid
    for(int i = 0; i < w; i += 16){
        for(int iy = 0; iy < h; iy += 16){
            if(i % 32 != iy % 32)
                painter.fillRect(x + i, y + iy, 16, 16, QColor(192, 192, 192));
            else
                painter.fillRect(x + i, y + iy, 16, 16, QColor(255, 255, 255));
        }
    }
    // Draw gradient
    painter.setRenderHint(QPainter::Antialiasing);
    QLinearGradient gradient(x, 0, x + w, 0);
    gradient.setStops(model->stops());
    painter.fillRect(fill, QBrush(gradient));

    // Draw stops
    QMapIterator<int, QColor> i(model->colors());
    painter.setPen(palette().color(QPalette::Shadow));
    while(i.hasNext()){
        i.next();
        painter.setBrush(QBrush(i.value()));
        int index = i.key();
        if(index == model->current()){
            // Highlight currently-edited stop
            painter.setPen(QPen(palette().color(QPalette::Highlight), 2));
            painter.drawRoundedRect(stopRect(index), 4, 4);
            painter.setPen(palette().color(QPalette::Shadow));
        } else
            painter.drawRoundedRect(stopRect(index), 4, 4);
    }
    // Draw selection (if any)
    if(model->dragging() && selectedPos >= x && selectedPos <= x + w){
        painter.setPen(QPen(palette().color(QPalette::Highlight), 2.5));
        painter.setBrush(model->draggedColor());
        painter.drawRoundedRect(selectedPos - 6, height() - 18 - 2, 12, 18, 4, 4);
    }
}

void GradientDialogWidget::mousePressEvent(QMouseEvent* event){
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    int x = event->position().x(), y = event->position().y();
#else
    int x = event->x(), y = event->y();
#endif

    if(y < height() - 24)
        return;
    QMapIterator<int, QColor> i(model->colors());
    while(i.hasNext()){
        i.next();
        // If a point was selected, remove it from the color map and set it as the selected point
        QRect stop = stopRect(i.key());
        if(x >= stop.left() - 1 && x <= stop.right() + 1){
            selectedPos = stop.x() + stop.width() / 2;
            selectedOffset = selectedPos - x;
            model->pickStop(i.key());
            // On right click, delete the point
            if(event->button() == Qt::RightButton && model->colors().count() > 1){
                selectedPos = -1;
                mouseReleaseEvent(event);
            } else
                update();
            return;
        }
    }
    // If nothing was selected, create a new point
    QRect fill = fillRect();
    if(x > fill.left() && x < fill.right() && event->button() != Qt::RightButton){
        selectedPos = x;
        selectedOffset = 0;
        model->newStop(selectedStop());
    }
}

void GradientDialogWidget::mouseMoveEvent(QMouseEvent* event){
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint evtpos(event->x(), event->y());
#else
    QPoint evtpos = event->position().toPoint();
#endif
    if(!model->dragging()){
        if(evtpos.y() >= height() - 24)
            setCursor(QCursor(Qt::PointingHandCursor));
        else
            setCursor(QCursor(Qt::ArrowCursor));
        return;
    }
    // Move selected point (if any)
    setCursor(QCursor(Qt::ClosedHandCursor));
    selectedPos = evtpos.x() + selectedOffset;
    // Allow the point to be deleted as long as there is at least 1 other point
    bool last = (model->colors().count() == 0);
    // Hug the left/right edges
    QRect rect = fillRect();
    int left = rect.left(), right = rect.right();
    if(selectedPos < left && (last || selectedPos >= left - 30))
        selectedPos = left;
    else if(selectedPos > right && (last || selectedPos <= right + 30))
        selectedPos = right;
    // Remove point if it's too far above/below the widget
    int top = -30, bottom = height() + 30;
    if(!last && (evtpos.y() < top || evtpos.y() > bottom))
        selectedPos = -1;
    model->moveDragged(selectedStop());
}

void GradientDialogWidget::mouseReleaseEvent(QMouseEvent* event){
    if(model->dragging()){
        // Rejoin selection with gradient
        model->dropDragged(selectedStop());
        selectedPos = -1;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const int y = event->y();
#else
    const int y = event->position().y();
#endif
    if(y >= height() - 24)
        setCursor(QCursor(Qt::PointingHandCursor));
    else
        setCursor(QCursor(Qt::ArrowCursor));
}
