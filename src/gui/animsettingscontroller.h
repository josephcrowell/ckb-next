#ifndef ANIMSETTINGSCONTROLLER_H
#define ANIMSETTINGSCONTROLLER_H

#include <QAbstractListModel>
#include <QObject>
#include <QStringList>
#include "animscript.h"

class KbAnim;

// The animation's own parameters (not the playback ones), in script order, with their
// values in the form the controls use (see AnimSettingsController::uiValue)
class AnimParamListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        TypeRole,       // AnimScript::Param::Type
        PrefixRole,
        PostfixRole,
        MinimumRole,
        MaximumRole,
        ValueRole,
    };

    explicit AnimParamListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    inline const AnimScript::Param& param(int row) const { return _params.at(row); }
    inline const QVariant& value(int row) const { return _values.at(row); }

    void setParams(const QList<AnimScript::Param>& params, const QList<QVariant>& values);
    void setValue(const QString& name, const QVariant& value);

private:
    QList<AnimScript::Param> _params;
    QList<QVariant> _values;
};

// UI-independent logic of the animation settings dialog, shared by the Widgets and QML UIs.
// Changes go to the animation's uncommitted parameters; LightController::finishAnimationSettings
// commits or discards them.
class AnimSettingsController : public QObject
{
    Q_OBJECT
public:
    AnimSettingsController(KbAnim* anim, QObject* parent = nullptr);

    inline KbAnim* animation() const { return _anim; }
    inline AnimParamListModel* params() const { return _params; }

    // Parameter value as the controls use it: bool, int, double, QColor (RGB, ARGB),
    // QString (gradients, strings) or int degrees (angles); invalid for labels
    static QVariant uiValue(const AnimScript::Param& param, const QVariant& value);
    // Sets a parameter from a control value, converted by the parameter's type
    void setParam(const QString& name, const QVariant& uiValue);

    // Playback parameters as committed
    bool flag(const QString& name) const;
    double number(const QString& name) const;

    bool hasDuration() const;
    // Last duration set (the committed one at first, 1 s without a duration parameter)
    inline double duration() const { return _lastDuration; }
    // Repeat values equal to the last duration follow it when it changes
    inline bool matchesDuration(double value) const { return value == _lastDuration; }
    // Remembers the duration; the caller then sets "duration", "repeat" and "kprepeat"
    inline void setDuration(double duration) { _lastDuration = duration; }

    // "repeat" scripts count stops (check box: forever); others time them (check box: stop after)
    bool hasRepeat() const;
    // Initial state of the check box and value for "stop" or "kpstop"
    bool stopChecked(const QString& name) const;
    QVariant stopValue(const QString& name) const;
    inline bool stopValueEnabled(bool checked) const { return _hasRepeat ? !checked : checked; }
    // Sets "stop" or "kpstop": -1 when not limited, the value otherwise
    void setStop(const QString& name, bool checked, const QVariant& uiValue);

    // Key press modes offered by this script, and the chosen one
    QStringList keypressModes() const;
    int keypressModeIndex() const;
    void setKeypressMode(int index);

private:
    KbAnim* _anim;
    AnimParamListModel* _params;
    double _lastDuration;
    bool _hasRepeat;
};

#endif // ANIMSETTINGSCONTROLLER_H
