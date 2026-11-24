#ifndef DEVICELISTMODEL_H
#define DEVICELISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QtQml/qqmlregistration.h>

class DeviceController;
class Kb;

// Connected devices in the order they were connected, one DeviceController each. The
// controllers are children of the model, so they go when the model or the device goes.
class DeviceListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum Roles {
        DeviceRole = Qt::UserRole + 1,
        NameRole,
    };

    explicit DeviceListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    void addDevice(Kb* device);
    void removeDevice(Kb* device);
    int rowOf(const Kb* device) const;

    QList<DeviceController*> _controllers;
};

#endif // DEVICELISTMODEL_H
