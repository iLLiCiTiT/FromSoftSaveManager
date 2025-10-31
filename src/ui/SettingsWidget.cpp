#include "SettingsWidget.h"

#include <iostream>
#include <QLineEdit>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>


SavePathInput::SavePathInput(const QString& gameTitle, QWidget* parent): QFrame(parent) {
    m_gameTitle = QString(gameTitle);

    m_pathInput = new QLineEdit(this);
    m_pathInput->setAttribute(Qt::WA_TranslucentBackground, true);
    m_pathInput->setPlaceholderText("< Path to save file >");

    m_openBtn = new SquareButton(this);
    m_openBtn->setIcon(QIcon(":/icons/folder_256x256.png"));
    m_openBtn->setToolTip("Select save file..");
    m_openBtn->setAttribute(Qt::WA_TranslucentBackground, true);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_pathInput, 1);
    layout->addWidget(m_openBtn, 0);

    connect(m_openBtn, SIGNAL(clicked()), this, SLOT(selectPath()));
};

QString SavePathInput::getPath() {
    return m_pathInput->text();
};

void SavePathInput::updatePath(const QString& path, const QString& hint) {
    m_hint = hint;
    m_pathInput->setText(path);
};

void SavePathInput::selectPath() {
    QString title = "Select '" + m_gameTitle + "' save file";
    QString path = QFileDialog::getOpenFileName(
        this,
        title,
        m_hint,
        tr("Save files (*.sl2);;All files (*.*)")
    );
    if (!path.isEmpty()) m_pathInput->setText(path);
};


SavePathInputsWidget::SavePathInputsWidget(const ConfigSettingsData& configData, QWidget* parent) : QWidget(parent) {
    QLabel* dsrLabel = new QLabel("Dark Souls: Remastered", this);
    QLabel* ds2Label = new QLabel("Dark Souls II: SOTFS", this);
    QLabel* ds3Label = new QLabel("Dark Souls III", this);
    QLabel* sekiroLabel = new QLabel("Sekiro: Shadows Die Twice", this);
    QLabel* erLabel = new QLabel("Elden Ring", this);

    m_dsrPathInput = new SavePathInput("Dark Souls: Remastered", this);
    m_ds2PathInput = new SavePathInput("Dark Souls II: Scholar of the First Sin", this);
    m_ds3PathInput = new SavePathInput("Dark Souls III", this);
    m_sekiroPathInput = new SavePathInput("Sekiro: Shadows Die Twice", this);
    m_erPathInput = new SavePathInput("Elden Ring", this);

    updateConfigInfo(configData);

    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    int row = 0;
    layout->addWidget(dsrLabel, row, 0);
    layout->addWidget(m_dsrPathInput, row++, 1);
    layout->addWidget(ds2Label, row, 0);
    layout->addWidget(m_ds2PathInput, row++, 1);
    layout->addWidget(ds3Label, row, 0);
    layout->addWidget(m_ds3PathInput, row++, 1);
    layout->addWidget(sekiroLabel, row, 0);
    layout->addWidget(m_sekiroPathInput, row++, 1);
    layout->addWidget(dsrLabel, row, 0);
    layout->addWidget(m_dsrPathInput, row++, 1);
    layout->addWidget(erLabel, row, 0);
    layout->addWidget(m_erPathInput, row++, 1);

    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 1);
};

void SavePathInputsWidget::updateConfigInfo(const ConfigSettingsData& configData) {
    m_dsrPathInput->updatePath(
        configData.dsrSavePath.savePath,
        configData.dsrSavePath.savePathHint
    );
    m_ds2PathInput->updatePath(
        configData.ds2SavePath.savePath,
        configData.ds2SavePath.savePathHint
    );
    m_ds3PathInput->updatePath(
        configData.ds3SavePath.savePath,
        configData.ds3SavePath.savePathHint
    );
    m_sekiroPathInput->updatePath(
        configData.sekiroSavePath.savePath,
        configData.sekiroSavePath.savePathHint
    );
    m_erPathInput->updatePath(
        configData.erSavePath.savePath,
        configData.erSavePath.savePathHint
    );
};

void SavePathInputsWidget::applyChanges(const ConfigSettingsData& configData, ConfigConfirmData& confirmData) {
    QString dsrPath = m_dsrPathInput->getPath();
    QString ds2Path = m_ds2PathInput->getPath();
    QString ds3Path = m_ds3PathInput->getPath();
    QString sekiroPath = m_sekiroPathInput->getPath();
    QString erPath = m_erPathInput->getPath();
    if (dsrPath != configData.dsrSavePath.savePath) {
        confirmData.dsrSavePath = dsrPath;
    }
    if (ds2Path != configData.ds2SavePath.savePath) {
        confirmData.ds2SavePath = ds2Path;
    }
    if (ds3Path != configData.ds3SavePath.savePath) {
        confirmData.ds3SavePath = ds3Path;
    }
    if (sekiroPath != configData.sekiroSavePath.savePath) {
        confirmData.sekiroSavePath = sekiroPath;
    }
    if (erPath != configData.erSavePath.savePath) {
        confirmData.erSavePath = erPath;
    }
};

HotkeyInput::HotkeyInput(QWidget* parent): QFrame(parent) {
    setToolTip("Click to set/change hotkey");
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

    m_hotkeyLabel = new QLabel("< Not set >", this);
    m_hotkeyLabel->setAttribute(Qt::WA_TranslucentBackground, true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 0, 0, 0);
    layout->addWidget(m_hotkeyLabel, 0);
};

QKeyCombination HotkeyInput::getKeyCombination() {
    return m_keyCombination;
};
void HotkeyInput::setKeyCombination(const QKeyCombination& combination) {
    m_keyCombination = combination;
    updateLabel();
};

void HotkeyInput::mousePressEvent(QMouseEvent *event) {
    m_tmpCombination = m_keyCombination;
    m_keyCombination = QKeyCombination();
    m_isEditing = true;
    updateLabel();
    grabKeyboard();
};
void HotkeyInput::closeEvent(QCloseEvent *event) {
    if (m_isEditing) {
        m_keyCombination = m_tmpCombination;
        finishEdit();
    }
    QFrame::closeEvent(event);
};
void HotkeyInput::focusOutEvent(QFocusEvent *event) {
    if (m_isEditing) {
        m_keyCombination = m_tmpCombination;
        finishEdit();
    }
    QFrame::focusOutEvent(event);
};
void HotkeyInput::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_Escape:
            m_keyCombination = m_tmpCombination;
            finishEdit();
            break;
        case Qt::Key_Backspace:
        case Qt::Key_Delete:
            m_keyCombination = QKeyCombination();
            finishEdit();
            break;
        case Qt::Key_Control:
        case Qt::Key_Alt:
        case Qt::Key_AltGr:
        case Qt::Key_Meta:
        case Qt::Key_Shift:
            m_keyCombination = QKeyCombination(event->modifiers());
            updateLabel();
            break;
        default:
            m_keyCombination = QKeyCombination(event->keyCombination());
            finishEdit();
    }
};
void HotkeyInput::keyReleaseEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_Control:
        case Qt::Key_Alt:
        case Qt::Key_AltGr:
        case Qt::Key_Meta:
        case Qt::Key_Shift:
            m_keyCombination = QKeyCombination(event->modifiers());
            updateLabel();
            break;
        default:
            QFrame::keyReleaseEvent(event);
    }
};

void HotkeyInput::updateLabel() {
    if (!m_isEditing && m_keyCombination.key() == Qt::Key_unknown) {
        m_hotkeyLabel->setText("< Not set >");
        return;
    }

    if (m_keyCombination.key() == Qt::Key_Enter) {
        Qt::KeyboardModifiers modifiers = m_keyCombination.keyboardModifiers();
        modifiers &= ~Qt::KeyboardModifier::KeypadModifier;
        m_keyCombination = QKeyCombination(modifiers, Qt::Key_Enter);
    }
    bool addedA = false;
    QKeyCombination combination = m_keyCombination;
    if (combination.key() == Qt::Key_unknown) {
        if (combination.keyboardModifiers() == Qt::NoModifier) {
            m_hotkeyLabel->setText("...");
            return;
        }
        addedA = true;
        combination = QKeyCombination(combination.keyboardModifiers(), Qt::Key_A);
    }
    QKeySequence sequence(combination);
    QString label = sequence.toString(QKeySequence::PortableText);
    if (addedA) {
        label.removeLast();
        label += "...";
    }
    m_hotkeyLabel->setText(label);

};
void HotkeyInput::finishEdit() {
    m_isEditing = false;
    releaseKeyboard();
    m_tmpCombination = QKeyCombination();
    updateLabel();
};

HotkeysWidget::HotkeysWidget(const ConfigSettingsData& configData, QWidget* parent): QWidget(parent) {
    QLabel* quickSaveLabel = new QLabel("QuickSave", this);
    QLabel* quickLoadLabel = new QLabel("QuickLoad", this);

    m_quickSaveInput = new HotkeyInput(this);
    m_quickLoadInput = new HotkeyInput(this);

    updateConfigInfo(configData);

    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(quickSaveLabel, 0, 0);
    layout->addWidget(m_quickSaveInput, 0, 1);
    layout->addWidget(quickLoadLabel, 1, 0);
    layout->addWidget(m_quickLoadInput, 1, 1);
    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 1);
    layout->setRowStretch(0, 0);
    layout->setRowStretch(1, 0);
}

void HotkeysWidget::updateConfigInfo(const ConfigSettingsData& configData) {
    m_quickSaveInput->setKeyCombination(configData.quickSaveHotkey);
    m_quickLoadInput->setKeyCombination(configData.quickLoadHotkey);

};
void HotkeysWidget::applyChanges(const ConfigSettingsData& configData, ConfigConfirmData& confirmData) {
    QKeyCombination quickSaveHotkey = m_quickSaveInput->getKeyCombination();
    QKeyCombination quickLoadHotkey = m_quickLoadInput->getKeyCombination();

    if (quickSaveHotkey != configData.quickSaveHotkey) {
        confirmData.quickSaveHotkey = quickSaveHotkey;
    }
    if (quickLoadHotkey != configData.quickLoadHotkey) {
        confirmData.quickLoadHotkey = quickLoadHotkey;
    }
};

AutoBackupWidget::AutoBackupWidget(const ConfigSettingsData& configData, QWidget* parent): QWidget(parent) {
    QLabel* enabledLabel = new QLabel("Enabled", this);

    m_enabledInput = new QCheckBox(this);

    QLabel* frequencyLabel = new QLabel("Frequency", this);
    QLabel* frequencyInputLabel = new QLabel("seconds", this);
    m_frequencyInput = new FocusSpinBox(this);
    m_frequencyInput->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_frequencyInput->setRange(1, 3600);
    m_frequencyInput->setValue(60);

    QLabel* maxAutobackupLabel = new QLabel("Max autobackups", this);
    maxAutobackupLabel->setToolTip("Maximum number of autobackups to keep.\n0 means unlimited.");
    m_maxAutobackupInput = new FocusSpinBox(this);
    m_maxAutobackupInput->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_maxAutobackupInput->setRange(1, 100);
    m_maxAutobackupInput->setValue(10);

    updateConfigInfo(configData);

    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(enabledLabel, 0, 0);
    layout->addWidget(m_enabledInput, 0, 1);
    layout->addWidget(frequencyLabel, 1, 0);
    layout->addWidget(m_frequencyInput, 1, 1);
    layout->addWidget(frequencyInputLabel, 1, 2);
    layout->addWidget(maxAutobackupLabel, 2, 0);
    layout->addWidget(m_maxAutobackupInput, 2, 1);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setVerticalSpacing(5);
    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 1);
};

void AutoBackupWidget::updateConfigInfo(const ConfigSettingsData& configData) {
    m_enabledInput->setChecked(configData.autobackupEnabled);
    m_frequencyInput->setValue(configData.autobackupFrequency);
    m_maxAutobackupInput->setValue(configData.maxAutobackups);
};

void AutoBackupWidget::applyChanges(const ConfigSettingsData& configData, ConfigConfirmData& confirmData) {
    if (m_enabledInput->isChecked() != configData.autobackupEnabled) {
        confirmData.autobackupEnabled = m_enabledInput->isChecked();
    }
    if (m_frequencyInput->value() != configData.autobackupFrequency) {
        confirmData.autobackupFrequency = m_frequencyInput->value();
    }
    if (m_maxAutobackupInput->value() != configData.maxAutobackups) {
        confirmData.maxAutobackups = m_maxAutobackupInput->value();
    }
};

SettingsWidget::SettingsWidget(Controller* controller, QWidget* parent): QWidget(parent), m_controller(controller) {
    m_configData = m_controller->getConfigSettingsData();
    QLabel* pathsLabel = new QLabel("Paths", this);
    pathsLabel->setObjectName("settings_header");
    m_pathsWidget = new SavePathInputsWidget(m_configData, this);

    QLabel* hotkeysLabel = new QLabel("Hotkeys", this);
    hotkeysLabel->setObjectName("settings_header");
    m_hotkeysWidget = new HotkeysWidget(m_configData, this);

    QLabel* autoBackupLabel = new QLabel("Autobackup", this);
    autoBackupLabel->setObjectName("settings_header");
    m_autobackupWidget = new AutoBackupWidget(m_configData, this);

    QWidget* btnsWidget = new QWidget(this);

    QPushButton* saveBtn = new QPushButton("Save", btnsWidget);
    saveBtn->setObjectName("save_btn");
    QPushButton* discardBtn = new QPushButton("Discard changes", btnsWidget);

    QHBoxLayout* btnsLayout = new QHBoxLayout(btnsWidget);
    btnsLayout->setContentsMargins(0, 0, 0, 0);
    btnsLayout->addStretch(1);
    btnsLayout->addWidget(saveBtn, 0);
    btnsLayout->addWidget(discardBtn, 0);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(5);

    layout->addWidget(pathsLabel, 0);
    layout->addWidget(m_pathsWidget, 0);
    layout->addSpacing(10);
    layout->addWidget(hotkeysLabel, 0);
    layout->addWidget(m_hotkeysWidget, 0);
    layout->addSpacing(10);
    layout->addWidget(autoBackupLabel, 0);
    layout->addWidget(m_autobackupWidget, 0);
    layout->addStretch(1);
    layout->addWidget(btnsWidget, 0);

    connect(saveBtn, SIGNAL(clicked()), this, SLOT(onSave()));
    connect(discardBtn, SIGNAL(clicked()), this, SLOT(discardChanges()));
};

void SettingsWidget::onSave() {
    ConfigConfirmData confirmData;
    m_pathsWidget->applyChanges(m_configData, confirmData);
    m_hotkeysWidget->applyChanges(m_configData, confirmData);
    m_autobackupWidget->applyChanges(m_configData, confirmData);
    m_controller->saveConfigData(confirmData);
}

void SettingsWidget::discardChanges() {
    m_configData = m_controller->getConfigSettingsData();
    m_pathsWidget->updateConfigInfo(m_configData);
    m_hotkeysWidget->updateConfigInfo(m_configData);
    m_autobackupWidget->updateConfigInfo(m_configData);
}
