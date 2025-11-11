[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Buy Me a Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-support-yellow?logo=buy-me-a-coffee)](https://www.buymeacoffee.com/illicitit) 
# FromSoftSaveManager
Simple tool to view and manage save files of FromSoftware games. The start goal is to create backup save files and re-apply them. The end goal is to be able to create a backup of a character from a save file, and be able to reapply the character to a save file.

The tool is Windows only (at least for now). The start goal has been achieved, so the tool is in a usable state.

## Available features
- Backup save files using hotkeys (Quicksave/Quickload).
- Backup save files using UI.
- Load/Delete save files using UI.
- Open directory with backups.
- Settings for paths to save -> should be preset for each game is possible.
- Dark Souls: Remastered, Dark Souls III and Elden Ring have some save file parsing and UI showing parsed information.

### Screenshots
![Alt text](/example_01.png "Dark Souls: Remastered save file")
![Alt text](/example_02.png "Dark Souls III save file")

### How to use
1. Download zip file from [releases](https://github.com/iLLiCiTiT/FromSoftSaveManager/releases/latest).
2. Unzip the file to a directory of your choice.
3. Run `FromSoftSaveManager.exe`.
4. Make sure that path/ss are set correctly in settings.
5. Select a game you'd like to manage -> you should see only games that have set paths to save files.
6. You can create backups using shortcuts or manually.
7. To restore a backup use quickload shortcut or double-click backup in Backups view. Make sure you're in the gane menu (seeing "PRESS ANY BUTTON"), or close the game.

### Notes
- Changing game in tabs changes which game is saved and restored using shortcuts.
- In current state the UI does not report errors.
- At this moment backups are not deleted automatically, so you might want to clean them up sometimes.

## Planned user features
### Parse more information
Parse more information from save files and show the parsed data in UI.

### Backup individual characters from save files
Create a backup of a character from a save file and then re-apply the character to a different save file. Also, apply the correct steam id to the backup.

### Edit save files
Edit save files using UI. This is the biggest challenge for me.

## More details
### Why?
This is my hobby project I'm using as a way to learn new things I don't encounter in my day-to-day work. There are plenty of other tools that can do the same thing, but I wanted to learn something new.

I'm planning to add more features in the future, but my free time is limited. If you have ideas or suggestions, please create an Issue or make a Pull Request.

## Technical details
The project is written in C++ using Qt6. It was a Python project at the beginning, but I've decided to rewrite it in C++ to learn C++. Please be nice if you're a C++ developer. Please let me know if you have any suggestions on how to improve the code. I'm still using Python for parsing development and C++ for the actual application. 

### Requirements
- CMake - CMake can be downloaded from https://cmake.org/download/.
- Qt6 - Qt6 can be downloaded from https://www.qt.io/download.
- MinGW - MinGW can be downloaded from https://sourceforge.net/projects/mingw-w64/ or install it with Qt.
Make sure CMake and MinGW binaries are in your PATH environment variable.

### Build steps
1. Clone the repository `git clone https://github.com/iLLiCiTiT/FromSoftSaveManager.git --recurse-submodules`.
2. Change directory to the cloned repository `cd FromSoftSaveManager`.
3. Run cmake `cmake -B build -DCMAKE_MAKE_PROGRAM=mingw32-make -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH={Path to Qt6 installation}`.
4. Run make `cmake --build build --config Release --target FromSoftSaveManager -j 14`.
5. Run the application `./FromSoftSaveManager.exe`.

## Sources
Some of the features were already implemented in other projects. Especially the parsing will be something what I will be looking at. Please let me know if there are other projects not mentioned here.
- https://gabtoubl.github.io/ds1_save/ (javascript) DS1 save file parser.
- https://github.com/tarvitz/dsfp (python) DS1 save file parser.
- https://github.com/jtesta/souls_givifier (python) DSR, DS2, DS3, ER to change amount of souls.
- https://github.com/tremwil/DS3SaveUnpacker (C#) DS3 save file decryption.
- https://github.com/ClayAmore/er-save-lib/ ([former](https://github.com/ClayAmore/ER-Save-Editor)) (Rust) Elden Ring save file editor. This is the biggest inspiration for the project.
- Many, many more...
 