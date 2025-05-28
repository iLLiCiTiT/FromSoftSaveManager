from typing import Optional

from PySide6 import QtWidgets, QtCore, QtGui

from from_soft_manager.parse import DSRCharacter
from from_soft_manager.ui.utils import TabWidget

from .resources import get_resource
from .info import CharacterInfoWidget

CHAR_ID_ROLE = QtCore.Qt.UserRole + 1
CHAR_NAME_ROLE = QtCore.Qt.UserRole + 2


class CharsListModel(QtGui.QStandardItemModel):
    refreshed = QtCore.Signal()

    def __init__(self, controller, save_id, parent):
        super().__init__(parent)
        self.setColumnCount(1)

        self._controller = controller
        self._save_id = save_id

        self._chars_by_id: dict[int, Optional[DSRCharacter]] = {
            idx : None
            for idx in range(10)
        }

    def refresh(self):
        chars_info = self._controller.get_dsr_characters(self._save_id)
        # TODO capture error and use it in first item (NoFlags)
        root_item = self.invisibleRootItem()
        if chars_info.error:
            self._chars_by_id = {
                idx : None
                for idx in range(10)
            }
            root_item.removeRows(0, root_item.rowCount())
            item = QtGui.QStandardItem(chars_info.error)
            item.setFlags(QtCore.Qt.NoItemFlags)
            root_item.appendRow(item)
            self.refreshed.emit()
            return

        dsr_chars = chars_info.characters
        self._chars_by_id = {
            idx: char
            for idx, char in enumerate(dsr_chars)
        }
        new_items = []
        for idx, char in enumerate(dsr_chars):
            item = root_item.child(idx)
            if item is None:
                item = QtGui.QStandardItem()
                new_items.append(item)

            item.setFlags(
                QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled
            )
            # male = "male" if sex == 1 else "female"
            # player_class = CLASSES[player_class_id]
            # physique = PHYSIQUE[physique_id]
            # gift = GIFTS[gift_id]

            # face = FACE_TYPES[face_id]
            # hair_style = HAIR_TYPES[hair_style_id]
            # hair_color = HAIR_COLORS[hair_color_id]
            if char is None:
                index = idx
                item.setData("< Empty >", QtCore.Qt.DisplayRole)
                item.setData(None, CHAR_NAME_ROLE)
            else:
                index = char.index
                item.setData(char.name, QtCore.Qt.DisplayRole)
                item.setData(char.name, CHAR_NAME_ROLE)
            item.setData(index, CHAR_ID_ROLE)

        if new_items:
            root_item.appendRows(new_items)
        self.refreshed.emit()

    def get_char_by_id(self, item_id: int) -> Optional[DSRCharacter]:
        return self._chars_by_id.get(item_id)


class DSRWidget(QtWidgets.QWidget):
    _bg_pix = None

    def __init__(self, controller, save_id, parent):
        super().__init__(parent)

        view_wrap = QtWidgets.QWidget(self)

        view = QtWidgets.QListView(view_wrap)
        view.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        view.setTextElideMode(QtCore.Qt.ElideLeft)
        view.setVerticalScrollMode(QtWidgets.QAbstractItemView.ScrollPerPixel)

        model = CharsListModel(controller, save_id, view)
        view.setModel(model)

        view_wrap_layout = QtWidgets.QHBoxLayout(view_wrap)
        view_wrap_layout.setContentsMargins(24, 24, 0, 0)
        view_wrap_layout.addWidget(view)

        char_tabs = TabWidget(self)

        char_info_widget = CharacterInfoWidget(char_tabs)
        # char_equip_widget = QtWidgets.QWidget(char_tabs)

        char_tabs.add_tab(
            "Character Info",
            char_info_widget,
        )
        # char_tabs.add_tab(
        #     "Equipment",
        #     char_equip_widget,
        # )

        main_layout = QtWidgets.QHBoxLayout(self)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.addWidget(view_wrap, 0)
        main_layout.addWidget(char_tabs, 1)

        view.selectionModel().selectionChanged.connect(
            self._on_selection_change
        )
        model.refreshed.connect(self._on_refresh)

        self.save_id = save_id
        self._view = view
        self._model = model
        self._char_tabs = char_tabs
        self._char_info_widget = char_info_widget

    def refresh(self):
        self._model.refresh()

    def showEvent(self, event):
        super().showEvent(event)
        self._model.refresh()

    def paintEvent(self, event):
        painter = QtGui.QPainter(self)
        pix = self._get_bg_pix()
        painter.drawPixmap(self.rect(), pix)

    @classmethod
    def _get_bg_pix(cls):
        if cls._bg_pix is None:
            cls._bg_pix = QtGui.QPixmap(
                get_resource("bg.png")
            )
        return cls._bg_pix

    def _on_refresh(self):
        sel_model = self._view.selectionModel()
        index = next(iter(sel_model.selectedIndexes()), None)
        if index is not None:
            char_id = index.data(CHAR_ID_ROLE)
            character = self._model.get_char_by_id(char_id)
            self._char_info_widget.set_char(character)
            return

        index = self._model.index(0, 0)
        flags = self._model.flags(index)
        if not flags & QtCore.Qt.ItemIsSelectable:
            sel_model.clear()
            return
        sel_model.setCurrentIndex(
            index,
            QtCore.QItemSelectionModel.ClearAndSelect
        )

    def _on_selection_change(self, selection, _old_selection):
        set_char = False
        for index in selection.indexes():
            item_id = index.data(CHAR_ID_ROLE)
            char = self._model.get_char_by_id(item_id)
            if char is not None:
                self._char_info_widget.set_char(char)
                set_char = True
                break

        if not set_char:
            self._char_info_widget.set_char(None)
