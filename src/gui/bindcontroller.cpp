#include "bindcontroller.h"
#include <QCoreApplication>
#include "kbbind.h"
#include "kbmode.h"

BindController::BindController(QObject* parent) :
    QObject(parent), _bind(nullptr), _profile(nullptr)
{
}

bool BindController::setBind(KbBind* newBind, KbProfile* newProfile){
    if(_bind == newBind)
        return false;
    if(_bind){
        disconnect(_bind, &KbBind::layoutChanged, this, &BindController::layoutChanged);
        disconnect(_bind, &KbBind::updated, this, &BindController::bindUpdated);
    }
    connect(newBind, &KbBind::layoutChanged, this, &BindController::layoutChanged);
    connect(newBind, &KbBind::updated, this, &BindController::bindUpdated);
    _bind = newBind;
    _profile = newProfile;
    return true;
}

QString BindController::selectionText() const {
    int count = _selection.count();
    if(count == 0){
        // No keys selected
        return QCoreApplication::translate("KbBindWidget", "Click to select");
    }
    if(count == 1){
        // Single key selected: show key name and binding
        QString key = _selection[0];
        const Key& pos = _bind->map().key(key);
        if(!pos)
            return QCoreApplication::translate("KbBindWidget", "(Unknown)");
        return pos.friendlyName(false).split("\n")[0] + " → " + _bind->friendlyActionName(key).split("\n")[0];
    }
    return QString(QCoreApplication::translate("KbBindWidget", "%1 %2 selected")).arg(count)
            .arg(_bind->isMouse() ? QCoreApplication::translate("KbBindWidget", "buttons") : QCoreApplication::translate("KbBindWidget", "keys"));
}

bool BindController::winLockWarning() const {
    return (_selection.contains("rwin") || _selection.contains("lwin")) && _bind->winLock();
}

QHash<QString, QString> BindController::actions() const {
    const KeyMap& map = _bind->map();
    QHash<QString, QString> actions;
    foreach(const QString& key, map.keys())
        actions[key] = _bind->action(key);
    return actions;
}

QStringList BindController::targetKeys() const {
    if(_selection.isEmpty())
        return _bind->map().keys();
    return _selection;
}

QString BindController::resetQuestion() const {
    const KeyMap& map = _bind->map();
    int count = targetKeys().count();
    QString type = _bind->isMouse() ? QCoreApplication::translate("KbBindWidget", "button") : QCoreApplication::translate("KbBindWidget", "key");
    QString text;
    if(count == map.count())
        text = QCoreApplication::translate("KbBindWidget", "<center>Reset all %1s to default?</center>");
    else if(count == 1)
        text = QCoreApplication::translate("KbBindWidget", "<center>Reset this %1 to default?</center>");
    else
        text = QCoreApplication::translate("KbBindWidget", "<center>Reset %1 %2s to default?</center>").arg(count);
    return text.arg(type);
}

void BindController::resetBinds(){
    _bind->resetAction(targetKeys());
}

QString BindController::copyPrompt() const {
    int count = _selection.count();
    QString type = _bind->isMouse() ? QCoreApplication::translate("KbBindWidget", "button") : QCoreApplication::translate("KbBindWidget", "key");
    QString text = QString("%1 %2").arg(count).arg(type) + (count == 1 ? "" : QCoreApplication::translate("KbBindWidget", "s"));
    if(count == 0)
        // Copy all keys if none selected
        text = QCoreApplication::translate("KbBindWidget", "all %1s").arg(type);
    return QCoreApplication::translate("KbBindWidget", "Copy binding for ") + text + QCoreApplication::translate("KbBindWidget", " to:");
}

void BindController::copyToModes(const QList<KbMode*>& modes){
    const QStringList keys = targetKeys();
    foreach(KbMode* mode, modes){
        KbBind* modeBind = mode->bind();
        foreach(const QString& key, keys)
            modeBind->setAction(key, _bind->action(key));
    }
}
