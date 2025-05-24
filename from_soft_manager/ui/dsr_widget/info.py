from PySide6 import QtWidgets, QtCore

from from_soft_manager.parse import DSR_COVENANTS
# (
#     cov_none_lvl,
#     cov_way_of_light_lvl,
#     cov_princesss_guard_lvl,
#     cov_warrior_of_sunlight_lvl,
#     cov_darkwraith_lvl,
#     cov_path_of_the_dragon_lvl,
#     cov_gravelord_servant_lvl,
#     cov_forest_hunter_lvl,
#     cov_darkmoon_blade_lvl,
#     cov_chaos_servant_lvl,
# ) = covenant_levels


class CharacterStatusWidget(QtWidgets.QWidget):
    def __init__(self, parent):
        super().__init__(parent)

        name_value_w = QtWidgets.QLabel(self)
        name_value_w.setAlignment(QtCore.Qt.AlignCenter)

        covenant_label = QtWidgets.QLabel("Covenant", self)
        covenant_value_w = QtWidgets.QLabel(self)

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

        main_layout.setRowStretch(main_layout.rowCount(), 1)

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
        covenant_name = DSR_COVENANTS[char.covenant_id]
        covenant_level = char.covenant_levels[char.covenant_id]
        if covenant_level >= 30:
            covenant_name += "+2"
        elif covenant_level >= 10:
            covenant_name += "+1"

        self._covenant_value_w.setText(covenant_name)

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
