# FromSoftSaveManager
Simple tool to view and manage save files of FromSoftware games. The end goal is to be able to create a backup of a character from save file, and be able to reapply the character to save file.

## State
This project is in early development. It is not yet ready for use.

### Screenshot
![Alt text](/example_01.png "Dark Souls: Remastered save file")

## Reason
Bored developer who likes to play FromSoftware games.

## Planned features
Order of features is not deterministic.

- Parse save files.
- [ ] DS:R
- [ ] DS2
- [ ] DS3
- [ ] Elden Ring

### View
- Create simple UI showing the parsed data.
- [ ] Dark DS:R
- [ ] DS2
- [ ] DS3
- [ ] Elden Ring

### Backup save files
- Create a backup of a character from save file and reapply the character to save file.
- [ ] DS:R
- [ ] DS2
- [ ] DS3
- [ ] Elden Ring


## Sources
Some of the features were already implemented in other projects. Especially the parsing will be something what I will be looking at. Please let me know if there are projects that already do allow to create a backup of individual characters.
- https://gabtoubl.github.io/ds1_save/ (javascript) DS1 save file parser.
- https://github.com/tarvitz/dsfp (python) DS1 save file parser.
- https://github.com/jtesta/souls_givifier (python) DSR, DS2, DS3, ER to change amount of souls.
- https://github.com/tremwil/DS3SaveUnpacker (C#) DS3 save file decryption.
- https://github.com/ClayAmore/ER-Save-Editor (Rust) Elden Ring save file editor -> better project that this but stale.