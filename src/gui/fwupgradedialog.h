#ifndef FWUPGRADEDIALOG_H
#define FWUPGRADEDIALOG_H

#include <QCloseEvent>
#include <QDialog>
#include <QEventLoop>
#include "kb.h"

class FirmwareController;

namespace Ui {
class FwUpgradeDialog;
}

class FwUpgradeDialog : public QDialog
{
    Q_OBJECT

public:
    FwUpgradeDialog(QWidget* parent, CkbVersionNumber newV, const QByteArray& fwBlob, Kb* device);
    ~FwUpgradeDialog();

    int exec();

private slots:
    void fwUpdateProgress(int current, int total);
    void fwUpdateFinished(bool succeeded);

    void on_cancelButton_clicked();
    void on_actionButton_clicked();

private:
    Ui::FwUpgradeDialog *ui;
    void closeEvent(QCloseEvent* event);

    FirmwareController* controller;

    // Event loop for synchronous exec()
    QEventLoop* evLoop;
    bool        exitSuccess;
};

#endif // FWUPGRADEDIALOG_H
