#include "Controller.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUrl>
#include <QByteArray>
#include <filesystem>
#include <iostream>
#include <utility>

namespace {
struct SoundInMemory {
    QByteArray data = {};
    ma_decoder decoder = {};
    ma_sound sound = {};
    bool isReady = false;
};

void tryInitSoundFromResource(ma_engine* engine, const QString& resourcePath, SoundInMemory& target) {
    QFile resource(resourcePath);
    if (!resource.open(QIODevice::ReadOnly)) return;

    target.data = resource.readAll();
    if (target.data.isEmpty()) return;

    if (ma_decoder_init_memory(target.data.constData(), static_cast<size_t>(target.data.size()), nullptr, &target.decoder) != MA_SUCCESS)
        return;

    if (ma_sound_init_from_data_source(engine, &target.decoder, 0, nullptr, &target.sound) != MA_SUCCESS) {
        ma_decoder_uninit(&target.decoder);
        return;
    }

    target.isReady = true;
}

void playSound(SoundInMemory& soundData) {
    if (!soundData.isReady) return;

    ma_sound_stop(&soundData.sound);
    ma_sound_seek_to_pcm_frame(&soundData.sound, 0);
    ma_sound_start(&soundData.sound);
}
}

class AudioNotifier {
public:
    AudioNotifier() {
        ma_engine_config config = ma_engine_config_init();
        config.noAutoStart = MA_FALSE;
        if (ma_engine_init(&config, &m_engine) != MA_SUCCESS) return;

        m_isInitialized = true;
        tryInitSoundFromResource(&m_engine, ":/audio/soul_suck.wav", m_saveSound);
        tryInitSoundFromResource(&m_engine, ":/audio/ember_restored.wav", m_loadSound);
    }

    ~AudioNotifier() {
        if (m_isInitialized) {
            if (m_saveSound.isReady) {
                ma_sound_uninit(&m_saveSound.sound);
                ma_decoder_uninit(&m_saveSound.decoder);
            }
            if (m_loadSound.isReady) {
                ma_sound_uninit(&m_loadSound.sound);
                ma_decoder_uninit(&m_loadSound.decoder);
            }
            ma_engine_uninit(&m_engine);
        }
    }

    void playSave() {
        if (!m_isInitialized) return;
        playSound(m_saveSound);
    }

    void playLoad() {
        if (!m_isInitialized) return;
        playSound(m_loadSound);
    }

private:
    ma_engine m_engine = {};
    bool m_isInitialized = false;
    SoundInMemory m_saveSound = {};
    SoundInMemory m_loadSound = {};
};


HotkeysThread::HotkeysThread(const ConfigHotkeys& config, QObject* parent): QThread(parent) {
    updateHotkeys(config);
}

void HotkeysThread::stop() {
    m_isRunning = false;
}

void HotkeysThread::run() {
    m_isRunning = true;
    bool quickSavePressed = false;
    bool quickLoadPressed = false;
    while (m_isRunning) {
        bool triggerSave = false;
        bool triggerLoad = false;
        if (m_quickSaveHotkey.empty() || keysArePressed(m_quickSaveHotkey)) {
            quickSavePressed = true;
        } else if (quickSavePressed) {
            quickSavePressed = false;
            triggerSave = true;
        }

        if (m_quickLoadHotkey.empty() || keysArePressed(m_quickLoadHotkey)) {
            quickLoadPressed = true;
        } else if (quickLoadPressed) {
            quickLoadPressed = false;
            triggerLoad = true;
        }

        if (m_hotkeysChanged) {
            m_hotkeysChanged = false;
        } else if (triggerSave) {
            emit quickSaveRequested();
        } else if (triggerLoad) {
            emit quickLoadRequested();
        }
        msleep(10);
    }
}

void HotkeysThread::updateHotkeys(const ConfigHotkeys& hotkeys) {
    m_quickSaveHotkey = qtCombinationToInt(hotkeys.quickSaveHotkey);
    m_quickLoadHotkey = qtCombinationToInt(hotkeys.quickLoadHotkey);
    m_hotkeysChanged = true;
}

// --- SaveChangesThread ---
std::filesystem::file_time_type getFileModificationTime(const std::filesystem::path& filePath) {
    if (!std::filesystem::exists(filePath)) return std::filesystem::file_time_type {};
    return std::filesystem::last_write_time(filePath);
}

SaveChangesThread::SaveChangesThread(const std::vector<SaveFileItem>& saveItems, QObject* parent) : QThread(parent) {
    updatePaths(saveItems);
}

void SaveChangesThread::updatePaths(const std::vector<SaveFileItem>& saveItems) {
    std::unordered_set<QString> currentIds;
    for (auto& [saveId, _]: m_saveFilesBySaveId) {
        currentIds.insert(saveId);
    }
    for (auto& saveItem: saveItems) {
        if (currentIds.find(saveItem.saveId) != currentIds.end()) {
            currentIds.erase(saveItem.saveId);
        }
        std::filesystem::path path = saveItem.savePath.toStdString();
        m_saveFilesBySaveId[saveItem.saveId] = path;
        m_lastChangedById[saveItem.saveId] = getFileModificationTime(path);
    }
    for (auto& saveId: currentIds) {
        m_saveFilesBySaveId.erase(saveId);
        m_lastChangedById.erase(saveId);
    }
}

void SaveChangesThread::stop() {
    m_isRunning = false;
}

void SaveChangesThread::run() {
    m_isRunning = true;

    while (m_isRunning) {
        for (auto& [saveId, path]: m_saveFilesBySaveId) {
            std::filesystem::file_time_type& oldMod = m_lastChangedById[saveId];
            std::filesystem::file_time_type newMod = getFileModificationTime(path);
            if (oldMod == newMod) continue;
            m_lastChangedById[saveId] = newMod;
            emit saveFileChanged(saveId);
        }
        msleep(1000);
    }
}

// --- Controller ---
Controller::Controller(QObject* parent): QObject(parent) {
    m_audioNotifier = std::make_unique<AudioNotifier>();

    m_configModel = new ConfigModel(this);
    auto saveFileItems = m_configModel->getSaveFileItems();
    m_backupsModel = new BackupsModel(saveFileItems, m_configModel->getAutosaveConfig(), m_configModel->getBackupDirPath(), this);
    m_hotkeysThread = new HotkeysThread(m_configModel->getHotkeysConfig(), this);
    m_saveChangesThread = new SaveChangesThread(saveFileItems, this);

    connect(m_configModel, SIGNAL(pathsChanged()), this, SLOT(onGamePathsChange()));
    connect(m_configModel, SIGNAL(hotkeysChanged()), this, SLOT(onHotkeysChange()));
    connect(m_configModel, SIGNAL(autoBackupChanged()), this, SLOT(onAutobackupChange()));

    connect(m_hotkeysThread, SIGNAL(quickSaveRequested()), this, SLOT(onQuickSaveRequest()));
    connect(m_hotkeysThread, SIGNAL(quickLoadRequested()), this, SLOT(onQuickLoadRequest()));

    connect(m_saveChangesThread, SIGNAL(saveFileChanged(QString)), this, SLOT(onSaveFileChange(QString)));

    connect(m_backupsModel, SIGNAL(createBackupFinished(bool, BackupType)), this, SLOT(onBackupCreate(bool, BackupType)));
    connect(m_backupsModel, SIGNAL(loadBackupFinished(bool)), this, SLOT(onBackupLoad(bool)));

    m_hotkeysThread->start();
    m_saveChangesThread->start();
}

Controller::~Controller() {
    m_hotkeysThread->stop();
    m_hotkeysThread->wait();
    m_hotkeysThread->deleteLater();

    m_saveChangesThread->stop();
    m_saveChangesThread->wait();
    m_saveChangesThread->deleteLater();

    m_configModel->saveConfig();
    delete m_configModel;
}

QString Controller::getLastSelectedSaveId() const {
    return m_configModel->getLastSelectedSaveId();
}

void Controller::setCurrentTabId(const QString& saveId) {
    m_currentSaveId = saveId;
    m_configModel->setLastSelectedSaveId(saveId);
}

ConfigSettingsData Controller::getConfigSettingsData() const {
    return m_configModel->getConfigSettingsData();
}

ConfigHotkeys Controller::getHotkeysConfig() const {
    return m_configModel->getHotkeysConfig();
}

void Controller::saveConfigData(const ConfigConfirmData& confirmData) {
    m_configModel->saveConfigData(confirmData);
}

std::vector<SaveFileItem> Controller::getSaveFileItems() const {
    return m_configModel->getSaveFileItems();
}

DSRCharInfoResult Controller::getDsrCharacters(const QString& saveId) const {
    QString r_savePath = m_configModel->getSavePathItem(saveId);
    if (r_savePath.isEmpty()) return {
        "Save file path is not set.",
        {},
    };
    std::string savePath = r_savePath.toStdString();
    if (!std::filesystem::exists(savePath)) return {
        "Save file does not exist.",
        {},
    };
    try {
        fssm::parse::SL2File sl2_dsr = fssm::parse::parse_sl2_file(savePath);
        fssm::parse::dsr::DSRSaveFile dsr = fssm::parse::dsr::parse_dsr_file(sl2_dsr);

        return {
            "",
            dsr.characters
        };
    }
    catch (const std::exception& e) {
        return {
            "Failed to read or parse DSR save",
            {},
        };
    }
}

DS3CharInfoResult Controller::getDs3Characters(const QString& saveId) const {
    QString r_savePath = m_configModel->getSavePathItem(saveId);
    if (r_savePath.isEmpty()) return {
        "Save file path is not set.",
        {},
    };
    std::string savePath = r_savePath.toStdString();
    if (!std::filesystem::exists(savePath)) return {
        "Save file does not exist.",
        {},
    };
    try {
        fssm::parse::SL2File sl2_dsr = fssm::parse::parse_sl2_file(savePath);
        fssm::parse::ds3::DS3SaveFile ds3 = fssm::parse::ds3::parse_ds3_file(sl2_dsr);

        return {
            "",
            ds3.characters
        };
    }
    catch (const std::exception& e) {
        return {
            "Failed to read or parse DS3 save",
            {},
        };
    }
}

ERCharInfoResult Controller::getERCharacters(const QString& saveId) const {
    QString r_savePath = m_configModel->getSavePathItem(saveId);
    if (r_savePath.isEmpty()) return {
        "Save file path is not set.",
        {},
    };
    std::string savePath = r_savePath.toStdString();
    if (!std::filesystem::exists(savePath)) return {
        "Save file does not exist.",
        {},
    };
    try {
        fssm::parse::SL2File sl2_dsr = fssm::parse::parse_sl2_file(savePath);
        fssm::parse::er::ERSaveFile er = fssm::parse::er::parse_er_file(sl2_dsr);

        return {
            "",
            er.characters
        };
    }
    catch (const std::exception& e) {
        return {
            "Failed to read or parse ER save",
            {},
        };
    }
}

void Controller::openBackupDir() {
    if (m_currentSaveId.isEmpty()) return;
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return;
    std::string backupDir = m_backupsModel->getGameBackupDir(itemOpt.value().game);
    if (!std::filesystem::exists(backupDir)) return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(backupDir)));
}

void Controller::onQuickSaveRequest() {
    if (m_currentSaveId.isEmpty()) return;
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return;
    QString savePath = itemOpt.value().savePath;
    if (savePath.isEmpty()) return;
    m_backupsModel->createQuickSaveBackup(savePath, itemOpt.value().game);
};

void Controller::onQuickLoadRequest() {
    if (m_currentSaveId.isEmpty()) return;
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return;
    m_backupsModel->quickLoad(itemOpt.value().savePath, itemOpt.value().game);
}

std::vector<BackupMetadata> Controller::getBackupItems() {
    if (m_currentSaveId.isEmpty()) return {};
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return {};
    return m_backupsModel->getBackupItems(itemOpt.value().game);
}

std::optional<BackupMetadata> Controller::createManualBackup() {
    if (m_currentSaveId.isEmpty()) return std::nullopt;
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return std::nullopt;
    return m_backupsModel->createManualBackup(itemOpt.value().savePath, itemOpt.value().game, "");
}

void Controller::restoreBackupById(const QString& backupId) {
    if (m_currentSaveId.isEmpty()) return;
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return;
    m_backupsModel->restoreBackupById(itemOpt.value().savePath, itemOpt.value().game, backupId);
}

void Controller::deleteBackupByIds(const std::vector<QString>& backupIds) {
    if (m_currentSaveId.isEmpty()) return;
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return;
    m_backupsModel->deleteBackupByIds(itemOpt.value().game, backupIds);
}

bool Controller::changeBackupLabel(const QString& backupId, const QString& label) {
    if (m_currentSaveId.isEmpty()) return false;
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return false;
    return m_backupsModel->changeBackupLabel(itemOpt.value().game, backupId, label);
}

// Config changed slots
void Controller::onGamePathsChange() {
    m_saveChangesThread->updatePaths(m_configModel->getSaveFileItems());
    emit pathsConfigChanged();
}

void Controller::onHotkeysChange() {
    m_hotkeysThread->updateHotkeys(m_configModel->getHotkeysConfig());
    emit hotkeysConfigChanged();
}

void Controller::onAutobackupChange() {
    m_backupsModel->updateAutobackupConfig(m_configModel->getAutosaveConfig());
    emit autobackupConfigChanged();
}

// Save file changed
void Controller::onSaveFileChange(const QString& saveId) {
    m_backupsModel->saveGameChanged(saveId);
    emit saveIdChanged(saveId);
}

void Controller::onBackupCreate(bool success, BackupType backupType) {
    if (success && backupType != BackupType::AUTOSAVE)
        m_audioNotifier->playSave();
}

void Controller::onBackupLoad(bool success) {
    if (success)
        m_audioNotifier->playLoad();
}
