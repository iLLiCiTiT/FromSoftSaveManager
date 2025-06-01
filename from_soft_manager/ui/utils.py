import uuid

from PySide6 import QtWidgets, QtCore, QtGui


class TabButton(QtWidgets.QPushButton):
    requested = QtCore.Signal(str)

    def __init__(self, tab_id, tab_label, parent):
        super().__init__(tab_label, parent)
        self.clicked.connect(self._on_click)
        self.setDefault(False)
        self.setAutoDefault(False)

        self.tab_id = tab_id

        self._selected = False

    def set_selected_tab(self, selected):
        if self._selected == selected:
            return
        self._selected = selected
        value = "1" if selected else ""
        self.setProperty("selected", value)
        self.style().polish(self)

    def _on_click(self):
        self.requested.emit(self.tab_id)


class TabWidget(QtWidgets.QWidget):
    def __init__(self, parent):
        super().__init__(parent)

        bar_widget = QtWidgets.QWidget(self)
        bar_layout = QtWidgets.QHBoxLayout(bar_widget)
        bar_layout.setContentsMargins(0, 0, 0, 0)
        bar_layout.setSpacing(0)
        bar_layout.addStretch(1)

        empty_content_widget = QtWidgets.QWidget(bar_widget)

        content_widget = QtWidgets.QWidget(self)
        content_layout = QtWidgets.QVBoxLayout(content_widget)
        content_layout.setContentsMargins(0, 0, 0, 0)
        content_layout.addWidget(empty_content_widget, 1)

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(bar_widget, 0)
        layout.addWidget(content_widget, 1)

        self._empty_content_widget = empty_content_widget
        self._bar_widget = bar_widget
        self._bar_layout = bar_layout
        self._content_widget = content_widget
        self._content_layout = content_layout

        self._current_tab = None
        self._tab_btns = {}
        self._tab_widgets = {}

    def add_tab(
        self,
        tab_label: str,
        tab_widget: QtWidgets.QWidget,
        tab_id: str | None = None,
    ):
        if tab_id is None:
            tab_id = uuid.uuid1().hex
        tab_button = TabButton(tab_id, tab_label, self._bar_widget)

        tab_button.requested.connect(self._on_tab_request)

        tab_widget.setVisible(False)

        pos = self._bar_layout.count() - 1
        self._bar_layout.insertWidget(pos, tab_button, 0)

        self._tab_btns[tab_id] = tab_button
        self._tab_widgets[tab_id] = tab_widget

        if self._current_tab is None:
            self._set_current_tab(tab_id)

    def _set_current_tab(self, tab_id):
        if self._current_tab == tab_id:
            return

        current_tab = self._tab_btns.get(self._current_tab)
        if current_tab is not None:
            current_tab.set_selected_tab(False)

        self._current_tab = tab_id

        tab_content = self._empty_content_widget
        if tab_id is not None:
            tab_content = self._tab_widgets[tab_id]
            tab_button = self._tab_btns[tab_id]
            tab_button.set_selected_tab(True)

        item = self._content_layout.takeAt(0)
        item.widget().setVisible(False)
        tab_content.setVisible(True)
        self._content_layout.addWidget(tab_content, 1)

    def _on_tab_request(self, tab_id):
        self._set_current_tab(tab_id)


class PixmapLabel(QtWidgets.QLabel):
    """Label resizing image to height of font."""
    def __init__(self, pixmap, parent):
        super().__init__(parent)
        self._empty_pixmap = QtGui.QPixmap(0, 0)
        self._source_pixmap = pixmap

        self._last_width = 0
        self._last_height = 0

    def set_source_pixmap(self, pixmap):
        """Change source image."""
        self._source_pixmap = pixmap
        self._set_resized_pix()

    def _get_pix_size(self):
        size = self.fontMetrics().height()
        size += size % 2
        return size, size

    def minimumSizeHint(self):
        width, height = self._get_pix_size()
        if width != self._last_width or height != self._last_height:
            self._set_resized_pix()
        return QtCore.QSize(width, height)

    def _set_resized_pix(self):
        if self._source_pixmap is None:
            self.setPixmap(self._empty_pixmap)
            return
        width, height = self._get_pix_size()
        self.setPixmap(
            self._source_pixmap.scaled(
                width,
                height,
                QtCore.Qt.KeepAspectRatio,
                QtCore.Qt.SmoothTransformation
            )
        )
        self._last_width = width
        self._last_height = height

    def resizeEvent(self, event):
        self._set_resized_pix()
        super().resizeEvent(event)


class CreateBackupDialog(QtWidgets.QDialog):
    def __init__(self, parent):
        super().__init__(parent)

        self.setWindowTitle("Create save backup")
        self.setModal(True)

        label_input = QtWidgets.QLineEdit(self)
        label_input.setPlaceholderText("Custom label..")
        label_input.setToolTip(
            "Label for the backup, leave empty to use default"
        )

        btns_widget = QtWidgets.QWidget(self)

        create_btn = QtWidgets.QPushButton("Create", self)
        create_btn.setDefault(True)
        cancel_btn = QtWidgets.QPushButton("Cancel", self)

        btns_layout = QtWidgets.QHBoxLayout(btns_widget)
        btns_layout.setContentsMargins(0, 0, 0, 0)

        btns_layout.addStretch(1)
        btns_layout.addWidget(create_btn, 0)
        btns_layout.addWidget(cancel_btn, 0)

        create_btn.clicked.connect(self.accept)
        cancel_btn.clicked.connect(self.reject)

        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(label_input, 0)
        layout.addStretch(1)
        layout.addWidget(btns_widget, 0)

        self._label_input = label_input

    def get_label_input(self):
        return self._label_input.text()


class ManageSavesWidget(QtWidgets.QFrame):
    def __init__(self, controller, parent):
        super().__init__(parent)

        quicksave_label = QtWidgets.QLabel(self)
        quicksave_label.setAlignment(QtCore.Qt.AlignCenter)

        btns_widget = QtWidgets.QWidget(self)

        # TODO use icon instead
        create_backup_btn = QtWidgets.QPushButton(
            "Create Backup", btns_widget
        )
        create_backup_btn.setToolTip("Create Backup")

        btns_layout = QtWidgets.QHBoxLayout(btns_widget)
        btns_layout.setContentsMargins(0, 0, 0, 0)
        btns_layout.addStretch(1)
        btns_layout.addWidget(create_backup_btn, 0)
        btns_layout.addStretch(1)

        main_layout = QtWidgets.QVBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(quicksave_label, 0)
        main_layout.addWidget(btns_widget, 0)
        main_layout.addStretch(1)

        create_backup_btn.clicked.connect(self._on_create_backup)
        controller.hotkeys_changed.connect(self._update_hotkeys)

        self._quicksave_label = quicksave_label

        self._controller = controller
        self._update_hotkeys()

    def _on_create_backup(self):
        dialog = CreateBackupDialog(self)
        if dialog.exec_() == QtWidgets.QDialog.Accepted:
            label = dialog.get_label_input()
            self._controller.create_manual_backup(label)

    def _update_hotkeys(self):
        info = self._controller.get_config_info()
        quicksave_hotkey = info.quicksave_hotkey
        quickload_hotkey = info.quickload_hotkey

        parts = []
        if quicksave_hotkey is not None:
            keys = QtGui.QKeySequence(quicksave_hotkey).toString()
            parts.append(f"{keys} to quicksave")

        if quickload_hotkey is not None:
            keys = QtGui.QKeySequence(quickload_hotkey).toString()
            parts.append(f"{keys} to quickload")

        if not parts:
            self._quicksave_label.setVisible(False)
            return

        self._quicksave_label.setVisible(True)
        msg = " and ".join(parts)
        self._quicksave_label.setText(f"<i>Use {msg}</i>")
