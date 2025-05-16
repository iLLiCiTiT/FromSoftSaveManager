# FromSoftSaveManager
Simple tool to view and manage save files of FromSoftware games. The end goal is to be able to create a backup of a character from save file, and be able to reapply the character to save file.

## State
This project is in early development. It is not yet ready for use.

## Reason
Bored developer who likes to play FromSoftware games.

## Planned features
Order of features is not deterministic.

### Parsing
- [ ] Parse DS:R save files.
- [ ] Parse save files.
- [ ] Parse DS2 save files.
- [ ] Parse DS3 save files.
- [ ] Parse Elden Ring save files.

### View
- [ ] Create simple UI showing the parsed data.
- [ ] Add images related to items.

### Backup save files
- [ ] Create a backup of a character from save file.
- [ ] Reapply the character to save file.
- [ ] Reapply the character to different position.

## Sources
Some of the features were already implemented in other projects. Especially the parsing will be something what I will be looking at. Please let me know if there are projects that already do allow to create a backup of individual characters.
- https://gabtoubl.github.io/ds1_save/ (javascript) DS1 save file parser.
- https://github.com/tarvitz/dsfp (python) DS1 save file parser.
- https://github.com/jtesta/souls_givifier (python) DSR, DS2, DS3, ER to change amount of souls.
- https://github.com/tremwil/DS3SaveUnpacker (C#) DS3 save file decryption.
