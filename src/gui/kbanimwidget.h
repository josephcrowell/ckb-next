#ifndef KBANIMWIDGET_H
#define KBANIMWIDGET_H

#include <QListWidgetItem>
#include <QTimer>
#include <QWidget>
#include "animscript.h"
#include "kbanim.h"
#include "kblight.h"

class LightController;

namespace Ui {
class KbAnimWidget;
}

class KbAnimWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KbAnimWidget(QWidget *parent = nullptr);
    ~KbAnimWidget();

    // Call once before use; the list follows the controller's light
    void setController(LightController* newController);
    // Lists a new animation (from LightController::addAnimation) and opens its settings
    void addAnim(KbAnim* animation);
    void duplicateAnim(KbAnim* old);

    void clearSelection();

    void setSelectedKeys(const QStringList& keys);

signals:
    void animChanged(KbAnim* selection);
    void didUpdateSelection(QStringList keys);

public slots:
    void refreshList();

private slots:
    void on_animList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *prev);
    void on_animList_itemChanged(QListWidgetItem *item);
    void on_animList_customContextMenuRequested(const QPoint &pos);
    void on_nameBox_textEdited(const QString &arg1);
    void on_opacityBox_valueChanged(double arg1);
    void on_blendBox_activated(int index);
    void on_keyButton_clicked();
    void on_deleteButton_clicked();

    void reorderAnims();


    void on_propertyButton_clicked();

    void on_nameBox_editingFinished();

private:
    LightController* controller;
    QHash<QUuid, KbAnim*> animations;

    KbAnim* current;
    void setCurrent(KbAnim* newCurrent);
    QStringList selectedKeys;
    bool noReorder;

    Ui::KbAnimWidget *ui;
};

#endif // KBANIMWIDGET_H
