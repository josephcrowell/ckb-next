#include "eventmanagercontroller.h"
#include "kbwindowinfomodel.h"

EventManagerController::EventManagerController(QObject* parent) : QObject(parent)
{
}

KbWindowInfoModel* EventManagerController::model(KbWindowInfo* rules)
{
    delete _model;
    info = rules;
    backup = info->items;
    _model = new KbWindowInfoModel(info, this);
    return _model;
}

bool EventManagerController::ruleEnabled() const
{
    return info && info->isEnabled();
}

void EventManagerController::accept(bool enabled)
{
    if(info)
        info->setEnabled(enabled);
}

void EventManagerController::reject()
{
    if(info)
        info->items = backup;
}

void EventManagerController::closed()
{
    if(info && info->isEmpty())
        info->setEnabled(false);
}
