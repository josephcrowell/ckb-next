#include "devicelistmodel.h"
#include <QDebug>
#include "appcontroller.h"
#include "devicecontroller.h"
#include "kb.h"
#include "kbmanager.h"

DeviceListModel::DeviceListModel(QObject* parent) : QAbstractListModel(parent)
{
    AppController* app = AppController::instance();
    if(!app){
        qWarning() << "DeviceListModel: no AppController, the device list stays empty";
        return;
    }
    for(Kb* device : app->devices())
        addDevice(device);
    connect(KbManager::kbManager(), &KbManager::kbConnected, this, &DeviceListModel::addDevice);
    connect(KbManager::kbManager(), &KbManager::kbDisconnected, this, &DeviceListModel::removeDevice);
}

int DeviceListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : _controllers.count();
}

QVariant DeviceListModel::data(const QModelIndex& index, int role) const {
    if(!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return QVariant();
    DeviceController* controller = _controllers.at(index.row());
    switch(role){
    case DeviceRole:
        return QVariant::fromValue(controller);
    case NameRole:
        return controller->device()->usbModel;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> DeviceListModel::roleNames() const {
    return {
        { DeviceRole, "device" },
        { NameRole, "name" },
    };
}

void DeviceListModel::addDevice(Kb* device){
    if(rowOf(device) >= 0)
        return;
    DeviceController* controller = new DeviceController(device, AppController::instance()->windowDetector(), this);
    // As KbWidget's constructor; kbConnected comes before the device loads its profile, whose
    // animations and bindings need the key layout
    controller->setupLayout();
    const int row = _controllers.count();
    beginInsertRows(QModelIndex(), row, row);
    _controllers.append(controller);
    endInsertRows();
}

void DeviceListModel::removeDevice(Kb* device){
    const int row = rowOf(device);
    if(row < 0)
        return;
    beginRemoveRows(QModelIndex(), row, row);
    DeviceController* controller = _controllers.takeAt(row);
    endRemoveRows();
    // Later, like the Widgets page: the QML page is destroyed later too, and its bindings
    // must not see the controller go first
    controller->deleteLater();
}

int DeviceListModel::rowOf(const Kb* device) const {
    for(int i = 0; i < _controllers.count(); i++){
        if(_controllers.at(i)->device() == device)
            return i;
    }
    return -1;
}
