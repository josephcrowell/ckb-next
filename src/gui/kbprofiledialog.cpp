#include <QMenu>
#include "kbprofiledialog.h"
#include "ui_kbprofiledialog.h"
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include "profilescontroller.h"

KbProfileDialog::KbProfileDialog(KbWidget *parent) :
    QDialog(parent),
    ui(new Ui::KbProfileDialog), device(parent->device), controller(new ProfilesController(parent->device, this))
{
    ui->setupUi(this);
    connect(ui->profileList, SIGNAL(orderChanged()), this, SLOT(profileList_reordered()));

    // Populate profile list
    repopulate();
    //activeProfile = device->newProfile(device->currentProfile());
}

KbProfileDialog::~KbProfileDialog(){
    delete ui;
}

void KbProfileDialog::profileList_reordered(){
    // Rebuild profile list from items
    QList<QUuid> order;
    int count = ui->profileList->count();
    for(int i = 0; i < count; i++){
        QListWidgetItem* item = ui->profileList->item(i);
        order.append(item->data(GUID).toUuid());
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    controller->reorder(order);
}

void KbProfileDialog::repopulate(){
    ui->profileList->clear();
    QListWidgetItem* current = nullptr;
    foreach(KbProfile* profile, device->profiles()){
        QListWidgetItem* item = new QListWidgetItem(QIcon(controller->isHardwareProfile(profile) ? ":/img/icon_profile_hardware.png" : ":/img/icon_profile.png"), profile->name(), ui->profileList);
        item->setData(GUID, profile->id().guid);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        if(profile == device->currentProfile()){
            item->setSelected(true);
            current = item;
        }
        ui->profileList->addItem(item);
    }
    if(current)
        ui->profileList->setCurrentItem(current);
    addNewProfileItem();
}

void KbProfileDialog::addNewProfileItem(){
    // Add an item for creating a new profile. Make it editable but not dragable.
    QListWidgetItem* item = new QListWidgetItem(tr("New profile..."), ui->profileList);
    item->setFlags((item->flags() | Qt::ItemIsEditable) & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled);
    item->setData(NEW_FLAG, 1);
    QFont font = item->font();
    font.setItalic(true);
    item->setFont(font);
    item->setIcon(QIcon(":/img/icon_plus.png"));
    ui->profileList->addItem(item);
}

void KbProfileDialog::on_profileList_itemClicked(QListWidgetItem *item){
    QUuid guid = item->data(GUID).toUuid();
    if(guid.isNull() && item->data(NEW_FLAG).toInt() == 1){
        // New profile
        item->setText("");
        ui->profileList->editItem(item);
        item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        QFont font = item->font();
        font.setItalic(false);
        item->setFont(font);
        item->setIcon(QIcon(":/img/icon_profile.png"));
        // Add the new profile and assign it to this item. It's selected only after the item
        // has its ID, so the item change isn't taken for a rename.
        KbProfile* newProfile = controller->addProfile();
        item->setData(GUID, newProfile->id().guid);
        item->setData(NEW_FLAG, 0);
        controller->select(newProfile);
        // Create another "new profile" item to replace this one
        addNewProfileItem();
    }
}

void KbProfileDialog::on_profileList_itemChanged(QListWidgetItem *item){
    KbProfile* currentProfile = device->currentProfile();
    if(!item || !currentProfile || item->data(GUID).toUuid() != currentProfile->id().guid)
        return;
    // Set the text to the actual name (trimmed, "" replaced with "Unnamed")
    item->setText(controller->rename(currentProfile, item->text()));
}

void KbProfileDialog::on_profileList_customContextMenuRequested(const QPoint &pos){
    QListWidgetItem* item = ui->profileList->itemAt(pos);
    KbProfile* currentProfile = device->currentProfile();
    if(!item || !currentProfile || item->data(GUID).toUuid() != currentProfile->id().guid)
        return;

    QMenu menu(this);
    QAction* rename = new QAction(tr("Rename"), this);
    QAction* duplicate = new QAction(tr("Duplicate"), this);
    QAction* del = new QAction(tr("Delete"), this);
    bool canDelete = controller->canDelete();
    if(!canDelete)
        // Can't delete the last profile on the device
        del->setEnabled(false);
    QAction* hwsave = new QAction(tr("Save to Hardware"), this);
    // Disable Save to hardware button for unsupported devices
    if(!controller->canSaveToHardware()){
        hwsave->setDisabled(true);
        hwsave->setToolTip(tr("Saving to hardware is not supported on this device."));
    }
    QAction* moveup = new QAction(tr("Move Up"), this);
    if(!controller->canMoveUp(currentProfile))
        moveup->setEnabled(false);
    QAction* movedown = new QAction(tr("Move Down"), this);
    if(!controller->canMoveDown(currentProfile))
        movedown->setEnabled(false);
    menu.addAction(rename);
    menu.addAction(duplicate);
    menu.addAction(del);
    menu.addSeparator();
    menu.addAction(hwsave);
    menu.addSeparator();
    menu.addAction(moveup);
    menu.addAction(movedown);
    QAction* result = menu.exec(QCursor::pos());
    if(result == rename){
        ui->profileList->editItem(item);
        // We need to return otherwise repopulate() gets called and the item is destroyed
        return;
    } else if(result == duplicate){
        controller->duplicate(currentProfile);
    } else if(result == del){
        if(!canDelete)
            return;
        if(QMessageBox::question(this, tr("Delete profile"), tr("Are you sure you want to delete this profile?")) != QMessageBox::Yes)
            return;
        controller->remove(currentProfile);
    } else if(result == hwsave){
        controller->saveToHardware();
        // Refresh item icons
        int count = ui->profileList->count();
        for(int i = 0; i < count; i++){
            QListWidgetItem* hwitem = ui->profileList->item(i);
            if(hwitem->data(NEW_FLAG).toUInt() == 1)
                continue;
            KbProfile* profile = device->find(hwitem->data(GUID).toUuid());
            hwitem->setIcon(QIcon(controller->isHardwareProfile(profile) ? ":/img/icon_profile_hardware.png" : ":/img/icon_profile.png"));
        }
        // No need to repopulate here either
        return;
    } else if(result == moveup){
        controller->moveUp(currentProfile);
    } else if(result == movedown){
        controller->moveDown(currentProfile);
    }
    repopulate();
}

void KbProfileDialog::on_exportButton_clicked(){
    // Selected items
    QList<QListWidgetItem*> selectedItems = ui->profileList->selectedItems();

    // Set up the file dialog
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("ckb-next profiles (*.ckb)"));
    dialog.setViewMode(QFileDialog::List);
    dialog.setAcceptMode(QFileDialog::AcceptSave);

    // Generate a default filename
    QStringList names;
    for(QListWidgetItem* item : selectedItems)
        names.append(item->text());
    QString filename = ProfilesController::exportFileName(names);

    if(!filename.isEmpty())
        dialog.selectFile(filename);

    if(!dialog.exec())
        return;

    QStringList filenames = dialog.selectedFiles();

    if(filenames.empty())
        return;

    // Pick only the first filename
    QList<KbProfile*> profiles;
    for(QListWidgetItem* item : selectedItems)
        profiles.append(device->find(item->data(GUID).toUuid()));
    bool compress = controller->exportProfiles(profiles, filenames.at(0));

    if(!compress)
        QMessageBox::warning(this, tr("Error"), tr("An error occurred while exporting the selected profiles."), QMessageBox::Ok);
    else
        QMessageBox::information(this, tr("Export Successful"), tr("Selected profiles have been exported successfully."), QMessageBox::Ok);
}

void KbProfileDialog::on_importButton_clicked(){
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("ckb-next profiles (*.ckb)"));

    if(!dialog.exec())
        return;

    QStringList filenames = dialog.selectedFiles();

    // Pick only the first filename
    const QString& filename = filenames.at(0);
    switch(controller->openPackage(filename)){
    case ProfilesController::NotAPackage:
        QMessageBox::warning(this, tr("Error"), tr("Selected file is not a valid profile."), QMessageBox::Ok);
        return;
    case ProfilesController::CannotRead:
        QMessageBox::warning(this, tr("Error"), tr("Could not read %1.").arg(filename), QMessageBox::Ok);
        return;
    case ProfilesController::InvalidMetadata:
        QMessageBox::warning(this, tr("Error"), tr("Package contains invalid metadata."), QMessageBox::Ok);
        return;
    case ProfilesController::TooNew:
        QMessageBox::warning(this, tr("Error"), tr("Unsupported package selected. Please update ckb-next to import it."), QMessageBox::Ok);
        return;
    case ProfilesController::WrongDeviceType:
        QMessageBox::warning(this, tr("Error"), tr("This profile was not created for the current device type."), QMessageBox::Ok);
        return;
    default:
        break;
    }

    if(controller->importDeviceDiffers()){
        const QStringList& n = controller->importDeviceNames();
        // Only identical devices are supported for now, but let the user override this if they want
        int ret = QMessageBox::question(this, tr("Profile Import"),
                                              tr("This profile was created for a %1 (%2)\nbut it is going to be imported to a %3 (%4).\n\n"
                                                   "You may need to manually add some keys to the appropriate animations.\n\n"
                                                   "Import Anyway?").arg(n.at(0), n.at(1), n.at(2), n.at(3)),
                                        QMessageBox::Yes,
                                        QMessageBox::No);

        if(ret == QMessageBox::No){
            controller->closePackage();
            return;
        }
    }

    if(controller->readPackageProfiles() == ProfilesController::NoValidProfiles){
        QMessageBox::warning(this, tr("Error"), tr("Selected package contains no valid profiles."), QMessageBox::Ok);
        return;
    }

    // TODO use a proper widget with a profile picker
    // For now, import everything included
    if(controller->importHasCorruptProfiles())
        QMessageBox::warning(this, tr("Error"), tr("Selected package contains some corrupt profiles.\n\nAn attempt will be made to import as many as possible."), QMessageBox::Ok);

    const QStringList& profilestr = controller->importNames();
    QString profilestrf(profilestr.join("\n• "));
    int msgret = QMessageBox::information(this, tr("Profile Import"),
                                   tr("The following profiles will be imported.\n\n• ").append(profilestrf),
                                   QMessageBox::Ok,
                                   QMessageBox::Cancel);
    if(msgret == QMessageBox::Cancel){
        controller->closePackage();
        repopulate();
        return;
    }

    controller->beginImport();
    for(int i = 0; i < profilestr.count(); i++){
        ProfilesController::ImportChoice choice = ProfilesController::NoConflict;
        if(controller->importConflict(i)){
            int ret = QMessageBox::question(this, tr("Profile Import"),
                                            profilestr.at(i) + tr(" already exists. Overwrite it?"),
                                            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                            QMessageBox::Cancel);
            if(ret == QMessageBox::Cancel)
                choice = ProfilesController::Skip;
            else if(ret == QMessageBox::Yes)
                choice = ProfilesController::Overwrite;
            else if(ret == QMessageBox::No)
                choice = ProfilesController::KeepBoth;
        }
        controller->importProfile(i, choice);
    }
    controller->finishImport();

    QMessageBox::information(this, tr("Import Successful"), tr("Profiles have been imported successfully."), QMessageBox::Ok);

    // Clean up
    controller->closePackage();
    repopulate();
}

void KbProfileDialog::on_profileList_itemSelectionChanged(){
    // Only change the profile if a single item is selected
    if(ui->profileList->selectedItems().count() > 1)
        return;

    QList<QListWidgetItem*> itemlist = ui->profileList->selectedItems();
    if(itemlist.empty())
        return;

    KbProfile* profile = device->find(itemlist.first()->data(GUID).toUuid());
    if(!profile)
        return;

    controller->select(profile);
}
