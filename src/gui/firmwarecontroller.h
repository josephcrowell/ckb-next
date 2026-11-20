#ifndef FIRMWARECONTROLLER_H
#define FIRMWARECONTROLLER_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include "ckbversionnumber.h"

class Kb;

// UI-independent logic of a firmware update, shared by the Widgets and QML UIs. Steps: download
// the newest firmware if no file was given, check it fits the device, save it for the daemon,
// then start(); progress and finished() follow. If the device goes away during the update, it
// counts as a success (it restarts to finish).
class FirmwareController : public QObject
{
    Q_OBJECT
public:
    // blob: the firmware file the user picked, or empty to download the newest one
    FirmwareController(Kb* device, const QByteArray& blob, QObject* parent = nullptr);
    // Removes the saved firmware file
    ~FirmwareController();

    // Reads a firmware file the user picked; false if it can't be read
    static bool readFile(const QString& path, QByteArray& blob);

    QString deviceName() const;
    CkbVersionNumber currentVersion() const;

    inline bool needsDownload() const { return _blob.isEmpty(); }
    // Downloads the newest firmware for the device (blocks until done; empty on failure)
    void download();
    // The firmware's version if it is valid for the device, null otherwise
    CkbVersionNumber blobVersion() const;
    // Saves the firmware to a temporary file for the daemon; false if it couldn't be
    bool saveBlob();

    // Whether the update can start (firmware saved and the device still there)
    bool canStart() const;
    void start();

signals:
    // Only positive values are passed on
    void progress(int current, int total);
    void finished(bool succeeded);

private:
    void deviceProgress(int current, int total);
    void deviceFinished(bool succeeded);
    void deviceRemoved();
    void cleanBlob();

    Kb* _device;
    ushort _productId;
    QByteArray _blob;
    // Where the firmware is saved; empty if it isn't
    QString _savePath;
};

#endif // FIRMWARECONTROLLER_H
