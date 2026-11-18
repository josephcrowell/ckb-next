#ifndef EVENTMANAGERCONTROLLER_H
#define EVENTMANAGERCONTROLLER_H

#include <QObject>
#include <QVector>
#include "kbwindowinfo.h"

class KbWindowInfoModel;

// UI-independent logic of the mode event manager, shared by the Widgets and QML UIs:
// edits one mode's window rules and keeps a backup so Cancel can undo the edits
class EventManagerController : public QObject
{
    Q_OBJECT
public:
    explicit EventManagerController(QObject* parent = nullptr);

    // Starts editing a mode's rules (KbMode::winInfo()): backs them up and returns a model of
    // them, owned by this controller and valid until the next call or the controller's deletion
    KbWindowInfoModel* model(KbWindowInfo* rules);
    // Whether the edited rules are enabled; false before model() is called
    bool ruleEnabled() const;

public slots:
    // OK: keep the edits and set whether the rules are enabled
    void accept(bool enabled);
    // Cancel: restore the rules backed up by model()
    void reject();
    // Call when the window closes: rules left empty are disabled
    void closed();

private:
    KbWindowInfo* info = nullptr;
    KbWindowInfoModel* _model = nullptr;
    QVector<KbWindowInfo::MatchPair> backup;
};

#endif // EVENTMANAGERCONTROLLER_H
