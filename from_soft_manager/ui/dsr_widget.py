from typing import Optional

from PySide6 import QtWidgets, QtCore, QtGui

from from_soft_manager.parse import DSRCharacter

CHAR_ID_ROLE = QtCore.Qt.UserRole + 1
CHAR_NAME_ROLE = QtCore.Qt.UserRole + 2


class CharacterStatusWidget(QtWidgets.QWidget):
    def __init__(self, parent):
        super().__init__(parent)

        name_value_w = QtWidgets.QLabel(self)
        name_value_w.setAlignment(QtCore.Qt.AlignCenter)

        covenant_label = QtWidgets.QLabel("Covenant", self)
        covenant_value_w = QtWidgets.QLabel(self)

        # Don't know how to get covenant yet
        covenant_label.setVisible(False)
        covenant_value_w.setVisible(False)

        level_label = QtWidgets.QLabel("Level", self)
        level_value_w = QtWidgets.QLabel(self)

        souls_label = QtWidgets.QLabel("Souls", self)
        souls_value_w = QtWidgets.QLabel(self)

        vitality_label = QtWidgets.QLabel("Vitality", self)
        vitality_value_w = QtWidgets.QLabel(self)

        attunement_label = QtWidgets.QLabel("Attunement", self)
        attunement_value_w = QtWidgets.QLabel(self)

        endurance_label = QtWidgets.QLabel("Endurance", self)
        endurance_value_w = QtWidgets.QLabel(self)

        strength_label = QtWidgets.QLabel("Strength", self)
        strength_value_w = QtWidgets.QLabel(self)

        dexterity_label = QtWidgets.QLabel("Dexterity", self)
        dexterity_value_w = QtWidgets.QLabel(self)

        resistance_label = QtWidgets.QLabel("Resistance", self)
        resistance_value_w = QtWidgets.QLabel(self)

        intelligence_label = QtWidgets.QLabel("Intelligence", self)
        intelligence_value_w = QtWidgets.QLabel(self)

        faith_label = QtWidgets.QLabel("Faith", self)
        faith_value_w = QtWidgets.QLabel(self)

        humanity_label = QtWidgets.QLabel("Humanity", self)
        humanity_value_w = QtWidgets.QLabel(self)

        main_layout = QtWidgets.QGridLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(name_value_w, 0, 0, 1, 2)

        for label_w, value_w in (
            (covenant_label, covenant_value_w),
            (level_label, level_value_w),
            (souls_label, souls_value_w),
            (vitality_label, vitality_value_w),
            (attunement_label, attunement_value_w),
            (endurance_label, endurance_value_w),
            (strength_label, strength_value_w),
            (dexterity_label, dexterity_value_w),
            (resistance_label, resistance_value_w),
            (intelligence_label, intelligence_value_w),
            (faith_label, faith_value_w),
            (humanity_label, humanity_value_w)
        ):
            row = main_layout.rowCount()
            label_w.setAlignment(QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter)
            value_w.setAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
            main_layout.addWidget(label_w, row, 0)
            main_layout.addWidget(value_w, row, 1)

        main_layout.addWidget(covenant_label, 1, 0)

        self._char = None
        self._name_label = name_value_w
        self._covenant_value_w = covenant_value_w
        self._level_value_w = level_value_w
        self._souls_value_w= souls_value_w
        self._vitality_value_w = vitality_value_w
        self._attunement_value_w = attunement_value_w
        self._endurance_value_w = endurance_value_w
        self._strength_value_w = strength_value_w
        self._dexterity_value_w = dexterity_value_w
        self._resistance_value_w = resistance_value_w
        self._intelligence_value_w = intelligence_value_w
        self._faith_value_w = faith_value_w
        self._humanity_value_w = humanity_value_w

        self.set_char(None)

    def set_char(self, char):
        if char is None:
            self._set_empty()
            return

        self._name_label.setText(char.name)
        self._covenant_value_w.setText("N/A")
        for widget, value in (
            (self._level_value_w, char.level),
            (self._souls_value_w, char.souls),
            (self._vitality_value_w, char.vitality),
            (self._attunement_value_w, char.attunement),
            (self._endurance_value_w, char.endurance),
            (self._strength_value_w, char.strength),
            (self._dexterity_value_w, char.dexterity),
            (self._resistance_value_w, char.resistance),
            (self._intelligence_value_w, char.intelligence),
            (self._faith_value_w, char.faith),
            (self._humanity_value_w, char.humanity),
        ):
            widget.setText(str(value))

    def _set_empty(self):
        self._name_label.setText("< Empty >")
        for value_w in (
            self._covenant_value_w,
            self._level_value_w,
            self._souls_value_w,
            self._vitality_value_w,
            self._attunement_value_w,
            self._endurance_value_w,
            self._strength_value_w,
            self._dexterity_value_w,
            self._resistance_value_w,
            self._intelligence_value_w,
            self._faith_value_w,
            self._humanity_value_w,
        ):
            value_w.setText("")


class CharacterInfoWidget(QtWidgets.QWidget):
    def __init__(self, parent):
        super().__init__(parent)

        status_widget = CharacterStatusWidget(self)

        main_layout = QtWidgets.QHBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(status_widget, 1)

        self._status_widget = status_widget

    def set_char(self, char):
        self._status_widget.set_char(char)


class CharsListModel(QtGui.QStandardItemModel):
    def __init__(self, controller, parent):
        super().__init__(parent)
        self.setColumnCount(1)

        self._controller = controller
        self._chars_by_id: dict[int, Optional[DSRCharacter]] = {
            idx : None
            for idx in range(10)
        }

    def refresh(self):
        # TODO capture error and use it in first item (NoFlags)
        dsr_chars = self._controller.get_dsr_chars()

        root_item = self.invisibleRootItem()
        if not dsr_chars:
            self._chars_by_id = {
                idx : None
                for idx in range(10)
            }
            root_item.removeRows(0, root_item.rowCount())
            item = QtGui.QStandardItem("No characters found")
            item.setFlags(QtCore.Qt.NoItemFlags)
            root_item.appendRow(item)
            return

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

            # male = "male" if sex == 1 else "female"
            # player_class = CLASSES[player_class_id]
            # physique = PHYSIQUE[physique_id]
            # gift = GIFTS[gift_id]

            # face = FACE_TYPES[face_id]
            # hair_style = HAIR_TYPES[hair_style_id]
            # hair_color = HAIR_COLORS[hair_color_id]
            if char is None:
                index = idx
                item.setData("Empty", QtCore.Qt.DisplayRole)
                item.setData(None, CHAR_NAME_ROLE)
            else:
                index = char.index
                item.setData(char.name, QtCore.Qt.DisplayRole)
                item.setData(char.name, CHAR_NAME_ROLE)
            item.setData(index, CHAR_ID_ROLE)

        if new_items:
            root_item.appendRows(new_items)

    def get_char_by_id(self, item_id: int) -> Optional[DSRCharacter]:
        return self._chars_by_id.get(item_id)


class DSRWidget(QtWidgets.QWidget):
    def __init__(self, controller, parent):
        super().__init__(parent)

        view = QtWidgets.QTreeView(self)
        view.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        view.setTextElideMode(QtCore.Qt.ElideLeft)
        view.setHeaderHidden(True)
        view.setVerticalScrollMode(QtWidgets.QAbstractItemView.ScrollPerPixel)

        model = CharsListModel(controller, view)
        # TODO better refresh logic
        model.refresh()
        view.setModel(model)

        char_info_widget = CharacterInfoWidget(self)

        main_layout = QtWidgets.QHBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(view, 0)
        main_layout.addWidget(char_info_widget, 1)

        view.selectionModel().selectionChanged.connect(
            self._on_selection_change
        )

        self._view = view
        self._model = model
        self._char_info_widget = char_info_widget
        self._widgets = []

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

    #
    # def showEvent(self, event):
    #     super().showEvent(event)
    #     self._update_widgets_size_hints()
    #
    # def resizeEvent(self, event):
    #     super().resizeEvent(event)
    #     self._update_widgets_size_hints()
    #
    # def _update_widgets_size_hints(self):
    #     for item in self._widgets:
    #         widget, index = item
    #         if not widget.isVisible():
    #             continue
    #         self._model.setData(
    #             index, widget.sizeHint(), QtCore.Qt.SizeHintRole
    #         )
