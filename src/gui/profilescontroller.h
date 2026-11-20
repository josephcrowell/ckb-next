#ifndef PROFILESCONTROLLER_H
#define PROFILESCONTROLLER_H

#include <QList>
#include <QObject>
#include <QPair>
#include <QStringList>
#include <QUuid>
#include <memory>

class CkbExternalSettings;
class Kb;
class KbProfile;
class QTemporaryDir;

// UI-independent logic of the profile manager, shared by the Widgets and QML UIs. The UI asks
// every question; importing runs in steps so it can ask between them.
class ProfilesController : public QObject
{
    Q_OBJECT
public:
    explicit ProfilesController(Kb* device, QObject* parent = nullptr);
    ~ProfilesController();

    inline Kb* device() const { return _device; }
    bool isHardwareProfile(KbProfile* profile) const;

    // Profile list. reorder() keeps profiles missing from the order at the end.
    void reorder(const QList<QUuid>& order);
    void select(KbProfile* profile);
    // A new profile with a blank mode, appended (not selected)
    KbProfile* addProfile();
    // Returns the stored name (trimmed, "Unnamed" when empty)
    QString rename(KbProfile* profile, const QString& name);
    bool canDelete() const;
    bool canMoveUp(KbProfile* profile) const;
    bool canMoveDown(KbProfile* profile) const;
    bool canSaveToHardware() const;
    // Inserted after the profile and selected
    void duplicate(KbProfile* profile);
    // Deletes the profile (later) and selects the one now at its place, or the last
    void remove(KbProfile* profile);
    void moveUp(KbProfile* profile);
    void moveDown(KbProfile* profile);
    void saveToHardware();

    // Export. The suggested file name joins the profile names (letters, digits and _ only).
    static QString exportFileName(const QStringList& profileNames);
    // Adds ".ckb" if missing; returns false if the package couldn't be written
    bool exportProfiles(const QList<KbProfile*>& profiles, QString fileName);

    // Import, step 1: extract the package and check its metadata
    enum ImportStatus { ImportOk, NotAPackage, CannotRead, InvalidMetadata, TooNew, WrongDeviceType, NoValidProfiles };
    ImportStatus openPackage(const QString& fileName);
    // After step 1: the package is for another model or layout; the names to show the user
    inline bool importDeviceDiffers() const { return _importDiffers; }
    inline const QStringList& importDeviceNames() const { return _importDeviceNames; }
    // Step 2: read the package's profiles (NoValidProfiles if none passes its hash check)
    ImportStatus readPackageProfiles();
    // After step 2: names of the profiles to import; some profiles failed their hash check
    inline const QStringList& importNames() const { return _importNames; }
    inline bool importHasCorruptProfiles() const { return _importCorrupt; }
    // Step 3: beginImport(), then for each profile importConflict() (the existing profile
    // with the same ID, or null) and importProfile() with the user's choice: Overwrite,
    // KeepBoth (new ID) or Skip; NoConflict when there is none. finishImport() stores the list.
    enum ImportChoice { Overwrite, KeepBoth, Skip, NoConflict };
    void beginImport();
    KbProfile* importConflict(int index) const;
    void importProfile(int index, ImportChoice choice);
    void finishImport();
    // Removes the extracted package's folder and closes the package. Called after step 4 and when the
    // user stops; after step 1 failed only the files are removed (done by openPackage).
    void closePackage();

private:
    bool verifyHash(const QString& file);
    // Removes the folder the package was extracted to
    void removeExtracted();

    Kb* _device;

    // Import in progress: the package is extracted to its own temporary folder
    std::unique_ptr<QTemporaryDir> _importDir;
    QStringList _extracted;
    quint16 _importVersion = 0;
    bool _importDiffers = false;
    QStringList _importDeviceNames;
    // <Pointer>, <GUID>
    QList<QPair<CkbExternalSettings*, QString>> _importSettings;
    QStringList _importNames;
    bool _importCorrupt = false;
    QList<KbProfile*> _importList;
};

#endif // PROFILESCONTROLLER_H
