# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

FromSoftSaveManager is a Windows-only C++/Qt6 tool for managing FromSoftware save files (Dark Souls Remastered, Dark Souls II: SotFS, Dark Souls III, Elden Ring, Sekiro). It parses binary `.sl2` save files, supports quicksave/quickload via hotkeys, and manages manual/auto backups. The project started as a Python/PySide6 prototype (in `python/`) and was rewritten in C++; the Python directory is kept as a parsing development sandbox.

## Build Commands

```bash
# Configure
cmake -B build -DCMAKE_MAKE_PROGRAM=mingw32-make -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH={Qt6 path}

# Build
cmake --build build --config Release --target FromSoftSaveManager -j 14

# Build without item images (smaller binary)
cmake -B build -DFSSM_USE_INV_IMAGES=OFF ...
```

Three build profiles exist in CMakeLists.txt: `debug`, `release`, and `release-skip-inv` (skips large item-image QRC bundles). All use MinGW with `-static-libgcc -static-libstdc++`.

There is no automated test suite. The `.sl2` files in the repo root and `exports/` serve as manual test data.

## Architecture

### Layer Overview

The app has three layers: **parse** (binary decoding), **ui** (Qt backend), and **QML** (frontend). Communication between QML and C++ goes exclusively through `Controller`.

### Parse Layer (`src/parse/`)

- `SL2File.h/.cpp` — Reads the BND4 binary container, detects game by file count (11=DSR, 23=DS2, 12=DS3/Sekiro/ER differentiated by entry size), and decrypts entries with AES-128-CBC using hardcoded per-game keys via `vendor/tiny-AES-c`.
- `Utils.h` — `ContentReader` provides typed reads (`read_u8_le`, `read_u32_le`, `read_u16_string`, etc.) with endianness-swapping.
- `Game.h` — Strongly-typed enum wrapper for `DSR`, `DS2_SOTFS`, `DS3`, `ER`, `Sekiro` with `toString()`/`fromString()`.
- Each game subdirectory (`DSR/`, `DS3/`, `EldenRing/`) has a `SaveFile.h/.cpp` that returns a typed struct (e.g. `DSRSaveFile`) containing a vector of character info structs, and optionally an `Items.h/.cpp` with a static item database.
- All code lives under the `fssm::parse::<game>::` namespace hierarchy.

DS2 and Sekiro parsing are not yet implemented (stubs only).

`DSR/SaveData.h/.cpp` is a full, byte-exact model of DSR slots (`USER_DATA000..009` character slots, `USER_DATA010` system slot) recovered from `DarkSoulsRemastered.exe`; struct names follow the game's RTTI (`PlayerGameData`, `EquipGameData`, ...). `parse_*_slot` and `serialize_*_slot` must round-trip exactly — check with the `DSRRoundTrip` console target (`tools/dsr_roundtrip.cpp`) after layout changes.

### UI Backend (`src/ui/`)

- `Controller.h/.cpp` — Central hub, exposed to QML as `contextController`. All QML-callable methods are `Q_INVOKABLE`. Owns `ConfigModel`, `BackupsModel`, `HotkeysThread`, `SaveChangesThread`, and `AudioNotifier`.
- `ConfigModel.h/.cpp` — Persists app config as JSON (nlohmann_json) in the OS app data directory. Tracks save paths, hotkeys, and autobackup settings.
- `BackupsModel.h/.cpp` — Manages `QUICKSAVE`, `AUTOSAVE`, and `MANUAL` backup types. `AutoBackupHandler` uses a `QTimer` for frequency-based autobackups. Backup metadata stored as JSON alongside backup files.
- `HotkeysThread` — `QThread` that polls `GetAsyncKeyState` (Windows API) for configured hotkey combos.
- `SaveChangesThread` — Polls filesystem modification timestamps to detect save file changes and trigger autobackup (a TODO notes this should use `QFileSystemWatcher` instead).

### Frontend (`src/resources/`)

The UI is pure Qt Widgets. There is no QML on this branch — a QML migration was attempted on a separate branch (`enhancement/qml-tryout`) but was never finished and not merged.

- `MainWindow` is a `QWidget` (960×490) containing a `SideBarWidget` and a `QStackedWidget`.
- The stack holds `SettingsWidget` and one per-game widget (`DSRWidget`, `DS3Widget`, `ERWidget`, etc.) per configured save file. Widgets are created lazily on first `refresh()` and removed when their save path is removed.
- `ManageBackupsWidget` is an overlay widget that animates in/out over the stack using `QGraphicsBlurEffect` + `QGraphicsOpacityEffect` + `QVariantAnimation`.
- `SideBarWidget` provides the game-icon tabs; selecting a tab emits `tabChanged(QString saveId)`, which drives the stack.
- Global dark theme stylesheet at `src/resources/style/app.qss`.

### Vendors

| Vendor | Location | Purpose |
|---|---|---|
| `tiny-AES-c` | `vendor/tiny-AES-c/` | AES-128-CBC decryption (git submodule) |
| `nlohmann/json` | `vendor/nlohmann_json/` | Header-only JSON |
| `miniaudio` | `vendor/miniaudio/miniaudio.h` | Single-header audio playback |

### Uncompiled Files

`src/main3.cpp` and `src/parse/DSR/SaveFile2.cpp` are WIP experiments not included in the build.
