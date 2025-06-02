[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Python](https://img.shields.io/badge/Python-3776AB?logo=python&logoColor=fff)](https://www.python.org/)
[![uv](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/astral-sh/uv/main/assets/badge/v0.json)](https://github.com/astral-sh/uv)
[![Buy Me a Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-support-yellow?logo=buy-me-a-coffee)](https://www.buymeacoffee.com/illicitit) 
# FromSoftSaveManager
Simple tool to view and manage save files of FromSoftware games. The first goal is to create backup save files and re-apply them. The end goal is to be able to create a backup of a character from save file, and be able to reapply the character to save file.

Some of the features will be available only on Windows, for example quicksave and quickload.

## State
This project is in early development, please be nice. The UI is made to do what has to do so is not as nice as should be.

### Available features
- Backup save files using hotkeys (Quicksave/Quickload).
- Backup save files using UI.
- Load/Delete save files using UI.
- Open directory with backups.
- Settings for paths to save -> should be preset for each game is possible.
- Dark Souls: Remastered has some save file parsing and some UI showing parsed information.

### Screenshot
![Alt text](/example_01.png "Dark Souls: Remastered save file")

### How to use
1. Download zip file from [releases](https://github.com/iLLiCiTiT/FromSoftSaveManager/releases/tag/0.1.0).
2. Unzip the file to a directory of your choice.
3. Run `FromSoftSaveManager.exe`.
4. Make sure that path/ss are set correctly in settings.
5. Select game you'd like to manage -> you should see only games that have set paths to save files.
6. You can create backups using shortcuts or manually.
7. To restore a backup one must close the game, or in menu when seeing "PRESS ANY BUTTON".

### Notes
- Changing game in tabs changes which game is saved and restored using shortcuts.
- In current state the UI does not report errors.
- At this moment backups are not deleted automatically, so you might want to clean them up sometimes.

### How to run from code
I'm personally using [UV](https://docs.astral.sh/uv/getting-started/installation/) to manage python environments, but you can use any other tool you like.
1. Clone the repository `git clone https://github.com/iLLiCiTiT/FromSoftSaveManager.git`.
2. Change directory to the cloned repository `cd FromSoftSaveManager`.
3. Create and install virtual environment `uv sync`.
4. Run the application `uv run main.py`.

## Planned features
These are overall goals of the project. I will use `Issues` tab to track progress.

### Backup save files
Create a backup of a save file using hotkeys, or automated backup each n-seconds. Main focus of the project at this moment.

### Parse save files
Parse save files and show the parsed data in UI.

### Edit save files
Edit save files using UI, if possible. There are projects that do change souls and items in save files, so it should be possible.

### Backup individual characters from save files
Create a backup of a character from save file and then re-apply the character to a different save file, if possible.

## Sources
Some of the features were already implemented in other projects. Especially the parsing will be something what I will be looking at. Please let me know if there are other projects not mentioned here.
- https://gabtoubl.github.io/ds1_save/ (javascript) DS1 save file parser.
- https://github.com/tarvitz/dsfp (python) DS1 save file parser.
- https://github.com/jtesta/souls_givifier (python) DSR, DS2, DS3, ER to change amount of souls.
- https://github.com/tremwil/DS3SaveUnpacker (C#) DS3 save file decryption.
- https://github.com/ClayAmore/ER-Save-Editor (Rust) Elden Ring save file editor -> better project that this but stale.
 