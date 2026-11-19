#ifndef GRADIENTMODEL_H
#define GRADIENTMODEL_H

#include <QAbstractListModel>
#include <QColor>
#include <QGradientStops>
#include <QMap>
#include <QObject>

// UI-independent state of the gradient editor, shared by the Widgets and QML UIs.
// Points are at whole percentages (0-100). While a point is dragged it is taken out of the
// points and kept as the dragged colour, at a position the UI passes in (-1: off the bar).
class GradientModel : public QObject
{
    Q_OBJECT
public:
    explicit GradientModel(QObject* parent = nullptr);

    // "pos:aarrggbb pos:aarrggbb ..." as stored in animation parameters
    static QGradientStops fromString(const QString& string, bool alpha);
    static QString toString(const QGradientStops& stops);

    void setStops(const QGradientStops& stops);
    inline QGradientStops stops() const { return _stops; }
    inline int stopCount() const { return _stops.count(); }
    // Editable points by position
    inline const QMap<int, QColor>& colors() const { return _colors; }

    QColor colorAt(int position) const;

    inline int current() const { return _current; }
    int moveCurrent(int to);
    void setCurrentColor(const QColor& color);

    inline bool dragging() const { return _selected.isValid(); }
    inline const QColor& draggedColor() const { return _selected; }
    // Takes the point at the position out to drag it
    void pickStop(int position);
    // Starts dragging a new point with the colour the gradient has there
    void newStop(int position);
    void moveDragged(int position);
    // Puts the dragged point back (dropped with -1, it is deleted) and makes it current
    void dropDragged(int position);

signals:
    void currentChanged(QColor color, bool spontaneous, int position);
    void stopsChanged();

private:
    QColor  _selected;
    int     _selectedStop;
    int     _current;
    QMap<int, QColor>   _colors;
    QGradientStops      _stops;

    QMap<int, QColor> selectionColors() const;
    void makeStops();
};

// Gradient presets: the built-in ones and the user's (stored in "Stored Gradients"),
// sorted by lower case name, which is also the key
class GradientPresetModel : public QAbstractListModel
{
    Q_OBJECT
public:
    struct Preset {
        QString         name;
        QGradientStops  gradient;
        bool            builtIn;
        inline Preset(QString _name, bool _builtIn = false)             : name(_name), builtIn(_builtIn) {}
        inline Preset(QString _name, const QGradientStops& _gradient)   : name(_name), gradient(_gradient), builtIn(false) {}
        inline Preset()                                                 : builtIn(false) {}
    };

    enum Roles {
        NameRole = Qt::UserRole + 1,
        KeyRole,
        BuiltInRole,
        StopsRole,      // list of {position, color}
    };

    explicit GradientPresetModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    static inline QString key(const QString& name) { return name.toLower(); }
    QString keyAt(int row) const;
    // A default (empty name) preset when there is none with that key
    Preset preset(const QString& key) const;

    // Built-in presets plus the stored ones
    void load();
    // Writes the user's presets to the settings
    void store() const;
    // Adds or replaces a user preset; returns its key
    QString setPreset(const QString& name, const QGradientStops& gradient);
    void removePreset(const QString& key);

private:
    QMap<QString, Preset> _presets;

    inline void addPreset(const Preset& preset) { _presets[key(preset.name)] = preset; }
};

#endif // GRADIENTMODEL_H
