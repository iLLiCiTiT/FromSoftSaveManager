from typing import Optional

from PySide6 import QtWidgets, QtCore, QtGui

from from_soft_manager.parse import (
    DSR_COVENANTS,
    DSRCharacter,
    DSR_CLASSES,
    DSR_PHYSIQUE,
    DSR_GIFTS,
)
from from_soft_manager.ui.utils import PixmapLabel
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

        covenant_label = QtWidgets.QLabel("Covenant", attributes_widget)
        covenant_value_w = QtWidgets.QLabel(attributes_widget)

        level_icon_w = StatusIconLabel("attr_level", attributes_widget)
        level_label = QtWidgets.QLabel("Level", attributes_widget)
        level_value_w = QtWidgets.QLabel(attributes_widget)

        souls_icon_w = StatusIconLabel("attr_souls", attributes_widget)
        souls_label = QtWidgets.QLabel("Souls", attributes_widget)
        souls_value_w = QtWidgets.QLabel(attributes_widget)

        vitality_icon_w = StatusIconLabel("attr_vitality", attributes_widget)
        vitality_label = QtWidgets.QLabel("Vitality", attributes_widget)
        vitality_value_w = QtWidgets.QLabel(attributes_widget)

        attunement_icon_w = StatusIconLabel("attr_attunement", attributes_widget)
        attunement_label = QtWidgets.QLabel("Attunement", attributes_widget)
        attunement_value_w = QtWidgets.QLabel(attributes_widget)

        endurance_icon_w = StatusIconLabel("attr_endurance", attributes_widget)
        endurance_label = QtWidgets.QLabel("Endurance", attributes_widget)
        endurance_value_w = QtWidgets.QLabel(attributes_widget)

        strength_icon_w = StatusIconLabel("attr_strength", attributes_widget)
        strength_label = QtWidgets.QLabel("Strength", attributes_widget)
        strength_value_w = QtWidgets.QLabel(attributes_widget)

        dexterity_icon_w = StatusIconLabel("attr_dexterity", attributes_widget)
        dexterity_label = QtWidgets.QLabel("Dexterity", attributes_widget)
        dexterity_value_w = QtWidgets.QLabel(attributes_widget)

        resistance_icon_w = StatusIconLabel("attr_resistance", attributes_widget)
        resistance_label = QtWidgets.QLabel("Resistance", attributes_widget)
        resistance_value_w = QtWidgets.QLabel(attributes_widget)

        intelligence_icon_w = StatusIconLabel("attr_intelligence", attributes_widget)
        intelligence_label = QtWidgets.QLabel("Intelligence", attributes_widget)
        intelligence_value_w = QtWidgets.QLabel(attributes_widget)

        faith_icon_w = StatusIconLabel("attr_faith", attributes_widget)
        faith_label = QtWidgets.QLabel("Faith", attributes_widget)
        faith_value_w = QtWidgets.QLabel(attributes_widget)

        humanity_icon_w = StatusIconLabel("attr_humanity", attributes_widget)
        humanity_label = QtWidgets.QLabel("Humanity", attributes_widget)
        humanity_value_w = QtWidgets.QLabel(attributes_widget)

        attributes_layout = QtWidgets.QGridLayout(attributes_widget)
        attributes_layout.setContentsMargins(0, 0, 0, 0)
        attributes_layout.addWidget(name_value_w, 0, 0, 1, 3)

        for icon_w, label_w, value_w in (
            (None, covenant_label, covenant_value_w),
            (level_icon_w, level_label, level_value_w),
            (souls_icon_w, souls_label, souls_value_w),
            (vitality_icon_w, vitality_label, vitality_value_w),
            (attunement_icon_w, attunement_label, attunement_value_w),
            (endurance_icon_w, endurance_label, endurance_value_w),
            (strength_icon_w, strength_label, strength_value_w),
            (dexterity_icon_w, dexterity_label, dexterity_value_w),
            (resistance_icon_w, resistance_label, resistance_value_w),
            (intelligence_icon_w, intelligence_label, intelligence_value_w),
            (faith_icon_w, faith_label, faith_value_w),
            (humanity_icon_w, humanity_label, humanity_value_w),
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

        attributes_layout.setRowStretch(attributes_layout.rowCount(), 1)
        attributes_layout.setColumnStretch(0, 0)
        attributes_layout.setColumnStretch(1, 1)
        attributes_layout.setColumnStretch(2, 1)

        stats_widget = QtWidgets.QWidget(self)

        hollow_label = QtWidgets.QLabel("State", stats_widget)
        hollow_value_w = QtWidgets.QLabel(stats_widget)

        sex_label = QtWidgets.QLabel("Sex", stats_widget)
        sex_value_w = QtWidgets.QLabel(stats_widget)

        class_label = QtWidgets.QLabel("Class", stats_widget)
        class_value_w = QtWidgets.QLabel(stats_widget)

        physique_label = QtWidgets.QLabel("Physique", stats_widget)
        physique_value_w = QtWidgets.QLabel(stats_widget)

        gift_label = QtWidgets.QLabel("Gift", stats_widget)
        gift_value_w = QtWidgets.QLabel(stats_widget)

        hp_icon_w = StatusIconLabel("hp", attributes_widget)
        hp_label = QtWidgets.QLabel("HP", stats_widget)
        hp_value_w = QtWidgets.QLabel(stats_widget)

        stamina_icon_w = StatusIconLabel("stamina", attributes_widget)
        stamina_label = QtWidgets.QLabel("Stamina", stats_widget)
        stamina_value_w = QtWidgets.QLabel(stats_widget)

        bleed_res_icon_w = StatusIconLabel("res_bleed", attributes_widget)
        bleed_res_label = QtWidgets.QLabel("Bleed Resistance", stats_widget)
        bleed_res_value_w = QtWidgets.QLabel(stats_widget)

        poison_res_icon_w = StatusIconLabel("res_poison", attributes_widget)
        poison_res_label = QtWidgets.QLabel("Poison Resistance", stats_widget)
        poison_res_value_w = QtWidgets.QLabel(stats_widget)

        curse_res_icon_w = StatusIconLabel("res_curse", attributes_widget)
        curse_res_label = QtWidgets.QLabel("Curse Resistance", stats_widget)
        curse_res_value_w = QtWidgets.QLabel(stats_widget)

        stats_layout = QtWidgets.QGridLayout(stats_widget)
        stats_layout.setContentsMargins(0, 0, 0, 0)

        for icon_w, label_w, value_w in (
            (None, hollow_label, hollow_value_w),
            (None, sex_label, sex_value_w),
            (None, class_label, class_value_w),
            (None, physique_label, physique_value_w),
            (None, gift_label, gift_value_w),
            (hp_icon_w, hp_label, hp_value_w),
            (stamina_icon_w, stamina_label, stamina_value_w),
            (bleed_res_icon_w, bleed_res_label, bleed_res_value_w),
            (poison_res_icon_w, poison_res_label, poison_res_value_w),
            (curse_res_icon_w, curse_res_label, curse_res_value_w),
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

        stats_layout.setRowStretch(stats_layout.rowCount(), 1)
        stats_layout.setColumnStretch(0, 0)
        stats_layout.setColumnStretch(1, 1)
        stats_layout.setColumnStretch(2, 1)

        main_layout = QtWidgets.QHBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(attributes_widget, 1)
        main_layout.addWidget(stats_widget, 1)

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

        self._hollow_value_w = hollow_value_w
        self._sex_value_w = sex_value_w
        self._hp_value_w = hp_value_w
        self._stamina_value_w = stamina_value_w
        self._bleed_res_value_w = bleed_res_value_w
        self._poison_res_value_w = poison_res_value_w
        self._curse_res_value_w = curse_res_value_w
        self._class_value_w = class_value_w
        self._physique_value_w = physique_value_w
        self._gift_value_w = gift_value_w

        self.set_char(None)

    def set_char(self, char: Optional[DSRCharacter]):
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

            (self._hollow_value_w, "Hollow" if char.hollow_state == 8 else "Human"),
            (self._sex_value_w, "Male" if char.sex == 1 else "Female"),
            (self._hp_value_w, f"{char.hp_current}/{char.hp_max}"),
            (self._stamina_value_w, f"{char.stamina_current}/{char.stamina_max}"),
            (self._bleed_res_value_w, char.bleed_res),
            (self._poison_res_value_w, char.poison_res),
            (self._curse_res_value_w, char.curse_res),
            (self._class_value_w, DSR_CLASSES[char.class_id]),
            (self._physique_value_w, DSR_PHYSIQUE[char.physique_id]),
            (self._gift_value_w, DSR_GIFTS[char.gift_id]),
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

            self._hollow_value_w,
            self._sex_value_w,
            self._hp_value_w,
            self._stamina_value_w,
            self._bleed_res_value_w,
            self._poison_res_value_w,
            self._curse_res_value_w,
            self._class_value_w,
            self._physique_value_w,
            self._gift_value_w,
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
