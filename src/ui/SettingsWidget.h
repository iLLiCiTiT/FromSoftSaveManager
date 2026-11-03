#pragma once

#include <QLineEdit>

#include "Controller.h"
#include "SquareButton.h"
#include "NiceCheckbox.h"
#include "Utils.h"

class SavePathInput: public QFrame {
    Q_OBJECT
public:
    explicit SavePathInput(const QString& gameTitle, QWidget* parent);
    QString getPath();
    void updatePath(const QString& path, const QString& hint);

private slots:
    void virtual selectPath();

private:
    QString m_gameTitle;
    QString m_hint;
    QLineEdit* m_pathInput;
    SquareButton* m_openBtn;
};

class SavePathInputsWidget: public QWidget {
    Q_OBJECT
public:
    explicit SavePathInputsWidget(const ConfigSettingsData& configData, QWidget* parent);
    void updateConfigInfo(const ConfigSettingsData& configData);
    void applyChanges(const ConfigSettingsData& configData, ConfigConfirmData& confirmData);
private:
    SavePathInput* m_dsrPathInput;
    SavePathInput* m_ds2PathInput;
    SavePathInput* m_ds3PathInput;
    SavePathInput* m_sekiroPathInput;
    SavePathInput* m_erPathInput;
};

class HotkeyInput: public QFrame {
    Q_OBJECT
public:
    explicit HotkeyInput(QWidget* parent);
    QKeyCombination getKeyCombination();
    void setKeyCombination(const QKeyCombination& combination);

    void mousePressEvent(QMouseEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void updateLabel();
    void finishEdit();

    QLabel* m_hotkeyLabel;
    bool m_isEditing = false;
    QKeyCombination m_keyCombination;
    QKeyCombination m_tmpCombination;
};

class HotkeysWidget: public QWidget {
    Q_OBJECT
public:
    explicit HotkeysWidget(const ConfigSettingsData& configData, QWidget* parent);
    void updateConfigInfo(const ConfigSettingsData& configData);
    void applyChanges(const ConfigSettingsData& configData, ConfigConfirmData& confirmData);
private:
    HotkeyInput* m_quickSaveInput;
    HotkeyInput* m_quickLoadInput;
};

class AutoBackupWidget: public QWidget {
    Q_OBJECT
public:
    explicit AutoBackupWidget(const ConfigSettingsData& configData, QWidget* parent);
    void updateConfigInfo(const ConfigSettingsData& configData);
    void applyChanges(const ConfigSettingsData& configData, ConfigConfirmData& confirmData);
private:
    NiceCheckbox* m_enabledInput;
    FocusSpinBox* m_frequencyInput;
    FocusSpinBox* m_maxAutobackupInput;
};

class SettingsWidget: public QWidget {
    Q_OBJECT
public:
    explicit SettingsWidget(Controller* controller, QWidget* parent);

public slots:
    void virtual discardChanges();

private:
    Controller* m_controller;
    ConfigSettingsData m_configData;
    SavePathInputsWidget* m_pathsWidget;
    HotkeysWidget* m_hotkeysWidget;
    AutoBackupWidget* m_autobackupWidget;

private slots:
    void virtual onSave();
};
