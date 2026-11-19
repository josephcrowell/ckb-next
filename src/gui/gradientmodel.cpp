#include "gradientmodel.h"
#include <cmath>
#include <cstdio>
#include <QTextStream>
#include <QVector>
#include "ckbsettings.h"

static const QString prefsPath = "Stored Gradients";

GradientModel::GradientModel(QObject* parent) :
    QObject(parent), _selectedStop(-1), _current(-1)
{
}

QGradientStops GradientModel::fromString(const QString& string, bool alpha){
    QGradientStops stops;
    // Parse string with sscanf
    QByteArray cString = string.toLatin1();
    const char* dataStr = cString.data();
    char pos = -1;
    uchar a, r, g, b;
    while(1){
        int scanned = 0;
        signed char newpos;
        if(sscanf(dataStr, "%hhd:%2hhx%2hhx%2hhx%2hhx%n", &newpos, &a, &r, &g, &b, &scanned) != 5)
            break;
        dataStr += scanned;
        // Don't allow stops out-of-order or past 100
        if(newpos <= pos || newpos > 100)
            break;
        pos = newpos;
        if(!alpha)
            a = 255;
        stops.append(QGradientStop(pos / 100., QColor(r, g, b, a)));
    }
    if(stops.count() == 0){
        // If nothing was read, try a single ARGB constant.
        if(sscanf(dataStr, "%2hhx%2hhx%2hhx%2hhx", &a, &r, &g, &b) == 4){
            stops.append(QGradientStop(0., QColor(r, g, b, a)));
            stops.append(QGradientStop(1., QColor(r, g, b, 0.)));
        }
    }
    // If that still didn't work, fill with white
    if(stops.count() == 0){
        stops.append(QGradientStop(0., QColor(255, 255, 255)));
        stops.append(QGradientStop(1., QColor(255, 255, 255)));
    }
    // Make sure there are stops at 0 and at 100
    if(stops[0].first != 0.)
        stops.prepend(QGradientStop(0., stops[0].second));
    if(stops.last().first != 1.)
        stops.prepend(QGradientStop(1., stops.last().second));
    return stops;
}

QString GradientModel::toString(const QGradientStops& stops){
    QStringList result;
    foreach(const QGradientStop& stop, stops){
        QString string;
        QTextStream out(&string);
        const QColor& color = stop.second;
        // "%d:%02x%02x%02x%02x"
        out << static_cast<int>(round(stop.first * 100.f)) << ':'
            << qSetFieldWidth(2) << qSetPadChar('0') <<
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
               hex
#else
               Qt::hex
#endif
            << color.alpha() << color.red() << color.green() << color.blue();
        result << string;
    }
    return result.join(" ");
}

void GradientModel::setStops(const QGradientStops& stops){
    _stops = stops;
    _colors.clear();
    foreach(const QGradientStop& stop, stops){
        double pos = stop.first;
        if(pos < 0. || pos > 1.)
            continue;
        _colors[round(pos * 100.)] = stop.second;
    }
    _current = 0;
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    emit currentChanged(_colors.first(), false, 0);
#else
    emit currentChanged(_colors.value(_colors.keys().first()), false, 0);
#endif
    emit stopsChanged();
}

void GradientModel::makeStops(){
    _stops.clear();
    QMap<int, QColor> colors = selectionColors();
    // If there's only a single color, put it at the beginning and end
    if(colors.count() == 1){
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
        QColor color = colors.first();
#else
        QColor color = colors.value(colors.keys().first());
#endif
        _stops.append(QGradientStop(0., color));
        _stops.append(QGradientStop(1., color));
        emit stopsChanged();
        return;
    }
    // Add colors at 0 and 100 if needed
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    if(!colors.contains(0))
        colors[0] = colors.first();
    if(!colors.contains(100))
        colors[100] = colors.last();
#else
    if(!colors.contains(0))
        colors[0] = colors.value(colors.keys().first());
    if(!colors.contains(100))
        colors[100] = colors.value(colors.keys().last());
#endif
    QMapIterator<int, QColor> i(colors);
    while(i.hasNext()){
        i.next();
        _stops.append(QGradientStop(i.key() / 100., i.value()));
    }
    emit stopsChanged();
}

QColor GradientModel::colorAt(int position) const {
    // Find which stops the position is between
    qreal pos = position / 100.;
    QVectorIterator<QGradientStop> i(_stops);
    QGradientStop previous = i.next();
    while(i.hasNext()){
        QGradientStop next = i.next();
        if(next.first >= pos){
            // Get color by linear interpolation. Premultiply the alpha value so that it returns the expected color
            // (i.e. stops with zero opacity won't contribute to color)
            QColor c1 = next.second, c2 = previous.second;
            qreal distance = next.first - previous.first;
            qreal dx = (pos - previous.first) / distance;
            qreal a1 = c1.alphaF(), a2 = c2.alphaF();
            qreal a3 = a1 * dx + a2 * (1.f - dx);
            if(a3 == 0.){
                c2.setAlpha(0);
                return c2;
            }
            qreal r = (c1.redF() * a1 * dx + c2.redF() * a2 * (1.f - dx)) / a3;
            qreal g = (c1.greenF() * a1 * dx + c2.greenF() * a2 * (1.f - dx)) / a3;
            qreal b = (c1.blueF() * a1 * dx + c2.blueF() * a2 * (1.f - dx)) / a3;
            return QColor::fromRgbF(r, g, b, a3);
        }
        previous = next;
    }
    return QColor();
}

int GradientModel::moveCurrent(int to){
    if(_current < 0 || to == _current || _selected.isValid())
        return _current;
    // If there's a point in the way, skip past it by continuing in the current direction
    if(_current < to){
        while(_colors.contains(to)){
            to++;
            if(to > 100)
                return _current;
        }
    } else {
        while(_colors.contains(to)){
            to--;
            if(to < 0)
                return _current;
        }
    }
    _colors[to] = _colors.take(_current);
    _current = to;
    makeStops();
    return to;
}

void GradientModel::setCurrentColor(const QColor& color){
    if(_current < 0 || _selected.isValid())
        return;
    QRgb rgb = _colors.value(_current).rgb();
    _colors[_current] = color;
    // If any points follow with the same color but different opacity, change them to match
    QMutableMapIterator<int, QColor> i(_colors);
    while(i.hasNext()){
        i.next();
        if(i.key() <= _current)
            continue;
        QColor& value = i.value();
        if(value.rgb() != rgb)
            break;
        int alpha = value.alpha();
        value = color;
        value.setAlpha(alpha);
    }
    makeStops();
}

QMap<int, QColor> GradientModel::selectionColors() const {
    if(!_selected.isValid())
        return _colors;
    // Re-add if in range
    QMap<int, QColor> res = _colors;
    if(_selectedStop >= 0 && _selectedStop <= 100)
        res[_selectedStop] = _selected;
    return res;
}

void GradientModel::pickStop(int position){
    _selected = _colors.take(position);
}

void GradientModel::newStop(int position){
    _selectedStop = position;
    _selected = colorAt(position);
    makeStops();
}

void GradientModel::moveDragged(int position){
    _selectedStop = position;
    makeStops();
}

void GradientModel::dropDragged(int position){
    if(!_selected.isValid())
        return;
    // Rejoin selection with gradient
    _selectedStop = position;
    _current = position;
    _colors = selectionColors();
    emit currentChanged(_colors.value(_current), true, _current);
    _selectedStop = -1;
    _selected = QColor();
    makeStops();
}

GradientPresetModel::GradientPresetModel(QObject* parent) :
    QAbstractListModel(parent)
{
}

int GradientPresetModel::rowCount(const QModelIndex& parent) const {
    if(parent.isValid())
        return 0;
    return _presets.count();
}

QVariant GradientPresetModel::data(const QModelIndex& index, int role) const {
    if(!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return QVariant();
    const QString k = keyAt(index.row());
    const Preset& p = _presets[k];
    switch(role){
    case Qt::DisplayRole:
    case NameRole:
        return p.name;
    case KeyRole:
        return k;
    case BuiltInRole:
        return p.builtIn;
    case StopsRole:{
        QVariantList stops;
        for(const QGradientStop& stop : p.gradient)
            stops << QVariantMap{{"position", stop.first}, {"color", stop.second}};
        return stops;
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> GradientPresetModel::roleNames() const {
    QHash<int, QByteArray> names = QAbstractListModel::roleNames();
    names[NameRole] = "name";
    names[KeyRole] = "key";
    names[BuiltInRole] = "builtIn";
    names[StopsRole] = "stops";
    return names;
}

QString GradientPresetModel::keyAt(int row) const {
    if(row < 0 || row >= _presets.count())
        return QString();
    return std::next(_presets.constBegin(), row).key();
}

GradientPresetModel::Preset GradientPresetModel::preset(const QString& key) const {
    return _presets.value(key);
}

void GradientPresetModel::load(){
    beginResetModel();
    _presets.clear();
    // Add built-in presets
    Preset blackAndWhite("B & W", true);
    blackAndWhite.gradient.append(QGradientStop(0., QColor(0, 0, 0)));
    blackAndWhite.gradient.append(QGradientStop(1., QColor(255, 255, 255)));
    Preset fade("Fade", true);
    fade.gradient.append(QGradientStop(0., QColor(255, 255, 255)));
    fade.gradient.append(QGradientStop(1., QColor(255, 255, 255, 0)));
    Preset rainbow("Rainbow", true);
    rainbow.gradient.append(QGradientStop(0.00, QColor(255, 0, 0)));
    rainbow.gradient.append(QGradientStop(0.17, QColor(255, 255, 0)));
    rainbow.gradient.append(QGradientStop(0.33, QColor(0, 255, 0)));
    rainbow.gradient.append(QGradientStop(0.50, QColor(0, 255, 255)));
    rainbow.gradient.append(QGradientStop(0.67, QColor(0, 0, 255)));
    rainbow.gradient.append(QGradientStop(0.83, QColor(255, 0, 255)));
    rainbow.gradient.append(QGradientStop(1.00, QColor(255, 0, 0)));
    Preset pastelRainbow("Pastel Rainbow", true);
    pastelRainbow.gradient.append(QGradientStop(0.00, QColor(240, 158, 167)));
    pastelRainbow.gradient.append(QGradientStop(0.17, QColor(246, 202, 148)));
    pastelRainbow.gradient.append(QGradientStop(0.33, QColor(193, 235, 192)));
    pastelRainbow.gradient.append(QGradientStop(0.50, QColor(160, 255, 252)));
    pastelRainbow.gradient.append(QGradientStop(0.67, QColor(199, 202, 255)));
    pastelRainbow.gradient.append(QGradientStop(0.83, QColor(246, 194, 243)));
    pastelRainbow.gradient.append(QGradientStop(1.00, QColor(240, 158, 167)));
    addPreset(blackAndWhite);
    addPreset(fade);
    addPreset(rainbow);
    addPreset(pastelRainbow);

    // Load stored presets
    CkbSettings settings(prefsPath);
    foreach(const QString& name, settings.childGroups()){
        QString pName = name.toLower();
        if(_presets.contains(pName))
            continue;
        SGroup group(settings, name);
        // Create keys in a map to ensure correct order and no duplicates
        QMap<int, QColor> colors;
        foreach(const QString& position, settings.childKeys()){
            int pos = position.toInt();
            if(pos < 0 || pos > 100)
                continue;
            colors[pos] = settings.value(position).value<QColor>();
        }
        if(colors.count() < 2)
            continue;
        Preset preset(name);
        QMapIterator<int, QColor> i(colors);
        while(i.hasNext()){
            i.next();
            preset.gradient.append(QGradientStop(i.key() / 100., i.value()));
        }
        _presets[pName] = preset;
    }
    endResetModel();
}

void GradientPresetModel::store() const {
    CkbSettings settings(prefsPath, true);
    QMapIterator<QString, Preset> i(_presets);
    while(i.hasNext()){
        Preset preset = i.next().value();
        if(!preset.builtIn){
            SGroup group(settings, preset.name);
            foreach(const QGradientStop& stop, preset.gradient)
                settings.setValue(QString::number((int)(stop.first * 100.)), stop.second);
        }
    }
}

QString GradientPresetModel::setPreset(const QString& name, const QGradientStops& gradient){
    beginResetModel();
    const QString k = key(name);
    _presets[k] = Preset(name, gradient);
    endResetModel();
    return k;
}

void GradientPresetModel::removePreset(const QString& key){
    beginResetModel();
    _presets.remove(key);
    endResetModel();
}
