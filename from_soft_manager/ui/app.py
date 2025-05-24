from PySide6 import QtCore, QtWidgets

from from_soft_manager import __version__
from .control import Controller
from .window import MainWindow


def main():
    app = QtWidgets.QApplication()
    app.setApplicationName("FromSoftEditor")
    app.setApplicationVersion(__version__)
    # TODO add icon
    # app.setWindowIcon(QtGui.QIcon("logo.ico"))
    app.setAttribute(QtCore.Qt.AA_EnableHighDpiScaling)

    controller = Controller()

    window = MainWindow(controller)
    window.show()

    app.exec_()
