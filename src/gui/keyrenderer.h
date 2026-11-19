#ifndef KEYRENDERER_H
#define KEYRENDERER_H

#include <QBitArray>
#include <QHash>
#include <QImage>
#include <QPointF>
#include <QRectF>
#include <QSet>
#include <QSizeF>
#include <QStringList>
#include "colormap.h"
#include "keymap.h"

class QPainter;

// Draws a device's keys and keeps the state shown with them (colours, bindings, selection,
// animated keys, mouse selection), without depending on a widget or a GL context.
// Shared by KeyWidget (Widgets UI) and the QML key item.
class KeyRenderer
{
public:
    KeyRenderer();

    // true to display colours, false to display key names
    inline bool rgbMode() const                 { return _rgbMode; }
    inline void setRgbMode(bool rgbMode)        { _rgbMode = rgbMode; }
    // For RGB maps, true to convert everything to grayscale
    inline bool monochrome() const              { return _monochrome; }
    inline void setMonochrome(bool monochrome)  { _monochrome = monochrome; }

    // Key map; resets the selection and animated keys
    inline const KeyMap& map() const            { return keyMap; }
    void setMap(const KeyMap& newMap);
    inline float aspectRatio() const            { return _aspectRatio; }

    // Key -> colour map (must contain exactly the keys in the key map)
    inline const QColorMap& colorMap() const    { return _colorMap; }
    inline void setColorMap(const QColorMap& newColorMap) { _colorMap = newColorMap; }
    // Key -> binding map
    typedef QHash<QString, QString> BindMap;
    inline const BindMap& bindMap() const       { return _bindMap; }
    inline void setBindMap(const BindMap& newBindMap) { _bindMap = newBindMap; }
    // Colours shown instead of the colour map (an empty map clears them), and indicator keys
    void setDisplayColorMap(const ColorMap& newDisplayMap, const QSet<QString>& indicators);

    // Selection (highlighted in blue)
    void setSelection(const QStringList& keys);
    // Returns the names of the selected keys
    QStringList selectAll();
    void clearSelection();

    // Animated keys (highlighted in green)
    void setAnimation(const QStringList& keys);
    void setAnimationToSelection();
    void clearAnimation();

    // Hitbox overlay, only drawn in debug builds
    void setDebug(bool debug);

    // Size of the area painted, in device-independent pixels
    void setSize(const QSizeF& size);
    // false while the size is too small to draw anything
    inline bool canPaint() const                { return drawInfoScale > 0.f; }
    // Draws everything; the caller clears the background first if needed
    void paint(QPainter* painter) const;

    // Mouse selection. Modifiers pick the mode: Alt subtracts, Shift adds, Ctrl toggles,
    // none replaces the selection.
    enum PressResult {
        PRESS_MISSED,
        PRESS_SELECTED,
        PRESS_SIDELIGHT_TOGGLED,
        PRESS_M95_LIGHT_TOGGLED,
    };
    PressResult press(const QPointF& pos, Qt::KeyboardModifiers modifiers);
    // Returns true if the highlight changed
    bool move(const QPointF& pos);
    // Applies the selection and returns true, with the selected key names in `selected`;
    // returns false if no press is in progress
    bool release(QStringList& selected);

    // Friendly name of the key under pos, or an empty string
    QString keyNameAt(const QPointF& pos) const;

private:
    KeyMap keyMap;
    QColorMap _colorMap;
    ColorMap _displayColorMap;
    BindMap _bindMap;
    QSet<QString> _indicators;

    QBitArray selection;
    QBitArray newSelection;
    QBitArray animation;
    // These should not be scaled so that the rect renders correctly
    QPointF mouseDown;
    QRectF mouseHighlightRect;
    enum {
        NONE,
        SET,
        ADD,
        SUBTRACT,
        TOGGLE,
    } mouseDownMode;
    bool _rgbMode, _monochrome;

    float _aspectRatio;

    QImage _currentOverlay, _currentOverlayScaled;
    QPointF _overlayPos;

    QSizeF _size;
    QPointF drawInfoOffset;
    float drawInfoScale;

    // Helper functions for rendering keys
    void drawLogo(const Key* key, QPainter* decPainter) const;
    void drawBottomRightCorner(QPainter* painter, float x, float y, float w, float h, float scale) const;
    void drawBottomLeftCorner(QPainter* painter, float x, float y, float w, float h, float scale) const;
    void drawTopRightCorner(QPainter* painter, float x, float y, float w, float h, float scale) const;
    void drawTopLeftCorner(QPainter* painter, float x, float y, float w, float h, float scale) const;
    void drawStrafeSidelights(const Key* key, QPainter* painter, const QColor& kC, const QColor& c, const QColor& bgC) const;
    void calculateDrawInfo();
    bool _debug;
#ifndef NDEBUG
    QHash<const char*, QRectF> hitboxes;
#endif
};

#endif // KEYRENDERER_H
