from PySide6 import QtWidgets, QtCore, QtGui

from from_soft_manager.parse import EldenRingCharacter
from from_soft_manager.ui.er_widget.resources import get_resource
from from_soft_manager.ui.utils import PixmapLabel


class AttrSepWidget(QtWidgets.QWidget):
    def __init__(self, parent):
        super().__init__(parent)
        self._pix = QtGui.QPixmap(get_resource("menu", "sep_1"))
        self.setSizePolicy(
            QtWidgets.QSizePolicy.Minimum,
            QtWidgets.QSizePolicy.Expanding
        )

    def sizeHint(self):
        return QtCore.QSize(6, 0)

    def paintEvent(self, event):
        painter = QtGui.QPainter(self)
        painter.setPen(QtCore.Qt.NoPen)
        painter.drawPixmap(event.rect(), self._pix)


class NameSepWidget(QtWidgets.QWidget):
    def __init__(self, parent):
        super().__init__(parent)
        self._pix = QtGui.QPixmap(get_resource("menu", "sep_2"))
        self.setSizePolicy(
            QtWidgets.QSizePolicy.Expanding,
            QtWidgets.QSizePolicy.Minimum
        )

    def sizeHint(self):
        return QtCore.QSize(0, 10)

    def paintEvent(self, event):
        painter = QtGui.QPainter(self)
        painter.setPen(QtCore.Qt.NoPen)
        painter.drawPixmap(event.rect(), self._pix)


class NameWidget(QtWidgets.QWidget):
    def __init__(self, parent):
        super().__init__(parent)

        name_label = QtWidgets.QLabel(self)
        sep_widget = NameSepWidget(self)

        main_layout = QtWidgets.QVBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)
        main_layout.addWidget(name_label, 0)
        main_layout.addWidget(sep_widget, 0)

        for widget in (
            name_label,
            sep_widget,
        ):
            widget.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)

        self._name_label = name_label
        self._sep_widget = sep_widget

    def setText(self, name: str):
        self._name_label.setText(name)


class ERAttributesLabel(QtWidgets.QWidget):
    def __init__(self, parent):
        super().__init__(parent)

        pix = QtGui.QPixmap(get_resource("menu", "attrs_icon.png"))
        icon_widget = PixmapLabel(pix, self)
        label_widget = QtWidgets.QLabel("Attribute Points")

        main_layout = QtWidgets.QHBoxLayout(self)
        main_layout.setSpacing(2)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(icon_widget, 0)
        main_layout.addWidget(label_widget, 1)

        for widget in (self, icon_widget, label_widget):
            widget.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)

        self._icon_widget = icon_widget
        self._label_widget = label_widget


class CharacterStatusWidget(QtWidgets.QWidget):
    def __init__(self, parent):
        super().__init__(parent)

        attributes_widget = QtWidgets.QWidget(self)

        name_value_w = NameWidget(attributes_widget)

        spacer_1 = QtWidgets.QWidget(attributes_widget)
        spacer_1.setFixedHeight(14)

        info_sep = AttrSepWidget(attributes_widget)

        level_label = QtWidgets.QLabel("Level", attributes_widget)
        level_value_w = QtWidgets.QLabel(attributes_widget)

        runes_label = QtWidgets.QLabel("Runes", attributes_widget)
        runes_value_w = QtWidgets.QLabel(attributes_widget)

        spacer_2 = QtWidgets.QWidget(attributes_widget)
        spacer_2.setFixedHeight(14)

        attributes_label = ERAttributesLabel(attributes_widget)

        attributes_sep = AttrSepWidget(attributes_widget)

        vigor_label = QtWidgets.QLabel("Vigor", attributes_widget)
        vigor_value_w = QtWidgets.QLabel(attributes_widget)

        mind_label = QtWidgets.QLabel("Mind", attributes_widget)
        mind_value_w = QtWidgets.QLabel(attributes_widget)

        endurance_label = QtWidgets.QLabel("Endurance", attributes_widget)
        endurance_value_w = QtWidgets.QLabel(attributes_widget)

        strength_label = QtWidgets.QLabel("Strength", attributes_widget)
        strength_value_w = QtWidgets.QLabel(attributes_widget)

        dexterity_label = QtWidgets.QLabel("Dexterity", attributes_widget)
        dexterity_value_w = QtWidgets.QLabel(attributes_widget)

        intelligence_label = QtWidgets.QLabel("Intelligence", attributes_widget)
        intelligence_value_w = QtWidgets.QLabel(attributes_widget)

        faith_label = QtWidgets.QLabel("Faith", attributes_widget)
        faith_value_w = QtWidgets.QLabel(attributes_widget)

        arcane_label = QtWidgets.QLabel("Arcane", attributes_widget)
        arcane_value_w = QtWidgets.QLabel(attributes_widget)

        attributes_layout = QtWidgets.QGridLayout(attributes_widget)
        attributes_layout.setSpacing(2)
        attributes_layout.setContentsMargins(0, 0, 0, 0)
        attributes_layout.addWidget(name_value_w, 0, 0, 1, 3)
        attributes_layout.addWidget(spacer_1, 1, 0)

        level_row = attrs_row = 0
        for label_w, value_w in (
            (level_label, level_value_w),
            (runes_label, runes_value_w),
            (vigor_label, vigor_value_w),
            (mind_label, mind_value_w),
            (endurance_label, endurance_value_w),
            (strength_label, strength_value_w),
            (dexterity_label, dexterity_value_w),
            (intelligence_label, intelligence_value_w),
            (faith_label, faith_value_w),
            (arcane_label, arcane_value_w),
        ):
            row = attributes_layout.rowCount()
            label_w.setAlignment(QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter)
            value_w.setAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
            attributes_layout.addWidget(label_w, row, 1)
            attributes_layout.addWidget(value_w, row, 2)
            for widget in (label_w, value_w):
                widget.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)

            if label_w is runes_label:
                attributes_layout.addWidget(spacer_2, row + 1, 0)
                attributes_layout.addWidget(attributes_label, row + 2, 0, 1, 3)
            elif label_w is level_label:
                level_row = row
            elif label_w is vigor_label:
                attrs_row = row

        attributes_layout.addWidget(info_sep, level_row, 0, 2, 1)
        attributes_layout.addWidget(attributes_sep, attrs_row, 0, 8, 1)

        attributes_layout.setRowStretch(attributes_layout.rowCount(), 1)
        attributes_layout.setColumnStretch(0, 0)
        attributes_layout.setColumnStretch(1, 1)
        attributes_layout.setColumnStretch(2, 1)

        stats_widget = QtWidgets.QWidget(self)

        hp_label = QtWidgets.QLabel("HP", stats_widget)
        hp_value_w = QtWidgets.QLabel(stats_widget)

        fp_label = QtWidgets.QLabel("FP", stats_widget)
        fp_value_w = QtWidgets.QLabel(stats_widget)

        stamina_label = QtWidgets.QLabel("Stamina", stats_widget)
        stamina_value_w = QtWidgets.QLabel(stats_widget)

        stats_layout = QtWidgets.QGridLayout(stats_widget)
        stats_layout.setContentsMargins(0, 0, 0, 0)

        for label_w, value_w in (
            (hp_label, hp_value_w),
            (fp_label, fp_value_w),
            (stamina_label, stamina_value_w),
        ):
            row = stats_layout.rowCount()
            label_w.setAlignment(QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter)
            value_w.setAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
            stats_layout.addWidget(label_w, row, 0, 1, 2)
            stats_layout.addWidget(value_w, row, 2)
            for widget in (label_w, value_w):
                if widget is None:
                    continue
                widget.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)

        stats_layout.setRowStretch(stats_layout.rowCount(), 1)
        stats_layout.setColumnStretch(0, 0)
        stats_layout.setColumnStretch(1, 1)
        stats_layout.setColumnStretch(2, 1)

        main_layout = QtWidgets.QHBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(attributes_widget, 1)
        main_layout.addWidget(stats_widget, 1)

        for widget in (
            self,
            attributes_widget,
            name_value_w,
            stats_widget,
            spacer_1,
            spacer_2,
            info_sep,
            attributes_sep,
        ):
            widget.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)

        self._char = None
        self._name_label = name_value_w
        self._level_value_w = level_value_w
        self._runes_value_w= runes_value_w
        self._vigor_value_w = vigor_value_w
        self._mind_value_w = mind_value_w
        self._endurance_value_w = endurance_value_w
        self._strength_value_w = strength_value_w
        self._dexterity_value_w = dexterity_value_w
        self._intelligence_value_w = intelligence_value_w
        self._faith_value_w = faith_value_w
        self._arcane_value_w = arcane_value_w

        self._hp_value_w = hp_value_w
        self._fp_value_w = fp_value_w
        self._stamina_value_w = stamina_value_w

        self.set_char(None)

    def set_char(self, char: EldenRingCharacter | None):
        if char is None:
            self._set_empty()
            return

        self._name_label.setText(char.name)
        for widget, value in (
            (self._level_value_w, char.level),
            (self._runes_value_w, f"{char.runes} ({char.runes_memory})"),
            (self._vigor_value_w, char.vigor),
            (self._mind_value_w, char.mind),
            (self._endurance_value_w, char.endurance),
            (self._strength_value_w, char.strength),
            (self._dexterity_value_w, char.dexterity),
            (self._intelligence_value_w, char.intelligence),
            (self._faith_value_w, char.faith),
            (self._arcane_value_w, char.arcane),

            (self._hp_value_w, f"{char.hp_current}/{char.hp_max}"),
            (self._fp_value_w, f"{char.fp_current}/{char.fp_max}"),
            (self._stamina_value_w, f"{char.stamina_current}/{char.stamina_max}"),
        ):
            widget.setText(str(value))

    def _set_empty(self):
        self._name_label.setText("< Empty >")
        for value_w in (
            self._level_value_w,
            self._runes_value_w,
            self._vigor_value_w,
            self._mind_value_w,
            self._endurance_value_w,
            self._strength_value_w,
            self._dexterity_value_w,
            self._intelligence_value_w,
            self._faith_value_w,
            self._arcane_value_w,

            self._hp_value_w,
            self._fp_value_w,
            self._stamina_value_w,
        ):
            value_w.setText("")


class CharacterInfoWidget(QtWidgets.QWidget):
    def __init__(self, parent):
        super().__init__(parent)

        self.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)
        status_widget = CharacterStatusWidget(self)

        main_layout = QtWidgets.QHBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(status_widget, 1)

        self._status_widget = status_widget

    def set_char(self, char):
        self._status_widget.set_char(char)
