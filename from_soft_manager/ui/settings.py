from PySide6 import QtWidgets, QtGui, QtCore

from from_soft_manager.ui.icons import get_icon_path
from from_soft_manager.ui.structures import ConfigInfo, ConfigConfirmData
from from_soft_manager.ui.utils import SquareButton


class HotkeyInput(QtWidgets.QFrame):
    def __init__(self, parent):
        super().__init__(parent)
        self.setToolTip("Click to set/change hotkey")
        # TODO need to handle clearing of hotkey

        hotkey_label = QtWidgets.QLabel(self)
        hotkey_label.setText("< Not set >")
        hotkey_label.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)

        main_layout = QtWidgets.QHBoxLayout(self)
        main_layout.setContentsMargins(5, 0, 0, 0)
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


class SavePathInput(QtWidgets.QFrame):
    def __init__(
        self,
        game_title: str,
        parent: QtWidgets.QWidget
    ):
        super().__init__(parent)

        path_input = QtWidgets.QLineEdit(self)
        path_input.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)
        path_input.setPlaceholderText("< Path to save file >")

        open_icon = QtGui.QIcon(get_icon_path("folder_256x256.png"))
        open_btn = SquareButton(self)
        open_btn.setIcon(open_icon)
        open_btn.setToolTip("Select save file..")
        open_btn.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)

        main_layout = QtWidgets.QHBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)
        main_layout.addWidget(path_input, 1)
        main_layout.addWidget(open_btn, 0)

        open_btn.clicked.connect(self._select_file)

        self._game_title = game_title
        self._hint = ""

        self._path_input = path_input
        self._open_btn = open_btn

    def get_path(self) -> str:
        return self._path_input.text()

    def update_path(self, path: str, hint: str):
        self._path_input.setText(path)
        self._hint = hint

    def _select_file(self):
        path, _ = QtWidgets.QFileDialog.getOpenFileName(
            self,
            f"Select '{self._game_title} Save File",
            self._hint,
            "SL2 files (*.sl2);;All Files (*)")
        if path:
            self._path_input.setText(path)


class SavePathInputsWidget(QtWidgets.QWidget):
    def __init__(self, config_info: ConfigInfo, parent: QtWidgets.QWidget):
        super().__init__(parent)

        dsr_path_label = QtWidgets.QLabel(
            "Dark Souls: Remastered", self
        )
        dsr_path_input = SavePathInput(
            "Dark Souls: Remastered", self
        )
        dsr_path_input.update_path(
            config_info.dsr_save_path.save_path,
            config_info.dsr_save_path.save_path_hint
        )

        ds2_path_label = QtWidgets.QLabel("Dark Souls II: SOTFS", self)
        ds2_path_input = SavePathInput(
            "Dark Souls II: Scholar of the First Sin", self
        )
        ds2_path_input.update_path(
            config_info.ds2_save_path.save_path,
            config_info.ds2_save_path.save_path_hint
        )

        ds3_path_label = QtWidgets.QLabel("Dark Souls III", self)
        ds3_path_input = SavePathInput("Dark Souls III", self)
        ds3_path_input.update_path(
            config_info.ds3_save_path.save_path,
            config_info.ds3_save_path.save_path_hint
        )

        sekiro_path_label = QtWidgets.QLabel(
            "Sekiro: Shadows Die Twice", self
        )
        sekiro_path_input = SavePathInput(
            "Sekiro: Shadows Die Twice", self
        )
        sekiro_path_input.update_path(
            config_info.sekiro_save_path.save_path,
            config_info.sekiro_save_path.save_path_hint
        )

        er_path_label = QtWidgets.QLabel("Elden Ring", self)
        er_path_input = SavePathInput("Elden Ring", self)
        er_path_input.update_path(
            config_info.er_save_path.save_path,
            config_info.er_save_path.save_path_hint
        )

        main_layout = QtWidgets.QGridLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        row = 1
        for label_w, input_w in (
            (dsr_path_label, dsr_path_input),
            (ds2_path_label, ds2_path_input),
            (ds3_path_label, ds3_path_input),
            (sekiro_path_label, sekiro_path_input),
            (er_path_label, er_path_input),
        ):
            main_layout.addWidget(label_w, row, 0)
            main_layout.addWidget(input_w, row, 1)
            row += 1

        main_layout.setColumnStretch(0, 0)
        main_layout.setColumnStretch(1, 1)

        self._dsr_path_input = dsr_path_input
        self._ds2_path_input = ds2_path_input
        self._ds3_path_input = ds3_path_input
        self._sekiro_path_input = sekiro_path_input
        self._er_path_input = er_path_input

    def update_config_info(self, config_info: ConfigInfo):
        self._dsr_path_input.update_path(
            config_info.dsr_save_path.save_path,
            config_info.dsr_save_path.save_path_hint
        )
        self._ds2_path_input.update_path(
            config_info.ds2_save_path.save_path,
            config_info.ds2_save_path.save_path_hint
        )
        self._ds3_path_input.update_path(
            config_info.ds3_save_path.save_path,
            config_info.ds3_save_path.save_path_hint
        )
        self._sekiro_path_input.update_path(
            config_info.sekiro_save_path.save_path,
            config_info.sekiro_save_path.save_path_hint
        )
        self._er_path_input.update_path(
            config_info.er_save_path.save_path,
            config_info.er_save_path.save_path_hint
        )

    def apply_changes(self, config_info: ConfigInfo, data: ConfigConfirmData):
        dsr_path = self._dsr_path_input.get_path()
        ds2_path = self._ds2_path_input.get_path()
        ds3_path = self._ds3_path_input.get_path()
        sekiro_path = self._sekiro_path_input.get_path()
        er_path = self._er_path_input.get_path()

        if dsr_path != config_info.dsr_save_path.save_path:
            data.dsr_save_path = dsr_path
        if ds2_path != config_info.ds2_save_path.save_path:
            data.ds2_save_path = ds2_path
        if ds3_path != config_info.ds3_save_path.save_path:
            data.ds3_save_path = ds3_path
        if sekiro_path != config_info.sekiro_save_path.save_path:
            data.sekiro_save_path = sekiro_path
        if er_path != config_info.er_save_path.save_path:
            data.er_save_path = er_path


class HotkeysWidget(QtWidgets.QWidget):
    def __init__(self, config_info: ConfigInfo, parent: QtWidgets.QWidget):
        super().__init__(parent)

        qs_label = QtWidgets.QLabel("QuickSave", self)
        quicksave_input = HotkeyInput(self)
        quicksave_input.set_combination(config_info.quicksave_hotkey)
        ql_label = QtWidgets.QLabel("QuickLoad", self)
        quickload_input = HotkeyInput(self)
        quickload_input.set_combination(config_info.quickload_hotkey)

        hotkeys_layout = QtWidgets.QGridLayout(self)
        hotkeys_layout.setContentsMargins(0, 0, 0, 0)
        hotkeys_layout.addWidget(qs_label, 1, 0)
        hotkeys_layout.addWidget(quicksave_input, 1, 1)
        hotkeys_layout.addWidget(ql_label, 2, 0)
        hotkeys_layout.addWidget(quickload_input, 2, 1)

        self._quicksave_input = quicksave_input
        self._quickload_input = quickload_input

    def update_config_info(self, config_info: ConfigInfo):
        self._quicksave_input.set_combination(config_info.quicksave_hotkey)
        self._quickload_input.set_combination(config_info.quickload_hotkey)

    def apply_changes(self, config_info: ConfigInfo, data: ConfigConfirmData):
        quicksave_hotkey = self._quicksave_input.get_key_combination()
        quickload_hotkey = self._quickload_input.get_key_combination()

        if quicksave_hotkey != config_info.quicksave_hotkey:
            if quicksave_hotkey is None:
                quicksave_hotkey = QtCore.QKeyCombination(
                    QtCore.Qt.Key_unknown
                )
            data.quicksave_hotkey = quicksave_hotkey
        if quickload_hotkey != config_info.quickload_hotkey:
            if quickload_hotkey is None:
                quickload_hotkey = QtCore.QKeyCombination(
                    QtCore.Qt.Key_unknown
                )
            data.quickload_hotkey = quickload_hotkey


class SettingsWidget(QtWidgets.QWidget):
    def __init__(self, controller, parent):
        super().__init__(parent)

        config_info: ConfigInfo = controller.get_config_info()

        # TODO add option to reset to default
        paths_label = QtWidgets.QLabel("Paths", self)
        paths_label.setObjectName("settings_header")
        paths_widget = SavePathInputsWidget(config_info, self)

        hotkeys_label = QtWidgets.QLabel("Hotkeys", self)
        hotkeys_label.setObjectName("settings_header")
        hotkeys_widget = HotkeysWidget(config_info, self)

        btns_widget = QtWidgets.QWidget(self)

        save_btn = QtWidgets.QPushButton("Save", btns_widget)
        save_btn.setObjectName("save_btn")
        discard_btn = QtWidgets.QPushButton("Discard changes", btns_widget)

        btns_layout = QtWidgets.QHBoxLayout(btns_widget)
        btns_layout.setContentsMargins(0, 0, 0, 0)
        btns_layout.addStretch(1)
        btns_layout.addWidget(save_btn, 0)
        btns_layout.addWidget(discard_btn, 0)

        main_layout = QtWidgets.QVBoxLayout(self)
        main_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.setSpacing(5)
        main_layout.addWidget(paths_label, 0)
        main_layout.addWidget(paths_widget, 0)
        main_layout.addSpacing(10)
        main_layout.addWidget(hotkeys_label, 0)
        main_layout.addWidget(hotkeys_widget, 0)
        main_layout.addStretch(1)
        main_layout.addWidget(btns_widget, 0)
        save_btn.clicked.connect(self._on_save)
        discard_btn.clicked.connect(self.discard_changes)

        self._controller = controller
        self._config_info = config_info
        self._paths_widget = paths_widget
        self._hotkeys_widget = hotkeys_widget

    def _on_save(self):
        self._controller.save_config_info(self._get_values())

    def discard_changes(self):
        config_info: ConfigInfo = self._controller.get_config_info()
        self._config_info = config_info
        self._paths_widget.update_config_info(config_info)
        self._hotkeys_widget.update_config_info(config_info)

    def _get_values(self) -> ConfigConfirmData:
        data = ConfigConfirmData()
        self._paths_widget.apply_changes(self._config_info, data)
        self._hotkeys_widget.apply_changes(self._config_info, data)
        return data
