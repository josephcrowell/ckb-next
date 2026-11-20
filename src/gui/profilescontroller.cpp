#include "profilescontroller.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QSettings>
#include <QTemporaryDir>
#include <QTextStream>
#include <JlCompress.h>
#include <ckbnextconfig.h>
#include "ckbsettings.h"
#include "kb.h"

ProfilesController::ProfilesController(Kb* device, QObject* parent) :
    QObject(parent), _device(device)
{
}

ProfilesController::~ProfilesController(){
    // Only reached with a package open if the UI went away while asking
    for(int i = 0; i < _importSettings.count(); i++)
        delete _importSettings.at(i).first;
}

bool ProfilesController::isHardwareProfile(KbProfile* profile) const {
    return profile == _device->hwProfile();
}

void ProfilesController::reorder(const QList<QUuid>& order){
    // Rebuild profile list from items
    QList<KbProfile*> newProfiles;
    foreach(const QUuid& id, order){
        KbProfile* profile = _device->find(id);
        if(profile && !newProfiles.contains(profile))
            newProfiles.append(profile);
    }
    // Add any missing profiles at the end of the list
    foreach(KbProfile* profile, _device->profiles()){
        if(!newProfiles.contains(profile))
            newProfiles.append(profile);
    }
    _device->profiles(newProfiles);
}

void ProfilesController::select(KbProfile* profile){
    _device->setCurrentProfile(profile);
}

KbProfile* ProfilesController::addProfile(){
    KbProfile* newProfile = _device->newProfileWithBlankMode();
    _device->appendProfile(newProfile);
    return newProfile;
}

QString ProfilesController::rename(KbProfile* profile, const QString& name){
    profile->name(name);
    return profile->name();
}

bool ProfilesController::canDelete() const {
    // Can't delete the last profile on the device
    return _device->profiles().count() > 1;
}

bool ProfilesController::canMoveUp(KbProfile* profile) const {
    return _device->indexOf(profile) != 0;
}

bool ProfilesController::canMoveDown(KbProfile* profile) const {
    return _device->indexOf(profile) < _device->profiles().count() - 1;
}

bool ProfilesController::canSaveToHardware() const {
    return _device->hwload;
}

void ProfilesController::duplicate(KbProfile* profile){
    QList<KbProfile*> profiles = _device->profiles();
    const int index = _device->indexOf(profile);
    KbProfile* newProfile = _device->newProfile(profile);
    newProfile->newId();
    profiles.insert(index + 1, newProfile);
    _device->profiles(profiles);
    _device->setCurrentProfile(newProfile);
}

void ProfilesController::remove(KbProfile* profile){
    QList<KbProfile*> profiles = _device->profiles();
    const int index = _device->indexOf(profile);
    profiles.removeAll(profile);
    _device->profiles(profiles);
    profile->deleteLater();
    // Select next profile
    if(index < profiles.count())
        _device->setCurrentProfile(profiles.at(index));
    else
        _device->setCurrentProfile(profiles.last());
}

void ProfilesController::moveUp(KbProfile* profile){
    QList<KbProfile*> profiles = _device->profiles();
    const int index = _device->indexOf(profile);
    profiles.removeAll(profile);
    profiles.insert(index - 1, profile);
    _device->profiles(profiles);
}

void ProfilesController::moveDown(KbProfile* profile){
    QList<KbProfile*> profiles = _device->profiles();
    const int index = _device->indexOf(profile);
    profiles.removeAll(profile);
    profiles.insert(index + 1, profile);
    _device->profiles(profiles);
}

void ProfilesController::saveToHardware(){
    _device->hwSave();
}

QString ProfilesController::exportFileName(const QStringList& profileNames){
    QString filename;
    int itemcount = profileNames.count();
    for(int i = 0; i < itemcount; i++){
        QString pname(profileNames.at(i));
        pname.replace(QRegularExpression("[^a-zA-Z0-9_]"), QString());
        if(pname.isEmpty())
            continue;
        filename.append(pname);
        if(i < itemcount - 1)
            filename.append("_");
    }
    return filename;
}

void ProfilesController::removeExtracted(){
    _extracted.clear();
    _importDir.reset();
}

bool ProfilesController::exportProfiles(const QList<KbProfile*>& profiles, QString filename){
    const quint16 profileVer = CKB_NEXT_PROFILE_VER;
    if(!filename.endsWith(".ckb"))
        filename.append(".ckb");

    // The package's files are written to a private folder, removed on return
    QTemporaryDir tmpDir;
    if(!tmpDir.isValid())
        return false;

    QStringList tmpExported;

    {
        // Generate package metadata
        QString metadatapath = tmpDir.filePath("ckbpkg.metadata");
        QSettings metadata(metadatapath, QSettings::IniFormat);

        metadata.beginGroup("metadata");
        metadata.setValue("version", profileVer);
        metadata.setValue("isKeyboard", _device->isKeyboard());
        metadata.setValue("isMouse", _device->isMouse());
        metadata.setValue("isMousepad", _device->isMousepad());
        metadata.setValue("isHeadsetStand", _device->isHeadsetStand());
        metadata.setValue("model", KeyMap::getModel(_device->model()));
        metadata.setValue("layout", _device->getCurrentLayout());
        metadata.setValue("monochrome", _device->monochrome);
        metadata.endGroup();

        metadata.sync();

        if(metadata.status() == QSettings::NoError)
            tmpExported.append(metadatapath);
    }

    // Create a QSettings ini for each selected profile
    for(int p = 0; p < profiles.count(); p++){
        KbProfile* prof = profiles.at(p);

        QString tmp = tmpDir.filePath(QString("ckbprofile%1.ini").arg(p));

        // Used to make sure QSettings is destroyed before trying to hash the ini
        {
            // The version passed here isn't used anywhere
            CkbExternalSettings exportitem(tmp, QSettings::IniFormat, CKB_NEXT_PROFILE_VER);
            exportitem.clear();
            prof->save(exportitem);
            exportitem.sync();

            if(exportitem.status() == QSettings::NoError)
                tmpExported.append(tmp);
        }
    }

    int fileCount = tmpExported.count();

    for(int p = 0; p < fileCount; p++){
        // Create a hash file for each generated file
        QString tmp(tmpExported.at(p));
        QFile tmpHashFile(tmp);
        if(tmpHashFile.open(QFile::ReadOnly)){
            QCryptographicHash tmpHash(QCryptographicHash::Sha256);
            if(tmpHash.addData(&tmpHashFile)){
                tmp.append(".s256");
                QFile tmpHashFileW(tmp);
                if(tmpHashFileW.open(QFile::WriteOnly)){
                    tmpExported.append(tmp);
                    QTextStream hashStream(&tmpHashFileW);
                    hashStream << tmpHash.result().toHex() << "\n";
                }
            }
        }
    }

    return JlCompress::compressFiles(filename, tmpExported);
}

bool ProfilesController::verifyHash(const QString& file){
    QFile iniFile(file);
    QFile hashFile(file + ".s256");
    if(iniFile.open(QFile::ReadOnly) && hashFile.open(QFile::ReadOnly)){
        // Max sha256 string length is 64
        QByteArray pkgHash = QByteArray::fromHex(hashFile.read(64).data());

        QCryptographicHash iniHash(QCryptographicHash::Sha256);
        if(iniHash.addData(&iniFile)){
            hashFile.close();
            iniFile.close();
            if(iniHash.result() == pkgHash)
                return true;
        }
    }
    return false;
}

ProfilesController::ImportStatus ProfilesController::openPackage(const QString& filename){
    _extracted.clear();
    _importDiffers = false;
    _importDeviceNames.clear();
    _importSettings.clear();
    _importNames.clear();
    _importCorrupt = false;
    _importList.clear();

    if(!filename.endsWith(".ckb"))
        return NotAPackage;

    // Extract the ckb package to a private folder
    _importDir.reset(new QTemporaryDir);
    if(!_importDir->isValid()){
        removeExtracted();
        return CannotRead;
    }
    _extracted = JlCompress::extractDir(filename, _importDir->path());

    if(_extracted.empty()){
        removeExtracted();
        return CannotRead;
    }

    // Check and read the metadata
    const QString metadataPath = _importDir->filePath("ckbpkg.metadata");
    if(!verifyHash(metadataPath)){
        removeExtracted();
        return InvalidMetadata;
    }
    QSettings metadata(metadataPath, QSettings::IniFormat);
    metadata.beginGroup("metadata");

    // Pkg version check
    _importVersion = metadata.value("version").toInt();
    if(CKB_NEXT_PROFILE_VER < _importVersion){
        removeExtracted();
        return TooNew;
    }

    // Don't allow importing profiles of devices that aren't the same type
    if(_device->isKeyboard() != metadata.value("isKeyboard").toBool() ||
       _device->isMouse() != metadata.value("isMouse").toBool() ||
       _device->isMousepad() != metadata.value("isMousepad").toBool() ||
       _device->isHeadsetStand() != metadata.value("isHeadsetStand").toBool()){
        removeExtracted();
        return WrongDeviceType;
    }

    QString metadataDevModel = metadata.value("model").toString();
    QString currentDevModel = KeyMap::getModel(_device->model());

    int currentDevLayout = (int)_device->getCurrentLayout();
    int metadataDevLayout = metadata.value("layout").toInt();

    if(currentDevModel != metadataDevModel ||
        currentDevLayout != metadataDevLayout){

        currentDevModel[0] = currentDevModel[0].toUpper();
        metadataDevModel[0] = metadataDevModel[0].toUpper();

        QString metadataDevLayoutStr = KeyMap::getLayout((KeyMap::Layout)metadataDevLayout).replace("_", " ").toUpper();
        QString currentDevLayoutStr = KeyMap::getLayout((KeyMap::Layout)currentDevLayout).replace("_", " ").toUpper();

        // Only identical devices are supported for now, but the user can override this
        _importDiffers = true;
        _importDeviceNames << metadataDevModel << metadataDevLayoutStr << currentDevModel << currentDevLayoutStr;
    }

    metadata.endGroup();
    return ImportOk;
}

ProfilesController::ImportStatus ProfilesController::readPackageProfiles(){
    int pkgProfileCount = 0;
    int pkgVerifiedCount = 0;

    for(int i = 0; i < _extracted.count(); i++){
        const QString& tmpFile = _extracted.at(i);
        if(tmpFile.endsWith("ini")){
            pkgProfileCount++;
            if(verifyHash(tmpFile)){
                CkbExternalSettings* sptr = new CkbExternalSettings(_extracted.at(i), QSettings::IniFormat, _importVersion);
                QString guid(sptr->childGroups().first());

                _importSettings.append(QPair<CkbExternalSettings*, QString>(sptr, guid));
                _importNames.append(sptr->value(guid + "/Name").toString());
                pkgVerifiedCount++;
            }
        }
    }

    if(pkgVerifiedCount == 0){
        removeExtracted();
        return NoValidProfiles;
    }

    // For now, import everything included
    _importCorrupt = (pkgVerifiedCount != pkgProfileCount);
    return ImportOk;
}

void ProfilesController::beginImport(){
    _importList = _device->profiles();
    _replaced.clear();
}

KbProfile* ProfilesController::importConflict(int index) const {
    CkbExternalSettings* sptr = _importSettings.at(index).first;
    QUuid guid(sptr->childGroups().first().trimmed());
    foreach(KbProfile* profile, _device->profiles()){
        if(profile->id().guid == guid)
            return profile;
    }
    return nullptr;
}

void ProfilesController::importProfile(int index, ImportChoice choice){
    if(choice == Skip)
        return;

    CkbExternalSettings* sptr = _importSettings.at(index).first;
    KbProfile* replaced = nullptr;
    if(choice == Overwrite){
        replaced = importConflict(index);
        _importList.removeAll(replaced);
    }

    QString profileGuid = sptr->childGroups().first();

    if(choice == KeepBoth)
        profileGuid = QUuid::createUuid().toString();

    qDebug() << "Importing" << _importNames.at(index);
    KbProfile* newProfile = _device->newProfile(sptr, profileGuid);
    _importList.append(newProfile);
    if(replaced)
        _replaced.append(qMakePair(replaced, newProfile));
}

void ProfilesController::finishImport(){
    _device->profiles(_importList);
    _importList.clear();
    // The overwritten profiles are no longer listed: the device must not keep using them
    QList<KbProfile*> deleted;
    for(const QPair<KbProfile*, KbProfile*>& pair : std::as_const(_replaced)){
        if(!pair.first || deleted.contains(pair.first))
            continue;
        if(pair.first == _device->currentProfile())
            _device->setCurrentProfile(pair.second);
        pair.first->deleteLater();
        deleted.append(pair.first);
    }
    _replaced.clear();
}

void ProfilesController::closePackage(){
    // Destroy the open CkbExternalSettings objects
    for(int i = 0; i < _importSettings.count(); i++)
        delete _importSettings.at(i).first;
    _importSettings.clear();

    removeExtracted();
}
