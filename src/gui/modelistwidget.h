#ifndef MODELISTWIDGET_H
#define MODELISTWIDGET_H

#include <QTableView>
#include <QResizeEvent>
#include <QStyledItemDelegate>
#include <QTimer>
#include "kb.h"
#include "modelisttablemodel.h"

class ModeListWidget : public QTableView
{
    Q_OBJECT
public:
    explicit ModeListWidget(QWidget* parent = nullptr);
    // Shows model, which belongs to the device's DeviceController
    void setDevice(Kb* dev, ModeListTableModel* model);
    inline void setIgnoreFocusLoss(const bool e) { ignoreFocusLoss = e; }
    ~ModeListWidget();
signals:
    void orderChanged();

private:
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    Kb* device;
    QTableView* floatingView;
    void updateFloatingGeometry();
    ModeListTableModel* modelptr;
    bool ignoreFocusLoss;
};

// Colours the item background from ModeListTableModel's HighlightedRole and ActiveFocusRole
class ModeListDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
protected:
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;
};

class FloatingDelegate : public ModeListDelegate
{
protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

#endif // MODELISTWIDGET_H
