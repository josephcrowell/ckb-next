#ifndef KBMODEEVENTMGR_H
#define KBMODEEVENTMGR_H

#include <QDialog>
#include <QVector>
#include "kbwindowinfo.h"

class QQuickWidget;
class KbWindowInfoModel;

class KbModeEventMgr : public QDialog
{
    Q_OBJECT

public:
    explicit KbModeEventMgr(QWidget* parent, KbWindowInfo* info, const QString& modeName);
    ~KbModeEventMgr();
    bool isValid() const { return valid; }
    virtual void closeEvent(QCloseEvent* evt);

protected:
    void changeEvent(QEvent* evt) override;

private slots:
    void onQmlAccepted(bool enabled);
    void onQmlRejected();

private:
    KbWindowInfo* info;
    KbWindowInfoModel* model;
    QQuickWidget* quick;
    QVector<KbWindowInfo::MatchPair> backup;
    bool valid = false;
};

#endif // KBMODEEVENTMGR_H
