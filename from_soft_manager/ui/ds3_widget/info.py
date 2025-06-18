from PySide6 import QtWidgets, QtCore, QtGui

from from_soft_manager.parse import DS3Character
from from_soft_manager.ui.utils import PixmapLabel

from .resources import get_status_icon_path


class StatusIconLabel(PixmapLabel):
    def __init__(self, icon_name: str, parent):
        icon = QtGui.QPixmap(get_status_icon_path(icon_name))
        super().__init__(icon, parent)


class CharacterStatusWidget(QtWidgets.QWidget):
    def __init__(self, parent):
        super().__init__(parent)

        attributes_widget = QtWidgets.QWidget(self)

        name_value_w = QtWidgets.QLabel(attributes_widget)
        name_value_w.setAlignment(QtCore.Qt.AlignCenter)

        level_icon_w = StatusIconLabel("attr_level", attributes_widget)
        level_label = QtWidgets.QLabel("Level", attributes_widget)
        level_value_w = QtWidgets.QLabel(attributes_widget)

        souls_icon_w = StatusIconLabel("attr_souls", attributes_widget)
        souls_label = QtWidgets.QLabel("Souls", attributes_widget)
        souls_value_w = QtWidgets.QLabel(attributes_widget)

        vigor_icon_w = StatusIconLabel("attr_vigor", attributes_widget)
        vigor_label = QtWidgets.QLabel("Vigor", attributes_widget)
        vigor_value_w = QtWidgets.QLabel(attributes_widget)

        attunement_icon_w = StatusIconLabel("attr_attunement", attributes_widget)
        attunement_label = QtWidgets.QLabel("Attunement", attributes_widget)
        attunement_value_w = QtWidgets.QLabel(attributes_widget)

        endurance_icon_w = StatusIconLabel("attr_endurance", attributes_widget)
        endurance_label = QtWidgets.QLabel("Endurance", attributes_widget)
        endurance_value_w = QtWidgets.QLabel(attributes_widget)

        vitality_icon_w = StatusIconLabel("attr_vitality", attributes_widget)
        vitality_label = QtWidgets.QLabel("Vitality", attributes_widget)
        vitality_value_w = QtWidgets.QLabel(attributes_widget)

        strength_icon_w = StatusIconLabel("attr_strength", attributes_widget)
        strength_label = QtWidgets.QLabel("Strength", attributes_widget)
        strength_value_w = QtWidgets.QLabel(attributes_widget)

        dexterity_icon_w = StatusIconLabel("attr_dexterity", attributes_widget)
        dexterity_label = QtWidgets.QLabel("Dexterity", attributes_widget)
        dexterity_value_w = QtWidgets.QLabel(attributes_widget)

        intelligence_icon_w = StatusIconLabel("attr_intelligence", attributes_widget)
        intelligence_label = QtWidgets.QLabel("Intelligence", attributes_widget)
        intelligence_value_w = QtWidgets.QLabel(attributes_widget)

        faith_icon_w = StatusIconLabel("attr_faith", attributes_widget)
        faith_label = QtWidgets.QLabel("Faith", attributes_widget)
        faith_value_w = QtWidgets.QLabel(attributes_widget)

        luck_icon_w = StatusIconLabel("attr_luck", attributes_widget)
        luck_label = QtWidgets.QLabel("Luck", attributes_widget)
        luck_value_w = QtWidgets.QLabel(attributes_widget)

        attributes_layout = QtWidgets.QGridLayout(attributes_widget)
        attributes_layout.setContentsMargins(0, 0, 0, 0)
        attributes_layout.addWidget(name_value_w, 0, 0, 1, 3)

        for icon_w, label_w, value_w in (
            (level_icon_w, level_label, level_value_w),
            (souls_icon_w, souls_label, souls_value_w),
            (vigor_icon_w, vigor_label, vigor_value_w),
            (attunement_icon_w, attunement_label, attunement_value_w),
            (endurance_icon_w, endurance_label, endurance_value_w),
            (vitality_icon_w, vitality_label, vitality_value_w),
            (strength_icon_w, strength_label, strength_value_w),
            (dexterity_icon_w, dexterity_label, dexterity_value_w),
            (intelligence_icon_w, intelligence_label, intelligence_value_w),
            (faith_icon_w, faith_label, faith_value_w),
            (luck_icon_w, luck_label, luck_value_w),
        ):
            row = attributes_layout.rowCount()
            label_w.setAlignment(QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter)
            value_w.setAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
            if icon_w is not None:
                attributes_layout.addWidget(icon_w, row, 0)
                attributes_layout.addWidget(label_w, row, 1)
            else:
                attributes_layout.addWidget(label_w, row, 0, 1, 2)
            attributes_layout.addWidget(value_w, row, 2)
            for widget in (icon_w, label_w, value_w):
                if widget is None:
                    continue
                widget.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)

        attributes_layout.setRowStretch(attributes_layout.rowCount(), 1)
        attributes_layout.setColumnStretch(0, 0)
        attributes_layout.setColumnStretch(1, 1)
        attributes_layout.setColumnStretch(2, 1)

        stats_widget = QtWidgets.QWidget(self)

        hp_icon_w = StatusIconLabel("attr_hp", stats_widget)
        hp_label = QtWidgets.QLabel("HP", stats_widget)
        hp_value_w = QtWidgets.QLabel(stats_widget)

        fp_icon_w = StatusIconLabel("attr_fp", stats_widget)
        fp_label = QtWidgets.QLabel("FP", stats_widget)
        fp_value_w = QtWidgets.QLabel(stats_widget)

        stamina_icon_w = StatusIconLabel("attr_stamina", stats_widget)
        stamina_label = QtWidgets.QLabel("Stamina", stats_widget)
        stamina_value_w = QtWidgets.QLabel(stats_widget)

        stats_layout = QtWidgets.QGridLayout(stats_widget)
        stats_layout.setContentsMargins(0, 0, 0, 0)

        for icon_w, label_w, value_w in (
            (hp_icon_w, hp_label, hp_value_w),
            (fp_icon_w, fp_label, fp_value_w),
            (stamina_icon_w, stamina_label, stamina_value_w),
        ):
            row = stats_layout.rowCount()
            label_w.setAlignment(QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter)
            value_w.setAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
            if icon_w is not None:
                stats_layout.addWidget(icon_w, row, 0)
                stats_layout.addWidget(label_w, row, 1)
            else:
                stats_layout.addWidget(label_w, row, 0, 1, 2)
            stats_layout.addWidget(value_w, row, 2)
            for widget in (icon_w, label_w, value_w):
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
            self, attributes_widget, name_value_w, stats_widget,
        ):
            widget.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)

        self._char = None
        self._name_label = name_value_w
        self._level_value_w = level_value_w
        self._souls_value_w= souls_value_w
        self._vigor_value_w = vigor_value_w
        self._attunement_value_w = attunement_value_w
        self._endurance_value_w = endurance_value_w
        self._vitality_value_w = vitality_value_w
        self._strength_value_w = strength_value_w
        self._dexterity_value_w = dexterity_value_w
        self._intelligence_value_w = intelligence_value_w
        self._faith_value_w = faith_value_w
        self._luck_value_w = luck_value_w

        self._hp_value_w = hp_value_w
        self._fp_value_w = fp_value_w
        self._stamina_value_w = stamina_value_w

        self.set_char(None)

    def set_char(self, char: DS3Character | None):
        if char is None:
            self._set_empty()
            return

        self._name_label.setText(char.name)
        for widget, value in (
            (self._level_value_w, char.level),
            (self._souls_value_w, char.souls),
            (self._vigor_value_w, char.vigor),
            (self._attunement_value_w, char.attunement),
            (self._endurance_value_w, char.endurance),
            (self._vitality_value_w, char.vitality),
            (self._strength_value_w, char.strength),
            (self._dexterity_value_w, char.dexterity),
            (self._intelligence_value_w, char.intelligence),
            (self._faith_value_w, char.faith),
            (self._luck_value_w, char.luck),

            (self._hp_value_w, f"{char.hp_current}/{char.hp_max}"),
            (self._fp_value_w, f"{char.fp_current}/{char.fp_max}"),
            (self._stamina_value_w, f"{char.stamina_current}/{char.stamina_max}"),
        ):
            widget.setText(str(value))

    def _set_empty(self):
        self._name_label.setText("< Empty >")
        for value_w in (
            self._level_value_w,
            self._souls_value_w,
            self._vigor_value_w,
            self._attunement_value_w,
            self._endurance_value_w,
            self._vitality_value_w,
            self._strength_value_w,
            self._dexterity_value_w,
            self._intelligence_value_w,
            self._faith_value_w,
            self._luck_value_w,

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