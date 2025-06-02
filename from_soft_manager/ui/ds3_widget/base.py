from PySide6 import QtWidgets, QtCore

from from_soft_manager.ui.utils import ManageSavesWidget


class DS3Widget(QtWidgets.QWidget):
    def __init__(self, controller, save_id, parent):
        super().__init__(parent)

        manage_saves_widget = ManageSavesWidget(controller, self)

        info_label = QtWidgets.QLabel(
            "Dark Souls 3 viewer is not implemented yet", self
        )
        info_label.setAlignment(QtCore.Qt.AlignCenter)

        main_layout = QtWidgets.QVBoxLayout(self)
        main_layout.setContentsMargins(24, 24, 24, 24)
        main_layout.setSpacing(0)
        main_layout.addWidget(info_label, 3)
        main_layout.addWidget(manage_saves_widget, 1)
        main_layout.addStretch(1)

        self.save_id = save_id
        self._controller = controller

    def refresh(self):
        pass
