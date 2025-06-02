import typing
from PySide6 import QtWidgets, QtCore, QtGui

from from_soft_manager import __version__
from from_soft_manager.parse import Game

if typing.TYPE_CHECKING:
    from from_soft_manager.ui.control import Controller

from .style import load_stylesheet
from .icons import get_icon_path
from .settings import SettingsWidget
from .dsr_widget import DSRWidget
from .ds2sotfs_widget import DS2SOTFSWidget
from .ds3_widget import DS3Widget
from .er_widget import ERWidget
from .utils import SquareButton


def set_btn_selected(btn: QtWidgets.QPushButton, selected: bool):
    btn._selected = selected
    value = "1" if selected else ""
    btn.setProperty("selected", value)
    btn.style().polish(btn)


class GameSaveTabButton(SquareButton):
    requested = QtCore.Signal(str)
    _icons_cache = {}

    def __init__(
        self,
        game: Game,
        save_id: str,
        tab_label: str,
        parent: QtWidgets.QWidget,
    ):
        super().__init__(parent)
        icon = self.get_icon(game)
        if icon:
            self.setIcon(icon)
        self.clicked.connect(self._on_click)
        self.setDefault(False)
        self.setAutoDefault(False)

        self._save_id = save_id

        self._selected = False

    @classmethod
    def get_icon(self, game: Game):
        if game not in self._icons_cache:
            filename = f"{game}_256x256.png"
            icon_path = get_icon_path(filename)
            if icon_path is None:
                return None
            self._icons_cache[game] = QtGui.QIcon(icon_path)
        return self._icons_cache[game]

    def _on_click(self):
        self.requested.emit(self._save_id)


class SideBarWidget(QtWidgets.QFrame):
    tab_changed = QtCore.Signal(str)
    settings_requested = QtCore.Signal()

    def __init__(self, parent):
        super().__init__(parent)

        settings_icon = QtGui.QIcon(get_icon_path("settings_256x256.png"))
        settings_btn = SquareButton(self)
        settings_btn.setObjectName("settings_btn")
        settings_btn.setIcon(settings_icon)

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(4, 4, 4, 4)
        layout.setSpacing(3)
        layout.addStretch(1)
        layout.addWidget(settings_btn, 0)

        settings_btn.clicked.connect(self._on_settings_request)

        self._layout = layout

        self._settings_btn = settings_btn
        self._current_tab_id = None
        self._widgets_by_id = {}

    def add_tab(self, game: Game, save_id: str, title: str | None = None):
        if title is None:
            if game == Game.DSR:
                title = "DS: Remastered"
            elif game == Game.DS2_SOTFS:
                title = "DS II: SotFS"
            elif game == Game.DS3:
                title = "Dark Souls III"
            elif game == Game.ER:
                title = "Elden Ring"
        tab_btn = GameSaveTabButton(game, save_id, title, self)
        tab_btn.requested.connect(self.set_current_tab)
        self._widgets_by_id[save_id] = tab_btn
        self._layout.insertWidget(self._layout.count() - 2, tab_btn, 0)

    def remove_tab(self, save_id: str):
        if save_id == self._current_tab_id:
            self.set_current_tab(None)
        widget = self._widgets_by_id.pop(save_id, None)
        if widget is None:
            return
        idx = self._layout.indexOf(widget)
        if idx != -1:
            self._layout.takeAt(idx)
        widget.setVisible(False)
        widget.deleteLater()

    def set_current_tab(self, save_id: str | None):
        if self._current_tab_id is None:
            current_tab = self._settings_btn
        else:
            current_tab = self._widgets_by_id.get(self._current_tab_id)

        if current_tab is not None:
            set_btn_selected(current_tab, False)

        self._current_tab_id = save_id
        if save_id is None:
            btn = self._settings_btn
        else:
            btn = self._widgets_by_id.get(save_id)
        if btn is not None:
            set_btn_selected(btn, True)

        self.tab_changed.emit(save_id or "")

    def _on_settings_request(self):
        self.settings_requested.emit()
        self.set_current_tab(None)


class MainWindow(QtWidgets.QDialog):
    def __init__(self, controller: "Controller"):
        super().__init__()

        icon = QtGui.QIcon(get_icon_path("icon.png"))
        self.setWindowTitle(f"FromSoft Save Manager - {__version__}")
        self.setWindowIcon(icon)

        side_bar_widget = SideBarWidget(self)

        content_widget = QtWidgets.QWidget(self)

        settings_widget = SettingsWidget(controller, content_widget)

        content_layout = QtWidgets.QStackedLayout(content_widget)
        content_layout.setContentsMargins(0, 0, 0, 0)
        content_layout.addWidget(settings_widget)

        main_layout = QtWidgets.QHBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)
        main_layout.addWidget(side_bar_widget, 0)
        main_layout.addWidget(content_widget, 1)

        side_bar_widget.settings_requested.connect(self._on_settings_click)
        side_bar_widget.tab_changed.connect(self._set_current_save_id)
        controller.paths_changed.connect(self.refresh)
        controller.save_id_changed.connect(self._on_save_id_change)

        # TODO add tabs for different widgets (when supported)
        self._current_save_id = None
        self._settings_widget = settings_widget
        self._widgets_by_id = {}

        self._side_bar_widget = side_bar_widget
        self._controller = controller
        self._content_widget = content_widget
        self._content_layout = content_layout

        self.resize(960, 490)

    def showEvent(self, event):
        super().showEvent(event)
        self.setStyleSheet(load_stylesheet())
        self.refresh()

    def refresh(self):
        save_items = self._controller.get_save_items()
        # TODO find out if current save is still available and change tab
        #    if not.
        used_ids = set()
        first_widget = None
        for save_item in save_items:
            used_ids.add(save_item.save_id)
            widget = self._widgets_by_id.get(save_item.save_id)
            if widget is not None:
                widget.refresh()
                if first_widget is None:
                    first_widget = widget
                continue

            if save_item.game == Game.DSR:
                widget = DSRWidget(self._controller, save_item.save_id, self)
            elif save_item.game == Game.DS2_SOTFS:
                widget = DS2SOTFSWidget(
                    self._controller, save_item.save_id, self
                )
            elif save_item.game == Game.DS3:
                widget = DS3Widget(self._controller, save_item.save_id, self)
            elif save_item.game == Game.ER:
                widget = ERWidget(self._controller, save_item.save_id, self)

            if widget is None:
                continue

            widget.refresh()
            if first_widget is None:
                first_widget = widget

            self._content_layout.addWidget(widget)
            self._widgets_by_id[save_item.save_id] = widget

            self._side_bar_widget.add_tab(
                save_item.game,
                save_item.save_id,
            )

        current_widget = self._widgets_by_id.get(self._current_save_id)
        if current_widget is None:
            save_id = None
            if first_widget is not None:
                save_id = first_widget.save_id
            self._side_bar_widget.set_current_tab(save_id)

        for save_id in tuple(self._widgets_by_id.keys()):
            if save_id in used_ids:
                continue
            widget = self._widgets_by_id.pop(save_id)
            idx = self._content_layout.indexOf(widget)
            if idx != -1:
                self._content_layout.takeAt(idx)
            widget.setVisible(False)
            widget.deleteLater()
            self._side_bar_widget.remove_tab(save_id)

    def _on_save_id_change(self, save_id: str):
        widget = self._widgets_by_id.get(save_id)
        if widget is not None:
            widget.refresh()

    def _set_current_save_id(self, save_id: str):
        if self._current_save_id == save_id:
            return

        self._current_save_id = save_id

        current_widget = self._content_layout.currentWidget()
        if current_widget is self._settings_widget:
            self._settings_widget.discard_changes()

        new_widget = self._widgets_by_id.get(save_id)
        if new_widget is not None:
            self._content_layout.setCurrentWidget(new_widget)

        self._controller.set_current_save_id(self._current_save_id)

    def _on_settings_click(self):
        current_widget = self._widgets_by_id.get(self._current_save_id)
        if current_widget is self._settings_widget:
            return

        self._current_save_id = None
        self._controller.set_current_save_id(None)
        self._content_layout.setCurrentWidget(self._settings_widget)
