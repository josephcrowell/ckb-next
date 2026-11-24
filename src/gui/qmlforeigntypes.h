#ifndef QMLFOREIGNTYPES_H
#define QMLFOREIGNTYPES_H

#include <QtQml/qqmlregistration.h>
#include "devicecontroller.h"

// C++ types QML receives but never creates. They are also compiled without the QML UI, so
// they are registered here instead of in their own headers.

struct DeviceControllerForeign
{
    Q_GADGET
    QML_FOREIGN(DeviceController)
    QML_NAMED_ELEMENT(DeviceController)
    QML_UNCREATABLE("Created by DeviceListModel")
};

#endif // QMLFOREIGNTYPES_H
