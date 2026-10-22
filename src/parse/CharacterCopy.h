#pragma once
// Copying a character slot from one .sl2 file into a slot of another (or the same) file.
// Findings per game are in docs/move-characters.md.
//
// Supported: DSR, DS3, Sekiro and Elden Ring. The character entry is copied as is,
// except for the Steam ID stored inside it (DS3, Sekiro, ER), which is rewritten to
// the destination file's, and the DS3 slot checksum. The destination system entry
// gets the source's Load Game summary and occupied flag for the slot. Every changed
// entry gets a new MD5 (and is re-encrypted for DSR/DS3). All other bytes of the
// destination file stay untouched.

#include <cstdint>
#include <string>
#include <vector>

#include "Game.h"

namespace fssm::parse {
    struct CopySlotInfo {
        int index = 0;
        bool occupied = false;
        std::string name;           // UTF-8, empty for Sekiro (always "Wolf")
        uint32_t level = 0;         // 0 for Sekiro
        uint32_t playTime = 0;      // seconds
    };

    struct CopyFileInfo {
        Game game{Game::Unknown};
        uint64_t steamId = 0;       // 0 for games that don't store one (DSR)
        std::vector<CopySlotInfo> characters;   // all 10 character slots
    };

    bool supports_character_copy(Game game);

    // Slots of a save file as its Load Game screen shows them. Throws std::runtime_error.
    CopyFileInfo read_copy_file_info(const std::string& path);

    // Content of `dstPath` with character `srcSlot` of `srcPath` written into `dstSlot`.
    // Nothing is written to disk. Both files must be of the same game; `srcPath` may
    // be `dstPath`. Throws std::runtime_error when the copy can't be done safely.
    std::vector<uint8_t> copy_character(
        const std::string& srcPath, int srcSlot,
        const std::string& dstPath, int dstSlot
    );

    // Throws std::runtime_error naming the first entry whose stored MD5 doesn't match.
    void check_entry_md5s(const std::string& path);
}
