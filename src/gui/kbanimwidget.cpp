#include <QMenu>
#include "animsettingdialog.h"
#include "kbanimwidget.h"
#include "ui_kbanimwidget.h"
#include "lightcontroller.h"
#include <QMessageBox>

KbAnimWidget::KbAnimWidget(QWidget* parent) :
    QWidget(parent), controller(nullptr), current(nullptr), noReorder(false),
    ui(new Ui::KbAnimWidget)
{
    ui->setupUi(this);
    ui->animList->setVisible(false);
    setCurrent(nullptr);
    connect(ui->animList, SIGNAL(orderChanged()), this, SLOT(reorderAnims()));
}

KbAnimWidget::~KbAnimWidget(){
    delete ui;
}

void KbAnimWidget::setController(LightController* newController){
    controller = newController;
    connect(controller, &LightController::animationsReloaded, this, &KbAnimWidget::refreshList);
}

void KbAnimWidget::refreshList(){
    noReorder = true;
    setCurrent(nullptr);
    ui->animList->clear();
    animations.clear();
    // Add the animations from the new lighting mode
    KbLight* light = controller ? controller->light() : nullptr;
    if(!light){
        ui->animList->setVisible(false);
        ui->noAnimLabel->setVisible(true);
        return;
    }
    QList<KbAnim*> newAnimations = light->animList();
    if(newAnimations.count() == 0){
        ui->animList->setVisible(false);
        ui->noAnimLabel->setVisible(true);
        return;
    }
    ui->animList->setVisible(true);
    foreach(KbAnim* anim, newAnimations){
        QListWidgetItem* item = new QListWidgetItem(anim->name(), ui->animList);
        item->setData(Qt::UserRole, anim->guid());
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        animations[anim->guid()] = anim;
        ui->animList->addItem(item);
    }
    ui->noAnimLabel->setVisible(false);
    noReorder = false;
}

void KbAnimWidget::reorderAnims(){
    if(controller && controller->light() && !noReorder){
        // Clear and rebuild the list of animations in case the animation moved
        int count = ui->animList->count();
        QList<KbAnim*> animList;
        for(int i = 0; i < count; i++){
            QListWidgetItem* item = ui->animList->item(i);
            KbAnim* anim = animations[item->data(Qt::UserRole).toUuid()];
            if(anim && !animList.contains(anim))
                animList.append(anim);
            item->setFlags(item->flags() | Qt::ItemIsEditable);
        }
        controller->reorderAnimations(animList);
    }
}

void KbAnimWidget::clearSelection(){
    ui->animList->setCurrentItem(nullptr);
    setCurrent(nullptr);
}

void KbAnimWidget::addAnim(KbAnim* animation){
    if(!animation)
        return;
    noReorder = true;
    QListWidgetItem* item = new QListWidgetItem(animation->name(), ui->animList);
    item->setData(Qt::UserRole, animation->guid());
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    animations[animation->guid()] = animation;
    ui->animList->addItem(item);
    ui->animList->setCurrentItem(item);
    ui->animList->setVisible(true);
    ui->noAnimLabel->setVisible(false);

    setCurrent(animation);
    noReorder = false;

    // Activate settings dialog
    on_propertyButton_clicked();
}

void KbAnimWidget::duplicateAnim(KbAnim* old){
    if(!controller->light())
        return;
    noReorder = true;
    KbAnim* animation = controller->duplicateAnimation(old);
    // Refresh the list. insertItem doesn't seem to place the item in the correct position on its own...
    refreshList();
    ui->animList->setCurrentRow(controller->light()->animList().indexOf(animation));
    ui->animList->setVisible(true);
    ui->noAnimLabel->setVisible(false);

    setCurrent(animation);
    noReorder = false;
}

void KbAnimWidget::setCurrent(KbAnim* newCurrent){
    if(newCurrent != current)
        emit animChanged(current = newCurrent);
    if(!current){
        selectedKeys.clear();
        ui->selectionStack->setCurrentIndex(0);
        return;
    }
    selectedKeys = current->keys();
    const AnimScript* script = current->script();
    if(!script){
        ui->selectionStack->setCurrentIndex(2);
        ui->aMissingLabel->setText("The \"" + current->scriptName() + "\" script could not be loaded. Please check your animation directory.");
        return;
    }
    ui->selectionStack->setCurrentIndex(1);
    ui->aNameLabel->setText(script->name());
    ui->aVerLabel->setText("v" + script->version());
    ui->aCopyLabel->setText(script->copyright());

    ui->nameBox->setText(current->name());
    ui->opacityBox->setValue(current->opacity() * 100.);
    ui->blendBox->setCurrentIndex((int)current->mode());
}

void KbAnimWidget::setSelectedKeys(const QStringList& keys){
    selectedKeys = keys;
    if(keys.count() == 0)
        ui->keyButton->setVisible(false);
    else
        ui->keyButton->setVisible(true);
}

void KbAnimWidget::on_animList_currentItemChanged(QListWidgetItem* cur, QListWidgetItem* prev){
    if(!cur)
        setCurrent(nullptr);
    else
        setCurrent(animations[cur->data(Qt::UserRole).toUuid()]);
}

void KbAnimWidget::on_animList_itemChanged(QListWidgetItem* item){
    if(item){
        KbAnim* anim = animations[item->data(Qt::UserRole).toUuid()];
        if(anim){
            // Don't allow empty names
            if(!controller->renameAnimation(anim, item->text())){
                item->setText(anim->name());
                return;
            }

            if(anim == current && !noReorder)
                ui->nameBox->setText(anim->name());
        }
    }
}

void KbAnimWidget::on_animList_customContextMenuRequested(const QPoint &pos){
    QListWidgetItem* item = ui->animList->itemAt(pos);
    if(!item)
        return;
    setCurrent(animations[item->data(Qt::UserRole).toUuid()]);

    QMenu menu(this);
    QAction* rename = new QAction(tr("Rename..."), this);
    QAction* duplicate = new QAction(tr("Duplicate"), this);
    QAction* del = new QAction(tr("Delete"), this);
    menu.addAction(rename);
    menu.addAction(duplicate);
    menu.addAction(del);
    QAction* result = menu.exec(QCursor::pos());
    if(result == rename)
        ui->animList->editItem(item);
    else if(result == duplicate)
        duplicateAnim(current);
    else if(result == del)
        on_deleteButton_clicked();
}

void KbAnimWidget::on_nameBox_textEdited(const QString &arg1){
    // Don't apply name change if left blank
    if(arg1.isEmpty())
        return;

    if(current){
        noReorder = true;
        controller->renameAnimation(current, arg1);
        ui->animList->currentItem()->setText(current->name());
        noReorder = false;
    }
}

void KbAnimWidget::on_opacityBox_valueChanged(double arg1){
    controller->setAnimationOpacity(current, arg1);
}

void KbAnimWidget::on_blendBox_activated(int index){
    controller->setAnimationBlendMode(current, index);
}

void KbAnimWidget::on_keyButton_clicked(){
    if(current){
        controller->setAnimationKeys(current, selectedKeys);
        emit didUpdateSelection(selectedKeys);
    }
    controller->restartAnimation();
}

void KbAnimWidget::on_deleteButton_clicked(){
    if(current){
        if(QMessageBox::question(this, tr("Delete animation"), tr("Are you sure you want to delete this animation?")) != QMessageBox::Yes)
            return;
        animations.remove(current->guid());
        controller->deleteAnimation(current);
        setCurrent(nullptr);
        delete ui->animList->currentItem();
        if(animations.count() == 0){
            ui->animList->setVisible(false);
            ui->noAnimLabel->setVisible(true);
        }
    }
    controller->restartAnimation();
}

void KbAnimWidget::on_propertyButton_clicked(){
    if(!current)
        return;
    // Present animation property popup
    AnimSettingDialog dialog(this, current);
    dialog.exec();
    const bool accepted = dialog.result() == QDialog::Accepted;
    controller->finishAnimationSettings(current, accepted);
    if(!accepted)
        return;
    // Update name
    ui->nameBox->setText(dialog.name());
    on_nameBox_textEdited(dialog.name());
}

void KbAnimWidget::on_nameBox_editingFinished(){
    if(ui->nameBox->text().isEmpty() && current)
        ui->nameBox->setText(current->name());
}
