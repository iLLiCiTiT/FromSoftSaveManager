from PySide6 import QtWidgets, QtGui, QtCore

from from_soft_manager.ui.structures import ConfigInfo, ConfigConfirmData


class HotkeyInput(QtWidgets.QFrame):
    def __init__(self, parent):
        super().__init__(parent)
        # TODO need to handle clearing of hotkey

        hotkey_label = QtWidgets.QLabel(self)
        hotkey_label.setText("< Not set >")

        main_layout = QtWidgets.QHBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(hotkey_label, 1)

        self._hotkey_label = hotkey_label

        self._is_editing = False
        self._key_combination = None
        self._tmp_combination = None

    def get_key_combination(self) -> QtCore.QKeyCombination | None:
        return self._key_combination

    def set_combination(self, combination: set[int] | None):
        self._key_combination = combination
        self._update_label()

    def mousePressEvent(self, event):
        self._tmp_combination, self._key_combination = self._key_combination, None
        self._is_editing = True
        self._update_label()
        self.grabKeyboard()

    def closeEvent(self, event):
        if self._is_editing:
            self._key_combination = self._tmp_combination
            self._finish_editing()
        super().closeEvent(event)

    def focusOutEvent(self, event):
        if self._is_editing:
            self._key_combination = self._tmp_combination
            self._finish_editing()
        super().focusOutEvent(event)

    def keyPressEvent(self, event: QtGui.QKeyEvent):
        key = event.key()
        if key == QtCore.Qt.Key_Escape:
            self._key_combination = self._tmp_combination
            self._finish_editing()
            return

        if key in (QtCore.Qt.Key_Backspace, QtCore.Qt.Key_Delete):
            self._key_combination = None
            self._finish_editing()
            return

        if key in (
            QtCore.Qt.Key_Control,
            QtCore.Qt.Key_Alt,
            QtCore.Qt.Key_AltGr,
            QtCore.Qt.Key_Meta,
            QtCore.Qt.Key_Shift,
        ):
            self._key_combination = QtCore.QKeyCombination(event.modifiers())
            self._update_label()
            return

        self._key_combination = QtCore.QKeyCombination(
            event.keyCombination()
        )
        self._finish_editing()

    def keyReleaseEvent(self, event: QtGui.QKeyEvent):
        key = event.key()
        if key in (
            QtCore.Qt.Key_Control,
            QtCore.Qt.Key_Alt,
            QtCore.Qt.Key_AltGr,
            QtCore.Qt.Key_Meta,
            QtCore.Qt.Key_Shift,
        ):
            self._key_combination = QtCore.QKeyCombination(event.modifiers())
            self._update_label()
            return
        super().keyReleaseEvent(event)

    def _update_label(self):
        if self._key_combination is None:
            label = "..." if self._is_editing else "< Not set >"
            self._hotkey_label.setText(label)
            return

        added_a = False
        if self._key_combination.key() == QtCore.Qt.Key_Enter:
            modifiers = self._key_combination.keyboardModifiers()
            modifiers &= ~QtCore.Qt.KeypadModifier
            self._key_combination = QtCore.QKeyCombination(
                modifiers, QtCore.Qt.Key_Return
            )

        combination = self._key_combination
        if combination.key() == QtCore.Qt.Key_unknown:
            if combination.keyboardModifiers() == QtCore.Qt.NoModifier:
                self._hotkey_label.setText("...")
                return
            added_a = True
            combination = QtCore.QKeyCombination(
                combination.keyboardModifiers(),
                QtCore.Qt.Key_A
            )

        sequence = QtGui.QKeySequence(combination)
        label = sequence.toString(QtGui.QKeySequence.PortableText)
        if added_a:
            label = label.rstrip("A") + "..."
        self._hotkey_label.setText(label)

    def _finish_editing(self):
        self._is_editing = False
        self.releaseKeyboard()
        self._tmp_combination = None
        self._update_label()


class SettingsDialog(QtWidgets.QDialog):
    def __init__(self, controller, parent):
        super().__init__(parent)
        self.setWindowTitle("Settings")
        self.setModal(True)

        config_info: ConfigInfo = controller.get_config_info()

        # TODO add option to reset to default
        # TODO use icons
        paths_widget = QtWidgets.QWidget(self)

        dsr_path_label = QtWidgets.QLabel(
            "Dark Souls: Remastered", paths_widget
        )
        dsr_path_input = QtWidgets.QLineEdit(parent)
        dsr_path_input.setText(config_info.dsr_save_path.save_path)
        dsr_path_input.setPlaceholderText("< Path to save file >")
        dsr_open_btn = QtWidgets.QPushButton("Open", paths_widget)

        ds2_path_label = QtWidgets.QLabel("Dark Souls II: SOTFS", paths_widget)
        ds2_path_input = QtWidgets.QLineEdit(parent)
        ds2_path_input.setText(config_info.ds2_save_path.save_path)
        ds2_path_input.setPlaceholderText("< Path to save file >")
        ds2_open_btn = QtWidgets.QPushButton("Open", paths_widget)

        ds3_path_label = QtWidgets.QLabel("Dark Souls III", paths_widget)
        ds3_path_input = QtWidgets.QLineEdit(parent)
        ds3_path_input.setText(config_info.ds3_save_path.save_path)
        ds3_path_input.setPlaceholderText("< Path to save file >")
        ds3_open_btn = QtWidgets.QPushButton("Open", paths_widget)

        er_path_label = QtWidgets.QLabel("Elden Ring", paths_widget)
        er_path_input = QtWidgets.QLineEdit(parent)
        er_path_input.setText(config_info.er_save_path.save_path)
        er_path_input.setPlaceholderText("< Path to save file >")
        er_open_btn = QtWidgets.QPushButton("Open", paths_widget)

        paths_layout = QtWidgets.QGridLayout(paths_widget)
        paths_layout.setContentsMargins(0, 0, 0, 0)
        paths_layout.addWidget(dsr_path_label, 0, 0)
        paths_layout.addWidget(dsr_path_input, 0, 1)
        paths_layout.addWidget(dsr_open_btn, 0, 2)
        paths_layout.addWidget(ds2_path_label, 1, 0)
        paths_layout.addWidget(ds2_path_input, 1, 1)
        paths_layout.addWidget(ds2_open_btn, 1, 2)
        paths_layout.addWidget(ds3_path_label, 2, 0)
        paths_layout.addWidget(ds3_path_input, 2, 1)
        paths_layout.addWidget(ds3_open_btn, 2, 2)
        paths_layout.addWidget(er_path_label, 3, 0)
        paths_layout.addWidget(er_path_input, 3, 1)
        paths_layout.addWidget(er_open_btn, 3, 2)

        paths_layout.setColumnStretch(0, 0)
        paths_layout.setColumnStretch(1, 1)
        paths_layout.setColumnStretch(2, 0)

        hotkeys_widget = QtWidgets.QWidget(self)

        hotkeys_label = QtWidgets.QLabel("Hotkeys:", hotkeys_widget)
        qs_label = QtWidgets.QLabel("QuickSave:", hotkeys_widget)
        quicksave_input = HotkeyInput(hotkeys_widget)
        quicksave_input.set_combination(config_info.quicksave_hotkey)
        ql_label = QtWidgets.QLabel("QuickLoad:", hotkeys_widget)
        quickload_input = HotkeyInput(hotkeys_widget)
        quickload_input.set_combination(config_info.quickload_hotkey)

        hotkeys_layout = QtWidgets.QGridLayout(hotkeys_widget)
        hotkeys_layout.setContentsMargins(0, 0, 0, 0)
        hotkeys_layout.addWidget(hotkeys_label, 0, 0, 1, 2)
        hotkeys_layout.addWidget(qs_label, 1, 0)
        hotkeys_layout.addWidget(quicksave_input, 1, 1)
        hotkeys_layout.addWidget(ql_label, 2, 0)
        hotkeys_layout.addWidget(quickload_input, 2, 1)

        btns_widget = QtWidgets.QWidget(self)

        save_btn = QtWidgets.QPushButton("Save", btns_widget)
        cancel_btn = QtWidgets.QPushButton("Cancel", btns_widget)

        btns_layout = QtWidgets.QHBoxLayout(btns_widget)
        btns_layout.setContentsMargins(0, 0, 0, 0)
        btns_layout.addStretch(1)
        btns_layout.addWidget(save_btn, 0)
        btns_layout.addWidget(cancel_btn, 0)

        main_layout = QtWidgets.QVBoxLayout(self)
        main_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.addWidget(paths_widget, 0)
        main_layout.addWidget(hotkeys_widget, 0)
        main_layout.addStretch(1)
        main_layout.addWidget(btns_widget, 0)

        dsr_open_btn.clicked.connect(self._on_dsr_open_click)
        ds2_open_btn.clicked.connect(self._on_ds2_open_click)
        ds3_open_btn.clicked.connect(self._on_ds3_open_click)
        er_open_btn.clicked.connect(self._on_er_open_click)
        save_btn.clicked.connect(self.accept)
        cancel_btn.clicked.connect(self.reject)

        self._config_info = config_info
        self._dsr_path_input = dsr_path_input
        self._ds2_path_input = ds2_path_input
        self._ds3_path_input = ds3_path_input
        self._er_path_input = er_path_input
        self._quicksave_input = quicksave_input
        self._quickload_input = quickload_input

        self.resize(600, 200)

    def get_values(self) -> ConfigConfirmData:
        dsr_path = self._dsr_path_input.text()
        ds2_path = self._ds2_path_input.text()
        ds3_path = self._ds3_path_input.text()
        er_path = self._er_path_input.text()
        quicksave_hotkey = self._quicksave_input.get_key_combination()
        quickload_hotkey = self._quickload_input.get_key_combination()
        data = ConfigConfirmData(dsr_path)
        if dsr_path != self._config_info.dsr_save_path.save_path:
            data.dsr_save_path = dsr_path
        if ds2_path != self._config_info.ds2_save_path.save_path:
            data.ds2_save_path = ds2_path
        if ds3_path != self._config_info.ds3_save_path.save_path:
            data.ds3_save_path = ds3_path
        if er_path != self._config_info.er_save_path.save_path:
            data.er_save_path = er_path
        if quicksave_hotkey != self._config_info.quicksave_hotkey:
            data.quicksave_hotkey = quicksave_hotkey
        if quickload_hotkey != self._config_info.quickload_hotkey:
            data.quickload_hotkey = quickload_hotkey
        return data

    def _select_file(
        self,
        game_title: str,
        path_hint: str,
        targe_input: QtWidgets.QLineEdit
    ):
        path, _ = QtWidgets.QFileDialog.getOpenFileName(
            self,
            f"Select '{game_title} Save File",
            path_hint,
            "SL2 files (*.sl2);;All Files (*)")
        if path:
            targe_input.setText(path)

    def _on_dsr_open_click(self):
        self._select_file(
            "Dark Souls: Remastered",
            self._config_info.dsr_save_path.save_path_hint,
            self._dsr_path_input
        )

    def _on_ds2_open_click(self):
        self._select_file(
            "Dark Souls II: Scholar of the First Sin",
            self._config_info.ds2_save_path.save_path_hint,
            self._ds2_path_input
        )

    def _on_ds3_open_click(self):
        self._select_file(
            "Dark Souls III",
            self._config_info.ds3_save_path.save_path_hint,
            self._ds3_path_input
        )

    def _on_er_open_click(self):
        self._select_file(
            "Elden Ring",
            self._config_info.er_save_path.save_path_hint,
            self._er_path_input
        )
