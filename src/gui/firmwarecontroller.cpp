#include "firmwarecontroller.h"
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>
#include "appcontroller.h"
#include "kb.h"
#include "kbfirmware.h"

FirmwareController::FirmwareController(Kb* device, const QByteArray& blob, QObject* parent) :
    QObject(parent), _device(device), _productId(device->productID), _blob(blob)
{
    connect(device, &QObject::destroyed, this, &FirmwareController::deviceRemoved);
    connect(device, &Kb::fwUpdateProgress, this, &FirmwareController::deviceProgress);
    connect(device, &Kb::fwUpdateFinished, this, &FirmwareController::deviceFinished);
}

FirmwareController::~FirmwareController(){
    cleanBlob();
}

bool FirmwareController::readFile(const QString& path, QByteArray& blob){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
        return false;
    blob = file.readAll();
    return true;
}

QString FirmwareController::deviceName() const {
    return _device->usbModel;
}

CkbVersionNumber FirmwareController::currentVersion() const {
    return _device->firmware.app;
}

void FirmwareController::download(){
    // This can take a while
    _blob = AppController::instance()->firmware()->dataForBoard(_productId);
}

CkbVersionNumber FirmwareController::blobVersion() const {
    ushort vendor = 0x1b1c;
    if(_blob.length() < 0x0108)
        return CkbVersionNumber();
    const char* bData = _blob.data();

    // Make sure it matches this device based on the vendor and product IDs embedded in the blob
    if (memcmp(&vendor, bData + 0x102, 2) || memcmp(&_productId, bData + 0x104, 2)) {
        qCritical() << "Something really bad happened - wrong firmware file detected";
        return CkbVersionNumber();
    }

    // Extract the version from the blob
    return CkbVersionNumber(_blob.mid(0x106, 2));
}

bool FirmwareController::saveBlob(){
    if(!_savePath.isEmpty())
        return true;
    // A folder only this user can open, so nobody can swap the file before the daemon reads it
    _saveDir.reset(new QTemporaryDir);
    if(!_saveDir->isValid()){
        _saveDir.reset();
        return false;
    }
    QString path = _saveDir->filePath("fwblob.bin");
    QFile output(path);
    if(!output.open(QIODevice::WriteOnly) || !output.write(_blob)){
        _saveDir.reset();
        return false;
    }
    output.close();
    _savePath = path;
    return true;
}

void FirmwareController::cleanBlob(){
    // Removes the folder with the file
    _saveDir.reset();
    _savePath = "";
}

bool FirmwareController::canStart() const {
    return !_savePath.isEmpty() && _device;
}

void FirmwareController::start(){
    _started = true;
    _device->fwUpdate(_savePath);
}

void FirmwareController::deviceProgress(int current, int total){
    if(current > 0 && total > 0)
        emit progress(current, total);
}

void FirmwareController::deviceFinished(bool succeeded){
    cleanBlob();
    emit finished(succeeded);
}

void FirmwareController::deviceRemoved(){
    _device = nullptr;
    if(!_started){
        // Nothing was installed
        cleanBlob();
        emit deviceLost();
        return;
    }
    // Assume success if upgrade in progress
    if(!_savePath.isEmpty())
        deviceFinished(true);
}
