#ifndef LIGHTCONTROLLER_H
#define LIGHTCONTROLLER_H

#include <QColor>
#include <QObject>
#include <QSet>
#include <QStringList>
#include "colormap.h"

class AnimationListModel;
class AnimScript;
class KbAnim;
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

    // Animations of the current light. Only these methods (and KbLight::load) change the list.
    inline AnimationListModel* animations() const { return _animations; }
    // "Default" presets use the script name; others get the script name appended unless equal
    static QString animationName(const AnimScript* script, int presetId);
    KbAnim* addAnimation(const AnimScript* script, int presetId, const QStringList& keys);
    KbAnim* duplicateAnimation(KbAnim* anim);
    // Removes and deletes (later) the animation; doesn't restart the others
    void deleteAnimation(KbAnim* anim);
    void reorderAnimations(const QList<KbAnim*>& order);
    // Returns false and leaves the name unchanged when the name is empty; trims it otherwise
    bool renameAnimation(KbAnim* anim, const QString& name);
    void setAnimationOpacity(KbAnim* anim, double percent);
    void setAnimationBlendMode(KbAnim* anim, int mode);
    // Keeps keys the animation had that aren't in the current layout (e.g. bslash_iso and bslash)
    void setAnimationKeys(KbAnim* anim, const QStringList& keys);
    // After the settings dialog: commit, re-init and restart, or reset the parameters
    void finishAnimationSettings(KbAnim* anim, bool accepted);
    void restartAnimation();

signals:
    // The light loaded its settings again (KbLight::didLoad); the animation list was reset
    void animationsReloaded();
    // Forwarded KbLight::updated
    void lightUpdated();
    // Key colours changed through this controller (KbLight::color doesn't signal it)
    void colorsChanged();
    // Animated colours to preview; an empty map clears the preview
    void previewFrame(const ColorMap& animatedColors, const QSet<QString>& indicators, quint64 renderInterval);

private slots:
    void applicationStateChanged(Qt::ApplicationState state);
    void reloadAnimations();

private:
    void applyShowAnimated(bool show);
    void startAnimationPreview();
    void stopAnimationPreview();

    KbLight* _light;
    AnimationListModel* _animations;
    QStringList _selection;
    QColor _selectedColor;
    bool _showAnimated;
};

#endif // LIGHTCONTROLLER_H
