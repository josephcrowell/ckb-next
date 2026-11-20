#include <QMessageBox>
#include "kbbindwidget.h"
#include "bindcontroller.h"
#include "modeselectdialog.h"
#include "rebindwidget.h"
#include "ui_kbbindwidget.h"
#include "keywidgetlayout.h"
#include "keywidget.h"

KbBindWidget::KbBindWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KbBindWidget), controller(new BindController(this)), keyWidget(new KeyWidget(this))
{
    ui->setupUi(this);
    keyWidget->rgbMode(false);
    connect(keyWidget, SIGNAL(selectionChanged(QStringList)), this, SLOT(newSelection(QStringList)));
    connect(controller, &BindController::layoutChanged, this, &KbBindWidget::newLayout);
    connect(controller, &BindController::bindUpdated, this, &KbBindWidget::updateBind);

    // Make a new KeyWidgetLayout that preserves the aspect ratio and add it
    KeyWidgetLayout* l = new KeyWidgetLayout();
    l->addWidget(keyWidget);
    ui->verticalLayout->removeWidget(ui->controlWidget);
    l->addWidget(ui->controlWidget);

    keyWidget->setFocusPolicy(Qt::ClickFocus);
    ui->verticalLayout->insertLayout(2, l);
    ui->verticalLayout->setStretchFactor(l, 1);
    ui->verticalLayout->setStretchFactor(ui->rbWidget, 1);
}

KbBindWidget::~KbBindWidget(){
    delete ui;
}

void KbBindWidget::setBind(KbBind* newBind, KbProfile* newProfile){
    keyWidget->clearSelection();
    ui->rbWidget->setBind(newBind, newProfile);
    newSelection(QStringList());
    if(!controller->setBind(newBind, newProfile))
        return;
    newLayout();
    ui->rbWidget->setBind(controller->bind(), controller->profile());
    updateBind();
}

void KbBindWidget::setControlsEnabled(const bool e){
    ui->resetButton->setEnabled(e);
    ui->copyButton->setEnabled(e);
}

void KbBindWidget::updateBind(){
    keyWidget->bindMap(controller->actions());
    ui->rbWidget->setSelection(controller->selection());
    updateSelDisplay();
}

void KbBindWidget::newLayout(){
    keyWidget->map(controller->bind()->map());
    ui->rbWidget->setBind(controller->bind(), controller->profile());
    updateSelDisplay();
}

void KbBindWidget::newSelection(const QStringList& selection){
    controller->setSelection(selection);
    ui->rbWidget->setSelection(selection, true);
    // Throw a warning if the user is trying to bind a win key while winlock is on
    if(controller->winLockWarning())
        QMessageBox::warning(this, tr("Winlock Warning"), tr("Windows key lock is currently enabled.\n\nThe binding will not function until winlock has been disabled."));
    updateSelDisplay();
}

void KbBindWidget::updateSelDisplay(){
    ui->selectLabel->setText(controller->selectionText());
}

void KbBindWidget::on_resetButton_clicked(){
    if(QMessageBox(QMessageBox::NoIcon, tr("Confirm action"), controller->resetQuestion(), QMessageBox::Yes | QMessageBox::No, this).exec() != QMessageBox::Yes)
        return;
    controller->resetBinds();
    updateBind();
}

void KbBindWidget::on_copyButton_clicked(){
    // Display popup
    KbProfile* profile = controller->profile();
    ModeSelectDialog dialog(this, profile->currentMode(), profile->modes(), controller->copyPrompt());
    if(dialog.exec() != QDialog::Accepted)
        return;
    // Copy selected keys to selected modes
    controller->copyToModes(dialog.selection());
}
