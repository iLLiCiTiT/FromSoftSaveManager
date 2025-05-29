import typing
from PySide6 import QtWidgets

from from_soft_manager.parse import Game
from from_soft_manager.ui.structures import ConfigInfo, ConfigConfirmData

if typing.TYPE_CHECKING:
    from from_soft_manager.ui.control import Controller

from .style import load_stylesheet
from .dsr_widget import DSRWidget


class SettingsDialog(QtWidgets.QDialog):
    def __init__(self, controller, parent):
        super().__init__(parent)
        self.setWindowTitle("Settings")
        self.setModal(True)

        config_info: ConfigInfo = controller.get_config_info()

        dsr_path = config_info.dsr_save_path

        dsr_path_label = QtWidgets.QLabel("Dark Souls: Remastered", self)
        # TODO add option to reset to default
        dsr_path_input = QtWidgets.QLineEdit(parent)
        dsr_path_input.setText(dsr_path)
        dsr_path_input.setPlaceholderText("< Path to save file >")
        # TODO use icon
        dsr_open_btn = QtWidgets.QPushButton("Open", self)

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
        main_layout.addWidget(btns_widget, 2, 0, 1, 3)

        main_layout.setColumnStretch(0, 0)
        main_layout.setColumnStretch(1, 1)
        main_layout.setColumnStretch(2, 0)
        main_layout.setRowStretch(1, 1)

        dsr_open_btn.clicked.connect(self._on_dsr_open_click)
        save_btn.clicked.connect(self.accept)
        cancel_btn.clicked.connect(self.reject)

        self._config_info = config_info
        self._orig_dsr_path = dsr_path
        self._dsr_path_input = dsr_path_input

        self.resize(600, 200)

    def get_values(self):
        dsr_path = self._dsr_path_input.text()
        if dsr_path == self._orig_dsr_path:
            return None
        return ConfigConfirmData(dsr_path)

    def _on_dsr_open_click(self):
        path, _ = QtWidgets.QFileDialog.getOpenFileName(
            self,
            "Select Dark Souls: Remastered Save File",
            self._config_info.dsr_save_path_hint,
            "SL2 files (*.sl2);;All Files (*)")
        if not path:
            return
        self._dsr_path_input.setText(path)


class MainWindow(QtWidgets.QDialog):
    def __init__(self, controller: "Controller"):
        super().__init__()
        self.setWindowTitle("FromSoft Save Manager")

        header_widget = QtWidgets.QWidget(self)

        settings_btn = QtWidgets.QPushButton("Settings", header_widget)

        header_layout = QtWidgets.QHBoxLayout(header_widget)
        header_layout.setContentsMargins(0, 0, 0, 0)
        header_layout.addStretch(1)
        header_layout.addWidget(settings_btn, 0)

        content_widget = QtWidgets.QWidget(self)
        content_layout = QtWidgets.QHBoxLayout(content_widget)
        content_layout.setContentsMargins(0, 0, 0, 0)

        main_layout = QtWidgets.QVBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(header_widget, 0)
        main_layout.addWidget(content_widget, 1)

        settings_btn.clicked.connect(self._on_settings_click)
        controller.paths_changed.connect(self.refresh)

        # TODO add tabs for different widgets (when supported)
        self._widgets = []
        self._controller = controller
        self._content_widget = content_widget
        self._content_layout = content_layout

        self.resize(800, 400)

    def showEvent(self, event):
        super().showEvent(event)
        self.setStyleSheet(load_stylesheet())
        self.refresh()

    def refresh(self):
        widgets_by_id = {
            widget.save_id: widget
            for widget in self._widgets
        }
        save_items = self._controller.get_save_items()
        # TODO find out if current save is still available and change tab
        #    if not.
        # TODO remove all widgets that are not in save_items
        used_ids = set()
        first_widget = None
        for save_item in save_items:
            used_ids.add(save_item.save_id)
            widget = widgets_by_id.get(save_item.save_id)
            if widget is not None:
                widget.refresh()
            elif save_item.game == Game.DSR:
                widget = DSRWidget(self._controller, save_item.save_id, self)
                self._content_layout.addWidget(widget, 1)
                self._widgets.append(widget)

            if widget is None:
                continue
            if first_widget is None:
                first_widget = widget
            else:
                widget.setVisible(False)

        for widget in tuple(self._widgets):
            if widget.save_id not in used_ids:
                self._content_layout.removeWidget(widget)
                widget.deleteLater()
            else:
                self._controller.set_current_save_id(widget.save_id)

    def _on_settings_click(self):
        dialog = SettingsDialog(self._controller, self)
        if dialog.exec_() == QtWidgets.QDialog.Accepted:
            values = dialog.get_values()
            if values is not None:
                self._controller.save_config_info(values)
