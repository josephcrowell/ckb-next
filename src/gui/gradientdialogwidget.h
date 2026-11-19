#ifndef GRADIENTDIALOGWIDGET_H
#define GRADIENTDIALOGWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QPaintEvent>
#include "gradientmodel.h"

class GradientDialogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GradientDialogWidget(QWidget *parent = nullptr);

    inline void             setStops(const QGradientStops& stops)   { model->setStops(stops); }
    inline QGradientStops   stops()                                 { return model->stops(); }
    inline int              stopCount()                             { return model->stopCount(); }

    inline QColor colorAt(int position)             { return model->colorAt(position); }

    inline int  current()                           { return model->current(); }
    inline int  moveCurrent(int to)                 { return model->moveCurrent(to); }
    inline void setCurrentColor(const QColor& color){ model->setCurrentColor(color); }

signals:
    void currentChanged(QColor color, bool spontaneous, int position);

private:
    GradientModel* model;
    // Dragged point's position and grab offset in pixels
    int     selectedPos, selectedOffset;

    QRect fillRect();
    QRect stopRect(int pos);
    int selectedStop();

    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
};

#endif // GRADIENTDIALOGWIDGET_H
