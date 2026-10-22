#pragma once

#include <string>
#include <ctime>

#include "../parse/Game.h"


enum class BackupType {
    QUICKSAVE,
    AUTOSAVE,
    MANUAL
};

static std::string backupTypeToString(const BackupType& backupType) {
    switch (backupType) {
        case BackupType::QUICKSAVE:
            return "quicksave";
        case BackupType::AUTOSAVE:
            return "autosave";
        default:
            return "manualsave";
    }
}

static BackupType backupTypeFromString(const std::string& backupType) {
    if (backupType == "quicksave") return BackupType::QUICKSAVE;
    if (backupType == "autosave") return BackupType::AUTOSAVE;
    return BackupType::MANUAL;
}

struct BackupMetadata {
    std::string id;
    fssm::Game game;
    BackupType backupType;
    std::string label;
    std::string filename;
    std::string datetime;
    time_t epoch;
    // Name of the user folder the backup is categorized in, empty means "Unsorted"
    std::string folder;
};
