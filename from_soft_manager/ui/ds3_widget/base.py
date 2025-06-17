from PySide6 import QtWidgets, QtCore, QtGui

from from_soft_manager.ui.utils import ManageSavesWidget

from .resources import get_resource


class DS3Widget(QtWidgets.QWidget):
    _bg_pix = None

    def __init__(self, controller, save_id, parent):
        super().__init__(parent)

        manage_saves_widget = ManageSavesWidget(controller, self)

        info_label = QtWidgets.QLabel(
            "Dark Souls 3 viewer is not implemented yet", self
        )
        info_label.setAttribute(
            QtCore.Qt.WA_TranslucentBackground, True
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

    def paintEvent(self, event):
        painter = QtGui.QPainter(self)
        painter.setClipRect(event.rect())
        painter.setPen(QtCore.Qt.NoPen)
        painter.setBrush(QtGui.QColor("#060507"))
        rect = self.rect()
        painter.drawRect(rect)
        pix = self._get_bg_pix()
        painter.drawPixmap(rect, pix.scaled(
            rect.size(),
            QtCore.Qt.KeepAspectRatioByExpanding,
            QtCore.Qt.SmoothTransformation
        ))

    @classmethod
    def _get_bg_pix(cls):
        if cls._bg_pix is None:
            cls._bg_pix = QtGui.QPixmap(
                get_resource("bg.png")
            )
        return cls._bg_pix
