#include <QStandardPaths>
#include "rebindwidget.h"
#include "ui_rebindwidget.h"
#include <QCheckBox>
#include <QDebug>
#include <QMessageBox>
#include "appcontroller.h"
#include "macrostringeditdialog.h"
#include "rebindcontroller.h"
#include <QScrollBar>

static inline void setLocalUiElementsEnabled(Ui::RebindWidget* const ui, const bool e){
    ui->editAsStringBtn->setEnabled(e);
    ui->btnAddEvent->setEnabled(e);
    ui->btnRemoveEvent->setEnabled(e);
    ui->btnClearMacro->setEnabled(e);
}

static inline void setUiElementsEnabled(Ui::RebindWidget* const ui, const bool e){
    ui->applyButton->setEnabled(e);
    ui->resetButton->setEnabled(e);
    ui->unbindButton->setEnabled(e);
    ui->rb_delay_asTyped->setEnabled(e);
    ui->rb_delay_default->setEnabled(e);
    ui->rep_debounce_input->setEnabled(e);
    ui->rep_delay_input->setEnabled(e);
    ui->captureTypeBox->setEnabled(e);
    AppController::instance()->setTabsEnabled(e);
    ui->tabWidget->tabBar()->setEnabled(e);
    ui->cancelButton->setEnabled(e);
    setLocalUiElementsEnabled(ui, e);
}

RebindWidget::RebindWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RebindWidget), controller(new RebindController(this)),
    macroDelegate(new MacroDropdownDelegate)
{
    ui->setupUi(this);
    ui->lightWrapBox->hide();
    ui->modeWrapBox->hide();

    // Populate boxes
    foreach(const QString& name, controller->keyNames(RebindController::ModifierKeys))
        ui->modBox->addItem(name);
    foreach(const QString& name, controller->keyNames(RebindController::FunctionKeys))
        ui->fnBox->addItem(name);
    foreach(const QString& name, controller->keyNames(RebindController::NumpadKeys))
        ui->numBox->addItem(name);
    foreach(const QString& name, controller->keyNames(RebindController::MediaKeys))
        ui->mediaBox->addItem(name);
#ifdef Q_OS_MACOS
    // Add tip label
    ui->progTipLabel->setText("Tip: use the open command to launch a file, directory, or app. For instance, to start Safari:\n  open /Applications/Safari.app");
#else
    ui->progTipLabel->setText(tr("Tip: use xdg-open to launch a file or directory. For instance, to open your home folder:\n  xdg-open ") + QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
#endif
    MacroTableModel* macroLines = controller->macros();
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &RebindWidget::tabChanged);
    ui->tableView->setModel(macroLines);
    ui->tableView->setColumnWidth(0, 24);
    ui->tableView->setItemDelegateForColumn(0, macroDelegate);
    connect(ui->tableView->verticalScrollBar(), &QScrollBar::rangeChanged, this, [this](int min, int max){
        ui->tableView->verticalScrollBar()->setValue(max);
    });

    // Detect when the user closes the editor. Needed for when they press esc after they have added a new row manually
    connect(ui->tableView->itemDelegate(), &QAbstractItemDelegate::closeEditor, this, [this](QWidget* e, QAbstractItemDelegate::EndEditHint h = QAbstractItemDelegate::NoHint){
        setLocalUiElementsEnabled(ui, true);
        ui->btnStartMacro->setEnabled(true);
        if(h != QAbstractItemDelegate::RevertModelCache)
            return;
        controller->macros()->removeEmptyRowAtEnd();
    });
    connect(macroLines, &QAbstractTableModel::dataChanged, this, &RebindWidget::regeneratePreview);
    connect(macroLines, &QAbstractTableModel::modelReset, this, &RebindWidget::regeneratePreview);
    ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    // Macro recording
    connect(controller, &RebindController::lineRecorded, this, &RebindWidget::macroLineRead);
    // Only emitted while recording, when the preview eats the key events
    connect(ui->macroPreview, &NoUserInputTextEdit::macroKeyEvent, controller, &RebindController::recordKeyEvent);
    // Pause macro recording if an error is detected (unknown keypress)
    connect(controller, &RebindController::recordingError, this, [this](QString k, QString m) {
        ui->macroPreview->eatKeyEvents(false);
        ui->macroPreview->releaseKeyboard();
        QMessageBox::warning(this, tr("Unknown key combination pressed"), tr("An unknown key combination (%1, %2) has been pressed.\n"
                                                                 "Make sure your keyboard layout is set to English - United States while recording macros.")
                             .arg(k, m));
        ui->macroPreview->grabKeyboard();
        ui->macroPreview->eatKeyEvents(true);
        controller->resetRecordingTimer();
    });
}

RebindWidget::~RebindWidget(){
    // Clean things up if the daemon died while we were recording a macro
    // (the controller stops reading)
    if(controller->isRecording()){
        // re-enable everything
        setUiElementsEnabled(ui, true);
    }
    delete ui;
    delete macroDelegate;
}

void RebindWidget::setBind(KbBind* newBind, KbProfile* newProfile){
    controller->setBind(newBind, newProfile);
    setSelection(QStringList());

    // Populate typing keys by position
    ui->typingBox->clear();
    ui->typingBox->addItem(" ");
    foreach(const QString& name, controller->keyNames(RebindController::TypingKeys))
        ui->typingBox->addItem(name);

    // Populate mode list
    ui->modeBox->clear();
    ui->modeBox->addItem(" ");
    foreach(const QString& name, controller->modeNames())
        ui->modeBox->addItem(name);

    // Enable/disable DPI based on device
    if(newBind->isMouse()){
        ui->dpiButton->setEnabled(true);
        ui->dpiBox->setEnabled(true);
        ui->dpiWarning->hide();
        // Fill DPI slots
        const QStringList dpiNames = controller->dpiNames();
        for(int i = 0; i < dpiNames.count(); i++)
            ui->dpiBox->setItemText(i + RebindController::DPI_OFFSET, dpiNames.at(i));
    } else {
        ui->dpiButton->setEnabled(false);
        ui->dpiBox->setEnabled(false);
        ui->dpiWarning->show();
    }
    // Always disable custom DPI boxes until selected
    ui->dpiCustXBox->setEnabled(false);
    ui->dpiCustYBox->setEnabled(false);
    ui->dpiCustLabel->setEnabled(false);
}

void RebindWidget::setSelection(const QStringList& newSelection, bool applyPrevious){
    // Apply changes to previous selection (if any were made). This comes before refilling the
    // animation list, which resets the other controls.
    if(!controller->selection().isEmpty() && applyPrevious){
        QStringList previous = controller->selection();
        controller->setSelection(QStringList());
        if(controller->isChanged(currentForm()))
            applyChanges(previous, false);
    }

    if(controller->bind()){
        // Populate animation list
        // FIXME: There should be a trigger to update this whenever an animation is added/removed, even if the key selection hasn't changed
        ui->animBox->clear();
        ui->animBox->addItem(" ");
        foreach(const QString& name, controller->animationNames())
            ui->animBox->addItem(name);
    }

    controller->setSelection(newSelection);
    if(newSelection.isEmpty()){
        setEnabled(false);
        return;
    } else
        setEnabled(true);

    const RebindController::ActionForm form = controller->form(controller->commonAction());
    // Clear everything
    setBox(nullptr);
    ui->dpiCustXBox->setValue(400);
    ui->dpiCustYBox->setValue(400);
    ui->animOnceBox->setChecked(false);
    ui->animKrBox->setChecked(false);
    ui->programKpBox->setText("");
    ui->programKrBox->setText("");
    ui->programKpSIBox->setChecked(false);
    ui->programKrSIBox->setChecked(false);
    ui->programKpModeBox->setCurrentIndex(0);
    ui->programKrModeBox->setCurrentIndex(0);
    ui->programKpModeBox->setEnabled(false);
    ui->programKrModeBox->setEnabled(false);
    // Clear new UI elements in MacroTab
    ui->macroPreview->clear();
    ui->macroName->clear();
    controller->macros()->clear();
    // Fill in field and select tab according to action type
    switch(form.kind){
    case RebindController::ActionForm::Mouse:
        // Mouse buttons
        ui->tabWidget->setCurrentIndex(form.tab);
        ui->mbBox->setCurrentIndex(form.mouse);
        ui->mb2Box->setCurrentIndex(form.mouseExtra);
        ui->wheelBox->setCurrentIndex(form.wheel);
        if(form.dpi > 0){
            ui->dpiBox->setCurrentIndex(form.dpi);
            if(form.dpi == RebindController::DPI_CUSTOM_INDEX){
                ui->dpiCustXBox->setValue(form.dpiCustom.x());
                ui->dpiCustYBox->setValue(form.dpiCustom.y());
            }
        }
        break;
    case RebindController::ActionForm::Key:
        // Standard key
        ui->tabWidget->setCurrentIndex(form.tab);
        ui->typingBox->setCurrentIndex(form.typing);
        ui->modBox->setCurrentIndex(form.modifier);
        ui->fnBox->setCurrentIndex(form.function);
        ui->numBox->setCurrentIndex(form.numpad);
        ui->mediaBox->setCurrentIndex(form.media);
        break;
    case RebindController::ActionForm::Program:
        // Program key
        ui->tabWidget->setCurrentIndex(form.tab);
        ui->programKpBox->setText(form.programPress);
        ui->programKrBox->setText(form.programRelease);
        ui->programKpSIBox->setChecked(form.pressSingle);
        ui->programKpModeBox->setCurrentIndex(form.pressMode);
        ui->programKpModeBox->setEnabled(form.pressSingle);
        ui->programKrSIBox->setChecked(form.releaseSingle);
        ui->programKrModeBox->setCurrentIndex(form.releaseMode);
        ui->programKrModeBox->setEnabled(form.releaseSingle);
        break;
    case RebindController::ActionForm::Animation:
        // Animation key
        ui->tabWidget->setCurrentIndex(form.tab);
        ui->animBox->setCurrentIndex(form.animation);
        ui->animOnceBox->setChecked(form.animRepeat);
        ui->animKrBox->setChecked(form.animStopOnRelease);
        break;
    case RebindController::ActionForm::Special:
        // Other special keys
        ui->tabWidget->setCurrentIndex(RebindController::TAB_SPECIAL);
        ui->modeWrapBox->setChecked(true);
        if(form.special == "mode"){
            ui->modeWrapBox->setChecked(form.modeWrap);
            ui->modeBox->setCurrentIndex(form.mode);
        } else if(form.special == "macro"){
            ui->tabWidget->setCurrentIndex(form.tab);
            if(form.macroRead){
                ui->macroName->setText(form.macroName);
                controller->loadMacro(form);
                if(form.macroAsTyped)
                    ui->rb_delay_asTyped->setChecked(true);
                else
                    ui->rb_delay_default->setChecked(true);
                ui->rep_debounce_input->setValue(form.macroDebounce.toInt());
                ui->rep_delay_input->setValue(form.macroDelay.toInt());
            }
        } else
            ui->modeBox->setCurrentIndex(0);
        // Brightness control
        if(form.special == "light"){
            ui->lightWrapBox->setChecked(form.lightWrap);
            ui->lightBox->setCurrentIndex(form.light);
        } else
            ui->lightBox->setCurrentIndex(0);
        // Win lock
        ui->lockBox->setCurrentIndex(form.lock);
        break;
    case RebindController::ActionForm::None:
        break;
    }
    controller->setLoaded(currentForm());
}

RebindController::ActionForm RebindWidget::currentForm() const {
    RebindController::ActionForm form;
    form.typing = ui->typingBox->currentIndex();
    form.modifier = ui->modBox->currentIndex();
    form.function = ui->fnBox->currentIndex();
    form.numpad = ui->numBox->currentIndex();
    form.media = ui->mediaBox->currentIndex();
    form.mouse = ui->mbBox->currentIndex();
    form.mouseExtra = ui->mb2Box->currentIndex();
    form.wheel = ui->wheelBox->currentIndex();
    form.dpi = ui->dpiBox->currentIndex();
    form.dpiCustom = QPoint(ui->dpiCustXBox->value(), ui->dpiCustYBox->value());
    form.mode = ui->modeBox->currentIndex();
    form.modeWrap = ui->modeWrapBox->isChecked();
    form.light = ui->lightBox->currentIndex();
    form.lightWrap = ui->lightWrapBox->isChecked();
    form.lock = ui->lockBox->currentIndex();
    form.animation = ui->animBox->currentIndex();
    form.animRepeat = ui->animOnceBox->isChecked();
    form.animStopOnRelease = ui->animKrBox->isChecked();
    form.programPress = ui->programKpBox->text();
    form.programRelease = ui->programKrBox->text();
    form.pressSingle = ui->programKpSIBox->isChecked();
    form.pressMode = ui->programKpModeBox->currentIndex();
    form.releaseSingle = ui->programKrSIBox->isChecked();
    form.releaseMode = ui->programKrModeBox->currentIndex();
    form.macroName = ui->macroName->text();
    form.macroDebounce = ui->rep_debounce_input->cleanText();
    form.macroDelay = ui->rep_delay_input->cleanText();
    return form;
}

void RebindWidget::applyChanges(const QStringList& keys, bool doUnbind){
    const RebindController::ActionForm form = currentForm();
    // Check if the macro is matched, and if not, warn the user
    const int pos = controller->unmatchedMacroLine(form);
    if(pos > -1){
        const MacroLine& ml = controller->macros()->at(pos);
        const QString msg = (ml.keyDown ?
                                 tr("Key %1 (%2) is pressed but never released.\n"
                               "This will result in the key being pressed by the macro until you manually press the key itself and release it.\n\n"
                               "Are you sure you want to continue?")
                               : tr("Key %1 (%2) is released but never pressed.\n"
                                    "This will have no observable effect unless the key is held down manually or by another macro.\n\n"
                                    "Are you sure you want to continue?"));
        if(QMessageBox::warning(this, tr("Macro warning"),
                             msg.arg(pos + 1).arg(ml.key),
                             QMessageBox::Ok, QMessageBox::Cancel) != QMessageBox::Ok) {
            // Select the row that's causing the warning
            ui->tableView->selectRow(pos);
            return;
        }
    }
    controller->apply(keys, form, doUnbind);
    controller->setLoaded(form);
}

void RebindWidget::on_applyButton_clicked(){
    applyChanges(controller->selection(), true);
}

void RebindWidget::on_cancelButton_clicked(){
    // Re-load selection
    setSelection(QStringList(controller->selection()));
}

void RebindWidget::on_resetButton_clicked(){
    controller->resetSelection();
}

void RebindWidget::on_unbindButton_clicked(){
    controller->unbindSelection();
}

void RebindWidget::setBox(QWidget* box){
    // Un-select every item except for the current one.
    // on_*_currentIndexChanged will take care of deselecting the checkbox.
    // Key
    if(box != ui->typingBox) ui->typingBox->setCurrentIndex(0);
    if(box != ui->modBox) ui->modBox->setCurrentIndex(0);
    if(box != ui->fnBox) ui->fnBox->setCurrentIndex(0);
    if(box != ui->numBox) ui->numBox->setCurrentIndex(0);
    if(box != ui->mediaBox) ui->mediaBox->setCurrentIndex(0);
    // Mouse
    if(box != ui->mbBox) ui->mbBox->setCurrentIndex(0);
    if(box != ui->mb2Box) ui->mb2Box->setCurrentIndex(0);
    if(box != ui->wheelBox) ui->wheelBox->setCurrentIndex(0);
    if(box != ui->dpiBox) ui->dpiBox->setCurrentIndex(0);
    // Anim
    if(box != ui->animBox) ui->animBox->setCurrentIndex(0);
    // Special
    if(box != ui->modeBox) ui->modeBox->setCurrentIndex(0);
    if(box != ui->lightBox) ui->lightBox->setCurrentIndex(0);
    if(box != ui->lockBox) ui->lockBox->setCurrentIndex(0);
    // Program
    if(box != ui->programKpBox && box != ui->programKrBox){
        ui->programKpButton->setChecked(false);
        ui->programKrButton->setChecked(false);
    }
    // Clear macro panel
    if (box != ui->macroPreview) {
        on_btnClearMacro_clicked();
    }
}

void RebindWidget::on_typingBox_currentIndexChanged(int index){
    if(index == 0)
        ui->typingButton->setChecked(false);
    else {
        ui->typingButton->setChecked(true);
        setBox(ui->typingBox);
    }
}

void RebindWidget::on_modBox_currentIndexChanged(int index){
    if(index == 0)
        ui->modButton->setChecked(false);
    else {
        ui->modButton->setChecked(true);
        setBox(ui->modBox);
    }
}

void RebindWidget::on_fnBox_currentIndexChanged(int index){
    if(index == 0)
        ui->fnButton->setChecked(false);
    else {
        ui->fnButton->setChecked(true);
        setBox(ui->fnBox);
    }
}

void RebindWidget::on_numBox_currentIndexChanged(int index){
    if(index == 0)
        ui->numButton->setChecked(false);
    else {
        ui->numButton->setChecked(true);
        setBox(ui->numBox);
    }
}

void RebindWidget::on_mediaBox_currentIndexChanged(int index){
    if(index == 0)
        ui->mediaButton->setChecked(false);
    else {
        ui->mediaButton->setChecked(true);
        setBox(ui->mediaBox);
    }
}

void RebindWidget::on_mbBox_currentIndexChanged(int index){
    if(index == 0)
        ui->mbButton->setChecked(false);
    else {
        ui->mbButton->setChecked(true);
        setBox(ui->mbBox);
    }
}

void RebindWidget::on_mb2Box_currentIndexChanged(int index){
    if(index == 0)
        ui->mb2Button->setChecked(false);
    else {
        ui->mb2Button->setChecked(true);
        setBox(ui->mb2Box);
    }
}

void RebindWidget::on_wheelBox_currentIndexChanged(int index){
    if(index == 0)
        ui->wheelButton->setChecked(false);
    else {
        ui->wheelButton->setChecked(true);
        setBox(ui->wheelBox);
    }
}

void RebindWidget::on_dpiBox_currentIndexChanged(int index){
    if(index != RebindController::DPI_CUSTOM_INDEX){
        // Clear custom DPI
        ui->dpiCustXBox->setEnabled(false);
        ui->dpiCustYBox->setEnabled(false);
        ui->dpiCustLabel->setEnabled(false);
    } else {
        ui->dpiCustXBox->setEnabled(true);
        ui->dpiCustYBox->setEnabled(true);
        ui->dpiCustLabel->setEnabled(true);
    }
    if(index == 0)
        ui->dpiButton->setChecked(false);
    else {
        ui->dpiButton->setChecked(true);
        setBox(ui->dpiBox);
    }
}

void RebindWidget::on_modeBox_currentIndexChanged(int index){
    if(index == 1 || index == 2)
        ui->modeWrapBox->show();
    else
        ui->modeWrapBox->hide();
    if(index == 0)
        ui->modeButton->setChecked(false);
    else {
        ui->modeButton->setChecked(true);
        setBox(ui->modeBox);
    }
}

void RebindWidget::on_lightBox_currentIndexChanged(int index){
    if(index == 0){
        ui->lightButton->setChecked(false);
        ui->lightWrapBox->hide();
    } else {
        ui->lightWrapBox->show();
        ui->lightButton->setChecked(true);
        setBox(ui->lightBox);
    }
}

void RebindWidget::on_lockBox_currentIndexChanged(int index){
    if(index == 0)
        ui->lockButton->setChecked(false);
    else {
        ui->lockButton->setChecked(true);
        setBox(ui->lockBox);
    }
}

void RebindWidget::on_programKpBox_textChanged(const QString &arg1){
    if(arg1.isEmpty()){
        ui->programKpButton->setChecked(false);
    } else {
        ui->programKpButton->setChecked(true);
        setBox(ui->programKpBox);
    }
}

void RebindWidget::on_programKrBox_textChanged(const QString &arg1){
    if(arg1.isEmpty()){
        ui->programKrButton->setChecked(false);
    } else {
        ui->programKrButton->setChecked(true);
        setBox(ui->programKrBox);
    }
}

void RebindWidget::on_animBox_currentIndexChanged(int index){
    if(index == 0)
        ui->animButton->setChecked(false);
    else {
        ui->animButton->setChecked(true);
        setBox(ui->animBox);
    }
}

void RebindWidget::on_typingButton_clicked(bool checked){
    if(checked && ui->typingBox->currentIndex() == 0)
        ui->typingBox->setCurrentIndex(1);
}

void RebindWidget::on_modButton_clicked(bool checked){
    if(checked && ui->modBox->currentIndex() == 0)
        ui->modBox->setCurrentIndex(1);
}

void RebindWidget::on_fnButton_clicked(bool checked){
    if(checked && ui->fnBox->currentIndex() == 0)
        ui->fnBox->setCurrentIndex(1);
}

void RebindWidget::on_numButton_clicked(bool checked){
    if(checked && ui->numBox->currentIndex() == 0)
        ui->numBox->setCurrentIndex(1);
}

void RebindWidget::on_mediaButton_clicked(bool checked){
    if(checked && ui->mediaBox->currentIndex() == 0)
        ui->mediaBox->setCurrentIndex(1);
}

void RebindWidget::on_modeButton_clicked(bool checked){
    if(checked && ui->modeBox->currentIndex() == 0)
        ui->modeBox->setCurrentIndex(1);
}

void RebindWidget::on_lightButton_clicked(bool checked){
    if(checked && ui->lightBox->currentIndex() == 0)
        ui->lightBox->setCurrentIndex(1);
}

void RebindWidget::on_lockButton_clicked(bool checked){
    if(checked && ui->lockBox->currentIndex() == 0)
        ui->lockBox->setCurrentIndex(1);
}

void RebindWidget::on_programKpButton_clicked(bool checked){
    if(!checked){
        ui->programKpBox->setText("");
    } else if(ui->programKpBox->text().isEmpty()){
        ui->programKpBox->setFocus();
        setBox(ui->programKpBox);
    }
}

void RebindWidget::on_programKrButton_clicked(bool checked){
    if(!checked){
        ui->programKrBox->setText("");
    } else if(ui->programKrBox->text().isEmpty()){
        ui->programKrBox->setFocus();
        setBox(ui->programKrBox);
    }
}

void RebindWidget::on_mbButton_clicked(bool checked){
    if(checked && ui->mbBox->currentIndex() == 0)
        ui->mbBox->setCurrentIndex(1);
}

void RebindWidget::on_mb2Button_clicked(bool checked){
    if(checked && ui->mb2Box->currentIndex() == 0)
        ui->mb2Box->setCurrentIndex(1);
}

void RebindWidget::on_wheelButton_clicked(bool checked){
    if(checked && ui->wheelBox->currentIndex() == 0)
        ui->wheelBox->setCurrentIndex(1);
}

void RebindWidget::on_dpiButton_clicked(bool checked){
    if(checked && ui->dpiBox->currentIndex() == 0)
        ui->dpiBox->setCurrentIndex(1);
}

void RebindWidget::on_programKpSIBox_clicked(bool checked){
    ui->programKpModeBox->setCurrentIndex(0);
    ui->programKpModeBox->setEnabled(checked);
}

void RebindWidget::on_programKrSIBox_clicked(bool checked){
    ui->programKrModeBox->setCurrentIndex(0);
    ui->programKrModeBox->setEnabled(checked);
}

void RebindWidget::on_animButton_clicked(bool checked){
    if(checked && ui->animBox->currentIndex() == 0)
        ui->animBox->setCurrentIndex(1);
}

void RebindWidget::on_btnStartMacro_clicked() {
    // Get the capture type
    const RebindController::CaptureType type = static_cast<RebindController::CaptureType>(ui->captureTypeBox->currentIndex());
    // If we're already capturing, stop
    if(controller->isRecording()) {
        if(controller->stopRecording(type, ui->btnStartMacro->isClickedByMouse())){
            QString oldstr = ui->macroPreview->toPlainText();
            oldstr.replace(QRegularExpression("<mouse1>$"), QString());
            ui->macroPreview->setPlainText(oldstr);
        }

        ui->btnStartMacro->setText(tr("Start Recording"));
        ui->lbl_macro->setText(tr("Click Apply or manually edit the events."));
        ui->macroPreview->eatKeyEvents(false);
        ui->macroPreview->releaseKeyboard();
        setUiElementsEnabled(ui, true);

        // If we captured any data, clear all other selections in the widget
        if(controller->macros()->length())
            setBox(ui->macroPreview);
        return;
    }
    // Else, start a new capture
    ui->macroPreview->setFocus();

    // Start the appropriate capture mode
    controller->startRecording(type);

    // Disable the buttons while recording
    ui->btnStartMacro->setText(tr("Stop Recording"));
    ui->lbl_macro->setText(tr("Type your macro and press Stop Recording when finished."));
    ui->macroPreview->eatKeyEvents(true);
    ui->macroPreview->grabKeyboard();
    setUiElementsEnabled(ui, false);
}

void RebindWidget::on_btnClearMacro_clicked() {
    ui->lbl_macro->setText(tr("Click Start Recording or manually edit the events."));
    controller->macros()->clear();
    ui->macroName->clear();
    ui->macroPreview->clear();
    ui->rep_debounce_input->setValue(RebindController::MACRO_DEFAULT_REPETITION_DEBOUNCE);
    ui->rep_delay_input->setValue(RebindController::MACRO_DEFAULT_REPETITION_DELAY);
}

void RebindWidget::on_rb_delay_default_toggled(bool checked){
    controller->macros()->setDefaultDelay(checked);
}

void RebindWidget::insertIntoMacroPreview(const bool keydown, const bool printable, const QString& line){
    if(keydown){
        ui->macroPreview->moveCursor(QTextCursor::End);
        if(printable)
            ui->macroPreview->insertPlainText(line);
        else if(line == QString("space")) // Special case to handle the spacebar
            ui->macroPreview->insertPlainText(" ");
        else
            ui->macroPreview->insertPlainText("<" + line + ">");
    }
}

void RebindWidget::macroLineRead(const QString& line, bool keydown){
    // The controller has appended the event to the macro
    const bool printable = (line.length() == 1);

    // Add keystroke to the key actions textedit
    // "+k", "-k", these can be represented as text
    if(line != "mouse1")
        ui->macroPreview->setFocus();
    insertIntoMacroPreview(keydown, printable, line);
}

void RebindWidget::tabChanged(int idx){
    if(controller->isRecording())
        return;
    // We monitor the tabs, and when we switch to the macro tab, we select the appropriate capture method
    if(ui->tabWidget->currentWidget() == ui->macroTab)
        ui->captureTypeBox->setCurrentIndex(controller->defaultCaptureType());
}

// This is automatically called whenever setCurrentIndex is called programmatically as well
void RebindWidget::on_captureTypeBox_currentIndexChanged(int index){
    controller->setCaptureType(static_cast<RebindController::CaptureType>(index));
    if(controller->captureType() != RebindController::CAPTURE_ALL_KEYBOARDS)
        return;
    if(RebindController::captureAllWarningSuppressed())
        return;
    QMessageBox msg;
    // MessageBox takes ownership of the checkbox, so we don't need to delete it
    msg.setCheckBox(new QCheckBox("Don't warn me again"));
    msg.setIcon(QMessageBox::Warning);
    msg.addButton(QMessageBox::Ok);
    msg.setText(tr("\"Record from all keyboards\" is only recommended if you do not have a keyboard managed by ckb-next.\n"
                   "It currently only functions with an English - United States keyboard layout.\n"
                   "Make sure your keyboard is switched to it before recording."));
    msg.setWindowTitle(tr("Record from all keyboards"));
    msg.exec();
    RebindController::setCaptureAllWarningSuppressed(msg.checkBox()->isChecked());
}

int RebindWidget::regeneratePreview(){
    ui->macroPreview->clear();
    MacroTableModel* macroLines = controller->macros();
    const int len = macroLines->length();
    for(int i = 0; i < len; i++){
        const MacroLine& ml = macroLines->at(i);
        insertIntoMacroPreview(ml.keyDown, ml.isPrintable(), ml.key);
    }

    return len;
}

void RebindWidget::on_editAsStringBtn_clicked(){
    MacroStringEditDialog* dlg = new MacroStringEditDialog(controller->macros(), this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
    connect(dlg, &MacroStringEditDialog::accepted, this, [=](){
        const int len = controller->macros()->length();
        if(len)
            setBox(ui->macroPreview);
    });
}

void RebindWidget::on_btnRemoveEvent_clicked(){
    const QModelIndexList mil = ui->tableView->selectionModel()->selectedRows();
    if(!mil.length())
        return;

    controller->macros()->removeMultipleColumns(mil);
    regeneratePreview();
}

void RebindWidget::on_btnAddEvent_clicked(){
    setLocalUiElementsEnabled(ui, false);
    ui->btnStartMacro->setEnabled(false);
    const QModelIndex idx = controller->macros()->addBlankElement();
    ui->tableView->setCurrentIndex(idx);
    ui->tableView->edit(idx);
}
