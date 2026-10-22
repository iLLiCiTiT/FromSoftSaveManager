# Backups: folders and storage location

Status of each part is marked **[done]** (in this branch) or **[later]** (design only).

## Current storage

All backups live in one SQLite database, `backups.db` (`BackupsDb`). Each row holds the
metadata (`id`, `game`, `backup_type`, `label`, `datetime`, `epoch`, `filename`) and the
whole `.sl2` file as a blob. Rows are keyed by game, not by save id, because the app has a
single save path per game.

The old format (one directory per backup with `metadata.json` next to the `.sl2`) is only
read by `MigrationHelper`, which offers to import it into the database on startup.

## Default backups folder **[done]**

Settings → *Backups* → *Backups folder* sets where `backups.db` lives, for all saves of all
games.

- Empty input means the default: the app data folder
  (`%LOCALAPPDATA%\FromSoftSaveManager\FromSoftSaveManager`), where `backups.db` has always
  been. The placeholder shows that path.
- Stored in `config.json` as `"backups_dir"`. An empty string is the default, so the default
  keeps following the app data folder.
- No per-save subfolders: one database already separates games by its `game` column. A folder
  per game/save would only make sense with a database per save (see *Per-save override*).

### Changing the folder

Existing backups are never moved or deleted. When the folder is changed and saved:

- If the new folder already has a `backups.db`, the app asks to confirm and then uses it
  (e.g. a folder synced from another PC).
- Otherwise it asks:
  - **Yes**: copy the current `backups.db` to the new folder (`QFile::copy`, never
    overwrites), then switch.
  - **No**: switch; the new folder starts empty.
  - **Cancel**: nothing is saved.
- The previous `backups.db` stays where it was. Switching back to that folder shows its
  backups again.

The database is reopened at runtime (`BackupsDb::reopen`), so no restart is needed.

### Per-save override **[later]**

The config has no per-save settings yet besides the path. If needed, add an optional
`"backups_dir"` to each `game_save_files.<game>` entry. Such a save would get its own
`BackupsDb` instance; `BackupsModel` would pick the database by save id instead of holding
one pointer.

## Folders (categories) **[done]**

Folders are flat, per game, and user-named. A backup is in at most one folder; a backup without
one is *Unsorted*.

### Database

- New column `backups.folder TEXT NOT NULL DEFAULT ''`, where an empty value means Unsorted.
- New table `backup_folders(game, name, PRIMARY KEY (game, name))`, so empty folders persist.
- `getFolders` returns the union of both, so a folder referenced only by backups still shows.

The folder is stored in the database, not as directories on disk, because backups aren't
files on disk anymore.

### Migration

`BackupsDb::createSchema` checks `PRAGMA table_info(backups)` and runs
`ALTER TABLE backups ADD COLUMN folder ... DEFAULT ''` when the column is missing. Existing
backups become Unsorted, so they list and restore exactly as before. Nothing else is rewritten.
Older app versions can still open the database: they ignore the extra column and table, and
their inserts get the default `''`.

### Operations

| Operation | Effect |
|---|---|
| Create folder | Inserts into `backup_folders`. |
| Rename folder | Updates `backup_folders` and `backups.folder` in one transaction. Renaming onto an existing name merges the two. |
| Remove folder | Its backups become Unsorted, then the folder row is deleted. Backups are **never** deleted with a folder. |
| Move backups | `UPDATE backups SET folder = ? WHERE id IN (...)` |

### Overlay UI

The overlay shows a tree, not a separate sidebar, because it is narrow (about 3/7 of the
window):

- Folders are top-level rows with a folder icon and a backup count in the *Date* column. They
  always sort above backups and alphabetically among themselves.
- Unsorted backups are top-level rows, as before. Backups in a folder are its children.
- **Folder** button: creates `New folder` (`New folder 2`, ...) and starts renaming it.
- **Save** button: the new manual backup goes into the selected folder (or the folder of the
  selected backup); Unsorted if nothing is selected.
- **Delete** button: deletes the selected backups. Selected folders are removed, and their
  backups become Unsorted.
- Drag one or more backups onto a folder (or onto a backup inside it) to move them. Drop them
  on empty space to make them Unsorted.
- Backup context menu: *Load*, *Rename*, *Move to ▸* (Unsorted, every folder, *New folder..*),
  *Delete*.
- Folder context menu: *Rename*, *New folder*, *Remove folder (keep backups)*. F2 renames a
  folder too.
- Folders start expanded. Folders you collapse stay collapsed while the app runs.

### Autobackup rotation

`cleanupAutoBackups` only rotates **Unsorted** autosaves. Moving an autosave into a folder
keeps it, the same way renaming a backup turns it into a manual one.

### Default folder per backup type **[later]**

Planned: an optional default folder for each type, e.g. quicksaves into `Quick`, autosaves
into `Auto`.

- Config: `"default_folders": {"quicksave": "", "autosave": "", "manualsave": ""}` per game
  (or global). An empty value means Unsorted.
- `BackupsModel::createBackup` already takes a `folder` argument. Callers would look up the
  default for the type.
- Rotation would then need to rotate autosaves in their default folder rather than only
  Unsorted ones. Quickload already ignores folders and takes the newest quicksave.

### Nested folders **[later]**

Folder names could become `/`-separated paths (`Boss runs/Malenia`) without a schema change.
The tree would split names into nodes, and rename/remove would match by prefix.

## Development: scratch config

Set `FSSM_CONFIG_DIR` to run against another config folder instead of
`%LOCALAPPDATA%\FromSoftSaveManager\FromSoftSaveManager`, e.g. to test with copies of saves
and backups without touching real ones:

```bash
FSSM_CONFIG_DIR=C:/tmp/fssm-scratch ./build/FromSoftSaveManager.exe
```
