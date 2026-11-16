#include "kbmodeeventmgr.h"
#include "kbwindowinfomodel.h"
#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlError>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QDebug>

KbModeEventMgr::KbModeEventMgr(QWidget* parent, KbWindowInfo* i, const QString& modeName) :
    QDialog(parent), info(i), model(new KbWindowInfoModel(info, this)), quick(nullptr),
    backup(info->items)
{
    setWindowModality(Qt::WindowModal);
    resize(735, 359);
    setWindowTitle(tr("Mode Event Manager"));

    // Draw text with the same font engine as Qt Widgets. This app has no other Qt Quick scenes.
    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);

    // Created after the model, and deleted first in the destructor
    quick = new QQuickWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(quick);
    quick->setResizeMode(QQuickWidget::SizeRootObjectToView);

    connect(quick, &QQuickWidget::statusChanged, this, [this](QQuickWidget::Status status) {
        if(status != QQuickWidget::Error)
            return;
        for(const QQmlError& e : quick->errors())
            qWarning() << "Mode event manager:" << e.toString();
    });
    // Without a connected slot, Qt Quick terminates the application on this error
    connect(quick, &QQuickWidget::sceneGraphError, this, [this](QQuickWindow::SceneGraphError, const QString& message) {
        qWarning() << "Mode event manager scene graph error:" << message;
        close();
    });

    quick->setSource(QUrl(QStringLiteral("qrc:/qml/ModeEventManager.qml")));
    QQuickItem* root = quick->rootObject();
    if(quick->status() != QQuickWidget::Ready || !root) {
        qWarning() << "Mode event manager: failed to load the QML interface";
        return;
    }

    if(!root->setProperty("model", QVariant::fromValue(static_cast<QObject*>(model)))
            || !root->setProperty("modeText", tr("Switch to mode \"%1\" when:").arg(modeName))
            || !root->setProperty("ruleEnabled", info->isEnabled())
            || !root->setProperty("windowActive", isActiveWindow())) {
        qWarning() << "Mode event manager: failed to set the QML properties";
        return;
    }

    if(!connect(root, SIGNAL(accepted(bool)), this, SLOT(onQmlAccepted(bool)))
            || !connect(root, SIGNAL(rejected()), this, SLOT(onQmlRejected()))
            || !connect(root, SIGNAL(escapePressed()), this, SLOT(reject()))) {
        qWarning() << "Mode event manager: failed to connect the QML signals";
        return;
    }
    valid = true;
}

KbModeEventMgr::~KbModeEventMgr()
{
    // Destroy the QML scene before the model it references
    QQuickWidget* q = quick;
    quick = nullptr;
    delete q;
}

void KbModeEventMgr::onQmlAccepted(bool enabled)
{
    info->setEnabled(enabled);
    close();
}

void KbModeEventMgr::onQmlRejected()
{
    info->items = backup;
    close();
}

void KbModeEventMgr::changeEvent(QEvent* evt)
{
    QDialog::changeEvent(evt);
    // The QML scene can't see the window's activation itself
    if(evt->type() == QEvent::ActivationChange && quick && quick->rootObject())
        quick->rootObject()->setProperty("windowActive", isActiveWindow());
}

void KbModeEventMgr::closeEvent(QCloseEvent* evt)
{
    // Disable the mode event if empty
    if(info->isEmpty())
        info->setEnabled(false);

    // Continue handling the event
    evt->accept();
}
