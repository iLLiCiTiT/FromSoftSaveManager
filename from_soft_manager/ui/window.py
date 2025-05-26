import typing
from PySide6 import QtWidgets

if typing.TYPE_CHECKING:
    from from_soft_manager.ui.control import Controller

from .style import load_stylesheet
from .dsr_widget import DSRWidget


class MainWindow(QtWidgets.QDialog):
    def __init__(self, controller: "Controller"):
        super().__init__()
        self.setWindowTitle("FromSoftEditor")

        dsr_widget = DSRWidget(controller, self)

        main_layout = QtWidgets.QVBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(dsr_widget)

        self._dsr_widget = dsr_widget

        self.resize(800, 400)

    def showEvent(self, event):
        super().showEvent(event)
        self.setStyleSheet(load_stylesheet())
