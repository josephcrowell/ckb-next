#ifndef BINDCONTROLLER_H
#define BINDCONTROLLER_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QStringList>

class KbBind;
class KbMode;
class KbProfile;

// UI-independent logic of the binding page, shared by the Widgets and QML UIs
class BindController : public QObject
{
    Q_OBJECT
public:
    explicit BindController(QObject* parent = nullptr);

    inline KbBind* bind() const { return _bind; }
    inline KbProfile* profile() const { return _profile; }
    // Returns false, leaving bind and profile unchanged, when the bind is the current one
    bool setBind(KbBind* newBind, KbProfile* newProfile);

    inline const QStringList& selection() const { return _selection; }
    inline void setSelection(const QStringList& selection) { _selection = selection; }
    QString selectionText() const;
    // A Windows key is selected while the Windows key lock is on
    bool winLockWarning() const;

    // Action of every key in the layout
    QHash<QString, QString> actions() const;

    // Keys that reset and copy act on: the selection, or every key when nothing is selected
    QStringList targetKeys() const;
    QString resetQuestion() const;
    void resetBinds();
    QString copyPrompt() const;
    void copyToModes(const QList<KbMode*>& modes);

signals:
    // Forwarded KbBind::layoutChanged and KbBind::updated
    void layoutChanged();
    void bindUpdated();

private:
    KbBind* _bind;
    KbProfile* _profile;
    QStringList _selection;
};

#endif // BINDCONTROLLER_H
