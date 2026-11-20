#ifndef REBINDCONTROLLER_H
#define REBINDCONTROLLER_H

#include <QObject>
#include <QList>
#include <QPoint>
#include <QStringList>
#include <QUuid>

class KbBind;
class KbProfile;
class MacroReader;
class MacroTableModel;

// UI-independent logic of the rebinding panel, shared by the Widgets and QML UIs.
// An action is shown and edited as an ActionForm: one choice per list, where index 0 is
// "none" and index n is entry n - 1 of that list.
class RebindController : public QObject
{
    Q_OBJECT
public:
    enum Tab { TAB_KB = 0, TAB_MOUSE = 1, TAB_ANIM = 2, TAB_SPECIAL = 3, TAB_PROGRAM = 4, TAB_MACRO = 5 };
    enum KeyList { TypingKeys, ModifierKeys, FunctionKeys, NumpadKeys, MediaKeys, MouseButtons, MouseExtraButtons, WheelDirections, KEY_LIST_COUNT };
    // Where macros are recorded from: this device's notify node, every device's, or key
    // events passed to recordKeyEvent
    enum CaptureType { CAPTURE_CURRENT_DEVICE, CAPTURE_ALL_DAEMON_DEVICES, CAPTURE_ALL_KEYBOARDS };

    // DPI list: index 0 is none, then cycle up, cycle down, the stages (sniper first) and custom
    static const int DPI_OFFSET;
    static const int DPI_CUSTOM_INDEX;

    static const int MACRO_DEFAULT_REPETITION_DEBOUNCE = 500;
    static const int MACRO_DEFAULT_REPETITION_DELAY = 500;

    struct ActionForm {
        // How the action was read: which tab and which fields it fills
        enum Kind { None, Mouse, Key, Program, Animation, Special } kind = None;
        int tab = -1;
        // Key and mouse lists (index in the KeyList, 0 = none) and DPI
        int typing = 0, modifier = 0, function = 0, numpad = 0, media = 0;
        int mouse = 0, mouseExtra = 0, wheel = 0, dpi = 0;
        QPoint dpiCustom = QPoint(400, 400);
        // Animation (index in animationNames() + 1); "repeat" is the inverse of "only once"
        int animation = 0;
        bool animRepeat = false, animStopOnRelease = false;
        // Special: "mode", "light", "lock", "macro" or another special action
        QString special;
        int mode = 0, light = 0, lock = 0;
        bool modeWrap = true, lightWrap = false;
        // Program: single instance and its stop mode, for press and release
        QString programPress, programRelease;
        bool pressSingle = false, releaseSingle = false;
        int pressMode = 0, releaseMode = 0;
        // Macro: only filled when the macro string could be read (macroRead)
        bool macroRead = false;
        QString macroName, macroString;
        bool macroAsTyped = false;
        QString macroDebounce, macroDelay;
    };

    explicit RebindController(QObject* parent = nullptr);
    ~RebindController();

    inline KbBind* bind() const { return _bind; }
    inline KbProfile* profile() const { return _profile; }
    // Also builds the typing key list for the bind's layout
    void setBind(KbBind* newBind, KbProfile* newProfile);

    inline const QStringList& selection() const { return _selection; }
    inline void setSelection(const QStringList& selection) { _selection = selection; }

    inline const QStringList& keys(KeyList list) const { return _keys[list]; }
    // Display names of the lists filled at run time (typing, modifier, function, numpad, media)
    inline const QStringList& keyNames(KeyList list) const { return _names[list]; }
    // "(Previous)", "(Next)" and the profile's modes when the bind was set
    inline const QStringList& modeNames() const { return _modeNames; }
    // Text of each DPI stage, sniper first; empty for devices without DPI
    QStringList dpiNames() const;
    // Names of the light's animations. A form's animation choice refers to the animations
    // listed by the last call, even if the light's list has changed since.
    QStringList animationNames();
    inline MacroTableModel* macros() const { return _macros; }

    // Action shared by every selected key; empty if any is unbound or they differ
    QString commonAction() const;
    ActionForm form(const QString& action) const;
    // Replaces the macro events with the form's macro string, parsing as much as possible
    void loadMacro(const ActionForm& form);
    // Row of the first macro event that isn't matched by a press or release, or -1. Only when
    // the form would apply the macro.
    int unmatchedMacroLine(const ActionForm& form) const;
    // Sets the keys to the form's action; unbinds them if the form has none and unbind is set
    void apply(const QStringList& keys, const ActionForm& form, bool unbind);
    // Remembers the action the form describes when it was loaded or applied
    inline void setLoaded(const ActionForm& form) { _loadedAction = action(form); }
    // The form describes a different action from the one loaded or applied last
    inline bool isChanged(const ActionForm& form) const { return action(form) != _loadedAction; }
    void resetSelection();
    void unbindSelection();

    // Macro recording. The capture type offered when the macro tab opens: this device for
    // keyboards, else every daemon device if one is a keyboard, else all keyboards.
    CaptureType defaultCaptureType() const;
    inline CaptureType captureType() const { return _captureType; }
    inline void setCaptureType(CaptureType type) { _captureType = type; }
    // "Record from all keyboards" warning: UI/Bind/SuppressCaptureAllWarning
    static bool captureAllWarningSuppressed();
    static void setCaptureAllWarningSuppressed(bool suppressed);
    inline bool isRecording() const { return _reader != nullptr; }
    // Turns on the notify nodes for the capture type and starts reading recorded events
    void startRecording(CaptureType type);
    // Stops reading and turns the notify nodes off. Removes the last left click when the
    // recording was stopped with a mouse click on the device; returns true if it did.
    bool stopRecording(CaptureType type, bool clickedByMouse);
    // Resets the delay before the next recorded event
    void resetRecordingTimer();

public slots:
    // Key events of the UI, recorded while recording from all keyboards
    void recordKeyEvent(int keycode, bool down, Qt::KeyboardModifiers modifiers);

signals:
    // An event was recorded and appended to macros()
    void lineRecorded(const QString& line, bool keydown);
    // A key event while recording from all keyboards couldn't be translated
    void recordingError(QString key, QString modifiers);

private:
    void lineRead(QString line, qint64 ustime, bool keydown);
    // The action the form describes, or a null string if it has none
    QString action(const ActionForm& form) const;
    bool appliesMacro(const ActionForm& form) const;

    KbBind* _bind;
    KbProfile* _profile;
    QStringList _selection;
    QStringList _keys[KEY_LIST_COUNT];
    QStringList _names[KEY_LIST_COUNT];
    QStringList _modeNames;
    QList<QUuid> _animationIds;
    QString _loadedAction;
    CaptureType _captureType = CAPTURE_CURRENT_DEVICE;
    // Recording
    MacroReader* _reader = nullptr;
    CaptureType _recordingType = CAPTURE_CURRENT_DEVICE;
    bool _leftMouseClicked = false;
    MacroTableModel* _macros;
};

#endif // REBINDCONTROLLER_H
