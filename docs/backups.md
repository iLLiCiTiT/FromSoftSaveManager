# Backups: folders

Status of each part is marked **[done]** (in this branch) or **[later]** (design only).

## Current storage

All backups live in one SQLite database, `backups.db` (`BackupsDb`), in the app data folder.
Each row holds the metadata (`id`, `game`, `backup_type`, `label`, `datetime`, `epoch`,
`filename`) and the whole `.sl2` file as a blob. Rows are keyed by game, not by save id,
because the app has a single save path per game.

Folders are a UI categorization stored in that database. They are not directories on disk.

## Folders (categories) **[done]**

Folders are flat, per game, and user-named. A backup is in at most one folder; a backup without
one is *Unsorted*.

### Database

- New column `backups.folder TEXT NOT NULL DEFAULT ''`, where an empty value means Unsorted.
- New table `backup_folders(game, name, is_default, PRIMARY KEY (game, name))`, so empty
  folders persist and one folder per game can be the default.
- `getFolders` returns the union of both, so a folder referenced only by backups still shows.

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
| Rename folder | Updates `backup_folders` and `backups.folder` in one transaction and keeps the default flag. Renaming onto an existing name merges the two. |
| Remove folder | Its backups become Unsorted, then the folder row is deleted. Backups are **never** deleted with a folder. If it was the default, Unsorted is the default again. |
| Move backups | `UPDATE backups SET folder = ? WHERE id IN (...)` |
| Set default | `UPDATE backup_folders SET is_default = (name = ?) WHERE game = ?`, so at most one folder per game is the default. |

## Default folder **[done]**

Each game can have one default folder; new backups of every type go there:

- **Quicksaves** (hotkey) and **autosaves**: always into the default folder.
- **Manual saves** (overlay *Save*): into the selected folder, or the folder of the selected
  backup. If nothing in a folder is selected, they go into the default folder.
- With no default folder set, new backups are Unsorted, as before.

`BackupsModel::createBackup` takes an optional folder. Without one, it uses
`BackupsDb::getDefaultFolder(game)`.

**Autosave rotation** (`cleanupAutoBackups`) rotates only the autosaves in the folder where new
autosaves go (the default folder, or Unsorted). Autosaves you moved elsewhere are kept. After
changing the default, autosaves in the previous folder are no longer rotated.

**Quickload** ignores folders and loads the newest quicksave.

### Default per backup type **[later]**

If one default isn't enough, `is_default` could become a type mask, e.g. `Quick` gets
quicksaves and `Auto` gets autosaves. The context menu would get one checkbox per type, and
`createBackup` would look up the default by type.

## Overlay UI **[done]**

The overlay shows a tree, not a separate sidebar, because it is narrow (about 3/7 of the
window):

- Folders are top-level rows with a folder icon and a backup count in the *Date* column. They
  always sort above backups and alphabetically among themselves.
- The default folder is bold and marked *(default)*.
- Unsorted backups are top-level rows, as before. Backups in a folder are its children.
- **Folder** button: creates `New folder` (`New folder 2`, ...) and starts renaming it.
- **Save** button: see *Default folder* above.
- **Delete** button: deletes the selected backups. Selected folders are removed, and their
  backups become Unsorted.
- Drag one or more backups onto a folder (or onto a backup inside it) to move them. Drop them
  on empty space to make them Unsorted.
- Backup context menu: *Load*, *Rename*, *Move to ▸* (Unsorted, every folder, *New folder..*),
  *Delete*.
- Folder context menu: *Default for new backups* (checkbox), *Rename*, *New folder*,
  *Remove folder (keep backups)*. F2 renames a folder too.
- Folders start expanded. Folders you collapse stay collapsed while the app runs.

## Nested folders **[later]**

Folder names could become `/`-separated paths (`Boss runs/Malenia`) without a schema change.
The tree would split names into nodes, and rename/remove would match by prefix.

## Development: scratch config

Set `FSSM_CONFIG_DIR` to run against another config folder instead of
`%LOCALAPPDATA%\FromSoftSaveManager\FromSoftSaveManager`, e.g. to test with copies of saves
and backups without touching real ones:

```bash
FSSM_CONFIG_DIR=C:/tmp/fssm-scratch ./build/FromSoftSaveManager.exe
```
