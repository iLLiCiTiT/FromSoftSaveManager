import uuid

from PySide6 import QtWidgets, QtCore, QtGui

from from_soft_manager.ui.structures import BackupType, BackupInfo

BACKUP_ID_ROLE = QtCore.Qt.UserRole + 1
BACKUP_TYPE_ROLE = QtCore.Qt.UserRole + 2
BACKUP_DATE_ROLE = QtCore.Qt.UserRole + 3


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

        for widget in (self, bar_widget, empty_content_widget, content_widget):
            widget.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)

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
        self._aspect_ratio = pixmap.width() / pixmap.height()

        self._last_width = 0
        self._last_height = 0

    def set_source_pixmap(self, pixmap):
        """Change source image."""
        self._source_pixmap = pixmap
        self._aspect_ratio = pixmap.width() / pixmap.height()
        self._set_resized_pix()

    def _get_pix_size(self):
        height = self.fontMetrics().height()
        height += height % 2
        if self._aspect_ratio > 1.0:
            width = height
            height /= self._aspect_ratio
        else:
            width = height * self._aspect_ratio

        return int(width), int(height)

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


class FocusSpinBox(QtWidgets.QSpinBox):
    """QSpinBox which allow scroll wheel changes only in active state."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)

    def wheelEvent(self, event):
        if self.hasFocus():
            super().wheelEvent(event)
        else:
            event.ignore()


class SquareButton(QtWidgets.QPushButton):
    """Make button square shape.

    Change width to match height on resize.
    """

    def __init__(self, *args, **kwargs):
        super(SquareButton, self).__init__(*args, **kwargs)

        sp = self.sizePolicy()
        sp.setVerticalPolicy(QtWidgets.QSizePolicy.Minimum)
        sp.setHorizontalPolicy(QtWidgets.QSizePolicy.Minimum)
        self.setSizePolicy(sp)
        self._ideal_width = None

    def showEvent(self, event):
        super(SquareButton, self).showEvent(event)
        self._ideal_width = self.height()
        self.updateGeometry()

    def resizeEvent(self, event):
        super(SquareButton, self).resizeEvent(event)
        self._ideal_width = self.height()
        self.updateGeometry()

    def sizeHint(self):
        sh = super(SquareButton, self).sizeHint()
        ideal_width = self._ideal_width
        if ideal_width is None:
            ideal_width = sh.height()
        sh.setWidth(ideal_width)
        return sh


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


class BaseClickableFrame(QtWidgets.QFrame):
    def __init__(self, parent):
        super().__init__(parent)

        self._mouse_pressed = False

    def _mouse_release_callback(self):
        pass

    def mousePressEvent(self, event):
        super().mousePressEvent(event)
        if event.isAccepted():
            return
        if event.button() == QtCore.Qt.LeftButton:
            self._mouse_pressed = True
            event.accept()

    def mouseReleaseEvent(self, event):
        pressed, self._mouse_pressed = self._mouse_pressed, False
        super().mouseReleaseEvent(event)
        if event.isAccepted():
            return

        accepted = pressed and self.rect().contains(event.pos())
        if accepted:
            event.accept()
            self._mouse_release_callback()


class BackupsModel(QtGui.QStandardItemModel):
    def __init__(self, parent):
        super().__init__(parent)

        self.setColumnCount(2)
        self.setHorizontalHeaderLabels(["Title", "Date"])

    def set_backups(self, backups: list[BackupInfo]):
        root_item = self.invisibleRootItem()
        root_item.removeRows(0, root_item.rowCount())

        backups.sort(key=lambda b: b.datetime, reverse=True)
        quicksave_count = 0
        autosave_count = 0
        new_items = []
        for backup in backups:
            label = backup.label
            if not label:
                if backup.backup_type == BackupType.quicksave:
                    label = f"< Quick Save {quicksave_count} >"
                    quicksave_count += 1
                elif backup.backup_type == BackupType.autosave:
                    label = f"< Auto Save {autosave_count} >"
                    autosave_count += 1
                else:
                    label = backup.datetime
            item = QtGui.QStandardItem(label)
            item.setColumnCount(self.columnCount())
            item.setFlags(
                QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable
            )
            item.setData(backup.backup_id, BACKUP_ID_ROLE)
            item.setData(backup.backup_type, BACKUP_TYPE_ROLE)
            item.setData(backup.datetime.format("YY MM.DD. HH:mm:ss"), BACKUP_DATE_ROLE)
            new_items.append(item)

        if not new_items:
            item = QtGui.QStandardItem("No backups found")
            item.setFlags(QtCore.Qt.NoItemFlags)
            new_items.append(item)

        root_item.appendRows(new_items)

    def flags(self, index):
        if index.column() != 0:
            index = self.index(index.row(), 0, index.parent())
        return super().flags(index)

    def data(self, index, role=QtCore.Qt.DisplayRole):
        if not index.isValid():
            return None

        col = index.column()
        if col == 1 and role == QtCore.Qt.DisplayRole:
            role = BACKUP_DATE_ROLE

        if col != 0:
            index = self.index(index.row(), 0, index.parent())
        return super().data(index, role)


class BackupsListWidget(QtWidgets.QWidget):
    def __init__(self, controller, parent):
        super().__init__(parent)
        self.setWindowFlags(QtCore.Qt.Popup | QtCore.Qt.FramelessWindowHint)

        load_label = QtWidgets.QLabel("Backups", self)

        delete_btn = QtWidgets.QPushButton("Delete", self)

        backups_view = QtWidgets.QTreeView(self)
        backups_view.setSortingEnabled(True)
        backups_view.setIndentation(0)
        backups_view.setTextElideMode(QtCore.Qt.ElideLeft)
        backups_view.setEditTriggers(
            QtWidgets.QAbstractItemView.NoEditTriggers
        )
        backups_view.setSelectionMode(
            QtWidgets.QAbstractItemView.ExtendedSelection
        )
        backups_view.setAllColumnsShowFocus(True)
        backups_view.setVerticalScrollMode(
            QtWidgets.QAbstractItemView.ScrollPerPixel
        )
        backups_view.sortByColumn(1, QtCore.Qt.AscendingOrder)
        backups_view.setAlternatingRowColors(True)
        backups_view.setMinimumHeight(100)

        backups_view.stackUnder(delete_btn)

        model = BackupsModel(backups_view)
        backups_view.setModel(model)

        main_layout = QtWidgets.QVBoxLayout(self)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.addWidget(load_label, 0)
        main_layout.addWidget(backups_view, 1)

        delete_btn.clicked.connect(self._on_delete_click)
        backups_view.doubleClicked.connect(self._on_double_click)

        self.resize(300, 260)

        self._controller = controller
        self._backups_view = backups_view
        self._model = model
        self._delete_btn = delete_btn

    def refresh(self):
        backups = self._controller.get_backup_items()
        self._model.set_backups(backups)

    def showEvent(self, event):
        super().showEvent(event)
        self._backups_view.resizeColumnToContents(0)
        self._update_delete_btn_geo()

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self._update_delete_btn_geo()

    def _update_delete_btn_geo(self):
        geo = self.geometry()
        sh = self._delete_btn.sizeHint()
        pos_x = (geo.x() + geo.width()) - (1 + sh.width())
        pos_y = (geo.y() + geo.height()) - (1 + sh.height())

        self._delete_btn.setGeometry(pos_x, pos_y, sh.width(), sh.height())

    def _on_double_click(self, index):
        if not index.isValid():
            return

        backup_id = index.data(BACKUP_ID_ROLE)
        if not backup_id:
            return
        self._controller.restore_by_backup_id(backup_id)
        self.close()

    def _on_delete_click(self):
        sel_model = self._backups_view.selectionModel()
        save_ids = set()
        for index in sel_model.selectedIndexes():
            save_id = index.data(BACKUP_ID_ROLE)
            if save_id:
                save_ids.add(save_id)
        self._controller.delete_backups(save_ids)
        self.refresh()


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

        load_backup_btn = QtWidgets.QPushButton(
            "Load Backup", btns_widget
        )
        load_backup_btn.setToolTip("Load Backup")

        open_backup_dir_btn = QtWidgets.QPushButton(
            "Open Backup dir", btns_widget
        )
        open_backup_dir_btn.setToolTip("Open Backup directory")

        btns_layout = QtWidgets.QHBoxLayout(btns_widget)
        btns_layout.setContentsMargins(0, 0, 0, 0)
        btns_layout.addStretch(1)
        btns_layout.addWidget(create_backup_btn, 0)
        btns_layout.addWidget(load_backup_btn, 0)
        btns_layout.addWidget(open_backup_dir_btn, 0)
        btns_layout.addStretch(1)

        main_layout = QtWidgets.QVBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.addWidget(quicksave_label, 0)
        main_layout.addWidget(btns_widget, 0)
        main_layout.addStretch(1)

        backup_list_widget = BackupsListWidget(controller, self)
        backup_list_widget.setVisible(False)

        create_backup_btn.clicked.connect(self._on_create_backup)
        load_backup_btn.clicked.connect(self._on_load_backup)
        open_backup_dir_btn.clicked.connect(self._on_open_backup_dir)
        controller.hotkeys_changed.connect(self._update_hotkeys)

        for widget in (
            self,
            quicksave_label,
            btns_widget,
            create_backup_btn,
            load_backup_btn,
            open_backup_dir_btn,
        ):
            widget.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)

        self._quicksave_label = quicksave_label

        self._load_backup_btn = load_backup_btn

        self._backup_list_widget = backup_list_widget

        self._controller = controller
        self._update_hotkeys()

    def _on_create_backup(self):
        dialog = CreateBackupDialog(self)
        if dialog.exec_() == QtWidgets.QDialog.Accepted:
            label = dialog.get_label_input()
            self._controller.create_manual_backup(label)

    def _on_load_backup(self):
        self._backup_list_widget.refresh()
        self._backup_list_widget.show()

        global_pos = self.mapToGlobal(self._load_backup_btn.pos())
        self._backup_list_widget.move(global_pos)

    def _on_open_backup_dir(self):
        self._controller.open_backup_dir()

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
