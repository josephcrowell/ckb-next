#ifndef QMLSINGLETONS_H
#define QMLSINGLETONS_H

#include <QJSEngine>
#include <QQmlEngine>
#include <QThread>
#include <QtQml/qqmlregistration.h>
#include "appcontroller.h"
#include "kbmanager.h"

// The application-wide objects as QML singletons. They are created in C++ before the QML
// is loaded and outlive the engine, so the engine must not take ownership of them. Only one
// engine, in the objects' thread, may use them.
template<typename T> T* qmlSingleton(T* instance, QJSEngine* engine){
    static QJSEngine* firstEngine = nullptr;
    if(!firstEngine)
        firstEngine = engine;
    Q_ASSERT(engine == firstEngine);
    Q_ASSERT(instance);
    Q_ASSERT(engine->thread() == instance->thread());
    QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
    return instance;
}

struct AppControllerForeign
{
    Q_GADGET
    QML_FOREIGN(AppController)
    QML_SINGLETON
    QML_NAMED_ELEMENT(AppController)
public:
    static AppController* create(QQmlEngine*, QJSEngine* engine){
        return qmlSingleton(AppController::instance(), engine);
    }
};

struct KbManagerForeign
{
    Q_GADGET
    QML_FOREIGN(KbManager)
    QML_SINGLETON
    QML_NAMED_ELEMENT(KbManager)
public:
    static KbManager* create(QQmlEngine*, QJSEngine* engine){
        return qmlSingleton(KbManager::kbManager(), engine);
    }
};

#endif // QMLSINGLETONS_H
