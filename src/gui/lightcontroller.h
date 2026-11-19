#ifndef LIGHTCONTROLLER_H
#define LIGHTCONTROLLER_H

#include <QColor>
#include <QObject>
#include <QSet>
#include <QStringList>
#include "colormap.h"

class KbLight;

// UI-independent logic of the lighting page, shared by the Widgets and QML UIs
class LightController : public QObject
{
    Q_OBJECT
public:
    explicit LightController(QObject* parent = nullptr);

    inline KbLight* light() const { return _light; }
    // Stops the animation preview of the previous light and starts it for the new one if shown
    void setLight(KbLight* newLight);

    // Returns false, leaving the selection unchanged, when there is no light
    bool setSelection(const QStringList& selection);
    inline const QStringList& selection() const { return _selection; }
    // Colour shared by all selected keys; invalid if there is no selection or they differ
    inline const QColor& selectedColor() const { return _selectedColor; }
    QString selectionText() const;

    void setColor(const QColor& newColor);
    void toggleSidelight();
    void toggleM95Light();

    int brightness() const;
    void setBrightness(int index);

    inline bool showAnimated() const { return _showAnimated; }
    void setShowAnimated(bool show);

signals:
    // Forwarded KbLight::updated
    void lightUpdated();
    // Key colours changed through this controller (KbLight::color doesn't signal it)
    void colorsChanged();
    // Animated colours to preview; an empty map clears the preview
    void previewFrame(const ColorMap& animatedColors, const QSet<QString>& indicators, quint64 renderInterval);

private slots:
    void applicationStateChanged(Qt::ApplicationState state);

private:
    void applyShowAnimated(bool show);
    void startAnimationPreview();
    void stopAnimationPreview();

    KbLight* _light;
    QStringList _selection;
    QColor _selectedColor;
    bool _showAnimated;
};

#endif // LIGHTCONTROLLER_H
