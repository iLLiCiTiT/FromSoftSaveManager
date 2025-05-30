from PySide6 import QtWidgets, QtCore

from from_soft_manager.ui.utils import ManageSavesWidget


class ERWidget(QtWidgets.QWidget):
    def __init__(self, controller, save_id, parent):
        super().__init__(parent)

        left_side = QtWidgets.QWidget(self)

        manage_saves_widget = ManageSavesWidget(controller, left_side)

        left_side_layout = QtWidgets.QVBoxLayout(left_side)
        left_side_layout.setContentsMargins(0, 0, 0, 0)
        left_side_layout.addStretch(4)
        left_side_layout.addWidget(manage_saves_widget, 1)

        info_label = QtWidgets.QLabel(
            "Elden Ring viewer is not implemented yet", self
        )
        info_label.setAlignment(QtCore.Qt.AlignCenter)

        main_layout = QtWidgets.QHBoxLayout(self)
        main_layout.setContentsMargins(24, 24, 24, 24)
        main_layout.setSpacing(0)
        main_layout.addWidget(left_side, 1)
        main_layout.addWidget(info_label, 2)
        main_layout.addStretch(1)

        self.save_id = save_id
        self._controller = controller
