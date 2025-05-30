from PySide6 import QtWidgets

from from_soft_manager.ui.structures import ConfigInfo, ConfigConfirmData


class SettingsDialog(QtWidgets.QDialog):
    def __init__(self, controller, parent):
        super().__init__(parent)
        self.setWindowTitle("Settings")
        self.setModal(True)

        config_info: ConfigInfo = controller.get_config_info()

        # TODO add option to reset to default
        # TODO use icons

        dsr_path_label = QtWidgets.QLabel("Dark Souls: Remastered", self)
        dsr_path_input = QtWidgets.QLineEdit(parent)
        dsr_path_input.setText(config_info.dsr_save_path.save_path)
        dsr_path_input.setPlaceholderText("< Path to save file >")
        dsr_open_btn = QtWidgets.QPushButton("Open", self)

        ds2_path_label = QtWidgets.QLabel("Dark Souls II: SOTFS", self)
        ds2_path_input = QtWidgets.QLineEdit(parent)
        ds2_path_input.setText(config_info.ds2_save_path.save_path)
        ds2_path_input.setPlaceholderText("< Path to save file >")
        ds2_open_btn = QtWidgets.QPushButton("Open", self)

        ds3_path_label = QtWidgets.QLabel("Dark Souls III", self)
        ds3_path_input = QtWidgets.QLineEdit(parent)
        ds3_path_input.setText(config_info.ds3_save_path.save_path)
        ds3_path_input.setPlaceholderText("< Path to save file >")
        ds3_open_btn = QtWidgets.QPushButton("Open", self)

        er_path_label = QtWidgets.QLabel("Elden Ring", self)
        er_path_input = QtWidgets.QLineEdit(parent)
        er_path_input.setText(config_info.er_save_path.save_path)
        er_path_input.setPlaceholderText("< Path to save file >")
        er_open_btn = QtWidgets.QPushButton("Open", self)

        btns_widget = QtWidgets.QWidget(self)

        save_btn = QtWidgets.QPushButton("Save", btns_widget)
        cancel_btn = QtWidgets.QPushButton("Cancel", btns_widget)

        btns_layout = QtWidgets.QHBoxLayout(btns_widget)
        btns_layout.setContentsMargins(0, 0, 0, 0)
        btns_layout.addStretch(1)
        btns_layout.addWidget(save_btn, 0)
        btns_layout.addWidget(cancel_btn, 0)

        main_layout = QtWidgets.QGridLayout(self)
        main_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.addWidget(dsr_path_label, 0, 0)
        main_layout.addWidget(dsr_path_input, 0, 1)
        main_layout.addWidget(dsr_open_btn, 0, 2)
        main_layout.addWidget(ds2_path_label, 1, 0)
        main_layout.addWidget(ds2_path_input, 1, 1)
        main_layout.addWidget(ds2_open_btn, 1, 2)
        main_layout.addWidget(ds3_path_label, 2, 0)
        main_layout.addWidget(ds3_path_input, 2, 1)
        main_layout.addWidget(ds3_open_btn, 2, 2)
        main_layout.addWidget(er_path_label, 3, 0)
        main_layout.addWidget(er_path_input, 3, 1)
        main_layout.addWidget(er_open_btn, 3, 2)
        main_layout.addWidget(btns_widget, 5, 0, 1, 3)

        main_layout.setColumnStretch(0, 0)
        main_layout.setColumnStretch(1, 1)
        main_layout.setColumnStretch(2, 0)
        main_layout.setRowStretch(4, 1)

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

        self.resize(600, 200)

    def get_values(self) -> ConfigConfirmData:
        dsr_path = self._dsr_path_input.text()
        ds2_path = self._ds2_path_input.text()
        ds3_path = self._ds3_path_input.text()
        er_path = self._er_path_input.text()
        data = ConfigConfirmData(dsr_path)
        if dsr_path != self._config_info.dsr_save_path.save_path:
            data.dsr_save_path = dsr_path
        if ds2_path != self._config_info.ds2_save_path.save_path:
            data.ds2_save_path = ds2_path
        if ds3_path != self._config_info.ds3_save_path.save_path:
            data.ds3_save_path = ds3_path
        if er_path != self._config_info.er_save_path.save_path:
            data.er_save_path = er_path
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
