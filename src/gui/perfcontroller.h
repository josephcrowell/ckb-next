#ifndef PERFCONTROLLER_H
#define PERFCONTROLLER_H

#include <QColor>
#include <QList>
#include <QObject>
#include "kbperf.h"

class KbMode;
class KbProfile;

// UI-independent logic of the keyboard and mouse performance pages, shared by the Widgets
// and QML UIs
class PerfController : public QObject
{
    Q_OBJECT
public:
    // Modes of a hardware indicator (num, caps, scroll lock), in the order the UI lists them
    enum HwMode { NORMAL, ALWAYS_ON, ALWAYS_OFF, RGB, BOTH };
    static HwMode modeFromRaw(bool software, KbPerf::i_hw hardware);
    static void rawFromMode(HwMode mode, bool& software, KbPerf::i_hw& hardware);

    struct Indicator {
        QColor color1, color2, color3;
        // The colours are used (key colouring on)
        bool software;
        KbPerf::i_hw hardware;
        // For hardware indicators
        HwMode mode;
    };

    explicit PerfController(QObject* parent = nullptr);

    inline KbPerf* perf() const { return _perf; }
    inline KbProfile* profile() const { return _profile; }
    void setPerf(KbPerf* newPerf, KbProfile* newProfile);
    // Copies the performance settings to the modes
    void copyToModes(const QList<KbMode*>& modes);

    // Indicator light opacity in percent
    int intensity() const;
    void setIntensity(int percent);

    // Keyboard indicators. The first KbPerf::HW_I_COUNT are hardware indicators set by mode;
    // the others are switched on and off.
    static inline bool isHardwareIndicator(int index) { return index < KbPerf::HW_I_COUNT; }
    Indicator indicator(int index) const;
    // Sets an indicator from the UI (the mode for hardware indicators, enabled for the
    // others) and returns whether its colours are used
    bool setIndicator(int index, bool enabled, int hwMode, const QColor& color1, const QColor& color2, const QColor& color3);
    // Mode and macro indicators exist on K95s; the mute indicator not on the Strafe
    bool hasK95Indicators() const;
    bool hasMuteIndicator() const;
    static bool muteDeviceSupported();
    int muteDevice() const;
    void setMuteDevice(int device);

    // Mouse DPI stages. X and Y are linked unless UI/DPI/UnlinkXY is set.
    inline bool xyLinked() const { return _xyLink; }
    // Changes the link for this page only; saveXyLinked() stores it
    inline void setXyLinked(bool linked) { _xyLink = linked; }
    void saveXyLinked();
    // Slider positions use a logarithmic scale; DPI values from it are rounded to 50 (100 above 1000)
    static int sliderToDpi(int position, int minimum, int maximum);
    static int dpiToSlider(int dpi, int minimum, int maximum);
    // Sets one axis of a stage, or both when X and Y are linked
    void setDpiX(int index, int value);
    void setDpiY(int index, int value);
    // Sets a stage's colour, or every stage's and the "other" colour
    void setDpiColor(int index, const QColor& color, bool allStages);
    void setDpiEnabled(int index, bool enabled);
    bool dpiIndicator() const;
    void setDpiIndicator(bool enabled);
    bool angleSnap() const;
    void setAngleSnap(bool enabled);
    // Dark Core mice only support lift heights 3 to 5
    inline void setDarkCore() { _darkCore = true; }
    int liftHeightIndex() const;
    void setLiftHeightIndex(int index);
    // Active DPI stage (KbPerf::OTHER and above: none of the stages)
    int dpiIndex() const;

signals:
    // Forwarded KbPerf::dpiChanged of the current perf
    void dpiChanged(int index);

private:
    KbPerf* _perf;
    KbProfile* _profile;
    bool _xyLink;
    bool _darkCore;
    QMetaObject::Connection _dpiConnection;
};

#endif // PERFCONTROLLER_H
