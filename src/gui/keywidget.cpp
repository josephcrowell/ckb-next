#include <QPainter>
#include "keywidget.h"
#include <QToolTip>
#include <limits>
#include <ckbnextconfig.h>

#ifdef FPS_COUNTER
static const QColor blue(128, 224, 255);
static const QColor green(160, 255, 168);
#endif

KeyWidget::KeyWidget(QWidget* parent) :
    QOpenGLWidget(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_AlwaysStackOnTop);
    renderer.setSize(size());
#ifdef FPS_COUNTER
    glFpsTimer.start();
    kbLoopElapsed = 0.0;
#endif
}

void KeyWidget::map(const KeyMap& newMap){
    renderer.setMap(newMap);
    update();
    updateGeometry();
}

bool KeyWidget::event(QEvent* e){
    if(e->type() != QEvent::ToolTip)
        return QOpenGLWidget::event(e);

    QHelpEvent* he = static_cast<QHelpEvent*>(e);
    const QString name = renderer.keyNameAt(he->pos());
    if(!name.isEmpty()){
        QToolTip::showText(he->globalPos(), name);
        return true;
    }
    // No key was found under the cursor
    QToolTip::hideText();
    e->ignore();
    return true;
}

void KeyWidget::setDebug(bool debug){
#ifndef NDEBUG
    renderer.setDebug(debug);
    update();
#else
    Q_UNUSED(debug);
#endif
}

void KeyWidget::colorMap(const QColorMap& newColorMap){
    renderer.setColorMap(newColorMap);
    update();
}

void KeyWidget::displayColorMap(const ColorMap& newDisplayMap, const QSet<QString>& indicators, quint64 renderInterval){
    if(!isVisible())
        return;
    renderer.setDisplayColorMap(newDisplayMap, indicators);
#ifdef FPS_COUNTER
    if(renderInterval != std::numeric_limits<quint64>::max())
        kbLoopElapsed = renderInterval;
#endif
    update();
}

void KeyWidget::bindMap(const BindMap& newBindMap){
    renderer.setBindMap(newBindMap);
    update();
}

void KeyWidget::paintGL(){
    // Don't bother painting anything if the scale is 0
    if(!renderer.canPaint())
        return;

    QPainter painter(this);

    // Clear everything
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderer.paint(&painter);

#ifdef FPS_COUNTER
    painter.setPen(QPen(green, 1.0));
    QFont fpsfont = painter.font();
    fpsfont.setBold(true);
    fpsfont.setPointSize(14);
    painter.setFont(fpsfont);
    painter.drawText(5, 18, QString::number(1.0/((double)glFpsTimer.restart()/1000.0), 'f', 2));
    painter.setPen(QPen(blue, 1.0));
    if(kbLoopElapsed > 0.0)
        painter.drawText(5, 36, QString::number(1.0/(kbLoopElapsed/1000.0), 'f', 2));
#endif
}

void KeyWidget::initializeGL()
{
    glClearColor(0.f, 0.f, 0.f, 0.f);
    QOpenGLWidget::initializeGL();
}

void KeyWidget::paintEvent(QPaintEvent* e){
    QOpenGLWidget::paintEvent(e);
}

static inline QPointF eventPos(const QMouseEvent* event){
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return event->localPos();
#else
    return event->position();
#endif
}

void KeyWidget::mousePressEvent(QMouseEvent* event){
    event->accept();
    switch(renderer.press(eventPos(event), event->modifiers())){
    case KeyRenderer::PRESS_SIDELIGHT_TOGGLED:
        emit sidelightToggled(); // get the kblightwidget to record it
        update();
        break;
    case KeyRenderer::PRESS_M95_LIGHT_TOGGLED:
        emit M95LightToggled();
        update();
        break;
    case KeyRenderer::PRESS_SELECTED:
        update();
        break;
    case KeyRenderer::PRESS_MISSED:
        break;
    }
}

void KeyWidget::mouseMoveEvent(QMouseEvent* event){
    event->accept();
    if(renderer.move(eventPos(event)))
        update();
}

void KeyWidget::mouseReleaseEvent(QMouseEvent* event){
    event->accept();
    QStringList selectedNames;
    if(!renderer.release(selectedNames))
        return;
    // Emit signal with the names of the keys
    emit selectionChanged(selectedNames);
    update();
}

void KeyWidget::setSelection(const QStringList& keys){
    renderer.setSelection(keys);
    update();
    emit selectionChanged(keys);
}

void KeyWidget::selectAll(){
    const QStringList selectedNames = renderer.selectAll();
    update();
    emit selectionChanged(selectedNames);
}

void KeyWidget::clearSelection(){
    renderer.clearSelection();
    update();
    emit selectionChanged(QStringList());
}

void KeyWidget::setAnimation(const QStringList& keys){
    renderer.setAnimation(keys);
    update();
}

void KeyWidget::setAnimationToSelection(){
    renderer.setAnimationToSelection();
    update();
}

void KeyWidget::clearAnimation(){
    renderer.clearAnimation();
    update();
}
