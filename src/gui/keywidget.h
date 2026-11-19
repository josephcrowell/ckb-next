#ifndef RGBWIDGET_H
#define RGBWIDGET_H

#include <QMouseEvent>
#include <QPaintEvent>
#include <QOpenGLWidget>
#include "keymap.h"
#include "colormap.h"
#include "keyrenderer.h"
#include <cmath>
#include <ckbnextconfig.h>
#include <QElapsedTimer>

// Widgets UI for KeyRenderer: GL surface, mouse, tooltips and signals
class KeyWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    // New key widget. rgbMode = true to display colors, false to display key names
    KeyWidget(QWidget* parent);
    inline bool     rgbMode()                   { return renderer.rgbMode(); }
    inline void     rgbMode(bool newRgbMode)    { renderer.setRgbMode(newRgbMode); update(); }
    // For RGB maps, monochrome = true to covert everything to grayscale
    inline bool     monochrome()                { return renderer.monochrome(); }
    inline void     monochrome(bool newMono)    { renderer.setMonochrome(newMono); update(); }

    // Key map
    const KeyMap&       map() const                         { return renderer.map(); }
    void                map(const KeyMap& newMap);
    // Key -> color map (must contain exactly the keys in the key map)

    const QColorMap&    colorMap() const                    { return renderer.colorMap(); }
    void                colorMap(const QColorMap& newColorMap);
    // Key -> binding map
    typedef KeyRenderer::BindMap BindMap;
    const BindMap&      bindMap() const                     { return renderer.bindMap(); }
    void                bindMap(const BindMap& newBindMap);

    // Set current selection (highlighted in blue)
    void setSelection(const QStringList& keys);
    void selectAll();
    void clearSelection();

    // Set animated keys (highlighted in green)
    void setAnimation(const QStringList& keys);
    void setAnimationToSelection();
    void clearAnimation();

    bool hasHeightForWidth() const override { return true; }
    inline float aspectRatio() const {
        return renderer.aspectRatio();
    }
    int heightForWidth(int w) const override {
        return std::round(w / aspectRatio());
    }
    void setDebug(bool debug);

public slots:
    // Sets display colors. Pass an empty map to clear.
    // These will be displayed instead of the regular color map, if supplied.
    void displayColorMap(const ColorMap& newDisplayMap, const QSet<QString>& indicators, quint64 renderInterval);

signals:
    // Emitted when the selection is changed.
    void selectionChanged(QStringList selected);
    void sidelightToggled();
    void M95LightToggled();

private:
    KeyRenderer renderer;

    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void paintGL() override;
    void initializeGL() override;
    void resizeEvent(QResizeEvent* event) override {
        renderer.setSize(event->size());
        QOpenGLWidget::resizeEvent(event);
    }
    bool event(QEvent* e) override;
#ifdef FPS_COUNTER
    QElapsedTimer glFpsTimer;
    double kbLoopElapsed;
#endif
};

#endif // RGBWIDGET_H
