import os

from PySide6 import QtCore, QtGui, QtWidgets

from from_soft_manager.parse.parse_ds3 import (
    DS3Character,
    ITEMS_BY_ID,
    InventoryItem,
)
from from_soft_manager.ui.utils import BaseClickableFrame, PixmapLabel

from .resources import get_resource

ITEM_LEVEL_ROLE = QtCore.Qt.UserRole + 1
ITEM_INFUSION_ICON_ROLE = QtCore.Qt.UserRole + 2
ITEM_ORDER_ROLE = QtCore.Qt.UserRole + 3
ITEM_AMOUNT_ROLE = QtCore.Qt.UserRole + 4
ITEM_STORAGE_BOX_AMOUNT_ROLE = QtCore.Qt.UserRole + 6
ITEM_IMAGE_ROLE = QtCore.Qt.UserRole + 7
ITEM_CATEGORY_ROLE = QtCore.Qt.UserRole + 8

CATEGORIES = [
    "tools",
    "materials",
    "key_items",
    "spells",
    "melee_weapons",
    "ranged_weapons",
    "catalysts",
    "shields",
    "head",
    "chest",
    "hands",
    "legs",
    "arrows_bolts",
    "rings",
    "covenant",
]
MERGABLE_CATEGORIES = {
    "tools",
    "materials",
    "spells",
    "arrows_bolts",
}


def get_item_pixmap(image_name: str) -> QtGui.QPixmap | None:
    path = get_resource("item_icons", f"{image_name}.png")
    if not os.path.exists(path):
        path = get_resource("test_data.png")
    return QtGui.QPixmap(path)


class DS3InventoryCategoryButton(BaseClickableFrame):
    clicked = QtCore.Signal(str)

    def __init__(self, category, parent):
        super().__init__(parent)

        self.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)

        image_path = get_resource(
            "menu_icons", f"inv_{category}.png"
        )

        bg_path_1 = get_resource(
            "menu_icons", f"inv_bg_1.png"
        )
        bg_path_2 = get_resource(
            "menu_icons", f"inv_bg_2.png"
        )
        pix = QtGui.QPixmap(image_path)
        bg_path_1_pix = QtGui.QPixmap(bg_path_1)
        bg_path_2_pix = QtGui.QPixmap(bg_path_2)

        self._category = category
        self._pix = pix
        self._bg_path_1_pix = bg_path_1_pix
        self._bg_path_2_pix = bg_path_2_pix
        self._hover_pix = None
        self._selected = False
        self._hovering = False

    def sizeHint(self):
        return self.minimumSizeHint()

    def minimumSizeHint(self):
        if self._selected:
            return QtCore.QSize(60, 60)
        return QtCore.QSize(30, 60)

    def set_selected(self, selected: bool):
        if self._selected == selected:
            return
        self._selected = selected
        self.updateGeometry()
        self.repaint()

    def enterEvent(self, event):
        super().enterEvent(event)
        self._hovering = True
        self.repaint()

    def leaveEvent(self, event):
        super().leaveEvent(event)
        self._hovering = False
        self.repaint()

    def paintEvent(self, event):
        rect = self.rect()
        painter = QtGui.QPainter(self)
        painter.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)
        painter.setBrush(QtCore.Qt.red)
        painter.setPen(QtCore.Qt.NoPen)
        painter.setClipRect(rect)
        bg_pix = None
        if self._selected:
            bg_pix = self._bg_path_1_pix
        elif self._hovering:
            bg_pix = self._bg_path_2_pix
        ar = float(rect.width()) / rect.height()
        if bg_pix is not None:
            bg_rect = bg_pix.rect()
            painter.drawPixmap(
                rect,
                bg_pix,
                bg_rect
            )

        pix_rect = self._pix.rect()
        if ar > float(pix_rect.width()) / pix_rect.height():
            width = int(float(pix_rect.height()) * ar)
            diff = pix_rect.width() - width
            pix_rect.setWidth(width)
            pix_rect.moveLeft(diff // 2)
        else:
            height = int(float(pix_rect.width()) / ar)
            diff = pix_rect.height() - height
            pix_rect.setHeight(height)
            pix_rect.moveTop(diff // 2)

        painter.drawPixmap(
            rect,
            self._pix,
            pix_rect
        )

    def _mouse_release_callback(self):
        self.clicked.emit(self._category)

    def _get_hover_pixmap(self):
        if self._hover_pix is not None:
            return self._hover_pix

        pix = QtGui.QPixmap(self._pix.width(), self._pix.height())
        pix.fill(QtCore.Qt.transparent)
        painter = QtGui.QPainter(pix)
        painter.setPen(QtCore.Qt.NoPen)
        painter.drawPixmap(0, 0, self._pix)
        painter.drawPixmap(0, 0, self._src_hover_pix)
        painter.end()

        self._hover_pix = pix
        return pix


class InventoryModel(QtGui.QStandardItemModel):
    def __init__(self, parent):
        super().__init__(parent)

    def _create_unknown_item(
        self, inventory_item: InventoryItem
    ) -> QtGui.QStandardItem:
        item_name = f"NA {inventory_item.item_id}"
        new_item = QtGui.QStandardItem(item_name)
        new_item.setData(inventory_item.level, ITEM_LEVEL_ROLE)
        new_item.setData(0, ITEM_STORAGE_BOX_AMOUNT_ROLE)
        new_item.setData(-1, ITEM_ORDER_ROLE)
        new_item.setData(inventory_item.amount, ITEM_AMOUNT_ROLE)
        new_item.setData(inventory_item.category, ITEM_CATEGORY_ROLE)
        new_item.setData(
            get_item_pixmap("unknown"),
            ITEM_IMAGE_ROLE
        )
        return new_item

    def set_char(self, char: DS3Character | None):
        root_item = self.invisibleRootItem()
        root_item.removeRows(0, root_item.rowCount())
        if char is None:
            return

        inventory_items = []
        items_by_category = {
            category: []
            for category in MERGABLE_CATEGORIES
        }
        for item in char.inventory_items + char.key_items:
            # Don't know what it is, but it is on every character
            if item.item_id == 1073741918:
                continue
            if item.category in items_by_category:
                items_by_category[item.category].append(item)
            else:
                inventory_items.append(item)

        blb_items = []
        blb_items_by_category = {
            category: []
            for category in MERGABLE_CATEGORIES
        }
        for item in char.storage_box_items:
            if item.category in blb_items_by_category:
                blb_items_by_category[item.category].append(item)
            else:
                blb_items.append(item)

        new_items = []
        for category in MERGABLE_CATEGORIES:
            inventory_item_by_id = {
                item.item_id: item
                for item in items_by_category[category]
            }
            blb_items_by_id = {
                item.item_id: item
                for item in  blb_items_by_category[category]
            }
            if not inventory_item_by_id and not blb_items_by_id:
                continue

            all_ids = set(inventory_item_by_id) | set(blb_items_by_id)
            for item_id in all_ids:
                inventory_item = inventory_item_by_id.get(item_id)
                blb_item = blb_items_by_id.get(item_id)
                if inventory_item is None:
                    model_item = self._create_model_item(blb_item, True)
                else:
                    model_item = self._create_model_item(inventory_item, False)
                    if blb_item is not None:
                        model_item.setData(
                            blb_item.amount, ITEM_STORAGE_BOX_AMOUNT_ROLE
                        )

                if model_item is not None:
                    new_items.append(model_item)

        for inventory_item in inventory_items:
            model_item = self._create_model_item(inventory_item, False)
            if model_item is not None:
                new_items.append(model_item)

        for inventory_item in blb_items:
            model_item = self._create_model_item(inventory_item, True)
            if model_item is not None:
                new_items.append(model_item)

        if new_items:
            root_item.appendRows(new_items)

    def _create_model_item(self, inventory_item, in_bottomless_box):
        item_id = inventory_item.item_id
        item = ITEMS_BY_ID.get(item_id)
        if item is None:
            return self._create_unknown_item(inventory_item)

        # # Skip fists
        # if item["type"] == 0 and item["id"] == 900000:
        #     return None
        #
        # # Skip "no armor"
        # if item["type"] == 268435456 and item["id"] in (
        #     900000, 901000, 902000, 903000
        # ):
        #     return None

        upgrade_level = inventory_item.level

        infusion_name = None
        if inventory_item.infusion == 0:
            pass
        elif inventory_item.infusion == 100:
            infusion_name = "heavy"
        elif inventory_item.infusion == 200:
            infusion_name = "sharp"
        elif inventory_item.infusion == 300:
            infusion_name = "refined"
        elif inventory_item.infusion == 400:
            infusion_name = "simple"
        elif inventory_item.infusion == 500:
            infusion_name = "crystal"
        elif inventory_item.infusion == 600:
            infusion_name = "fire"
        elif inventory_item.infusion == 700:
            infusion_name = "chaos"
        elif inventory_item.infusion == 800:
            infusion_name = "lighting"
        elif inventory_item.infusion == 900:
            infusion_name = "deep"
        elif inventory_item.infusion == 1000:
            infusion_name = "dark"
        elif inventory_item.infusion == 1100:
            infusion_name = "poison"
        elif inventory_item.infusion == 1200:
            infusion_name = "blood"
        elif inventory_item.infusion == 1300:
            infusion_name = "raw"
        elif inventory_item.infusion == 1400:
            infusion_name = "blessed"
        elif inventory_item.infusion == 1500:
            infusion_name = "hollow"

        label = item["label"]
        infusion_icon = None
        if infusion_name:
            infusion_icon = QtGui.QPixmap(
                get_resource("infusion_icons", f"{infusion_name}.png")
            )
            inf_label = item.get("infusion_label")
            if inf_label:
                label = inf_label.format(infusion=infusion_name.capitalize())
            else:
                label = " ".join([infusion_name.capitalize(), label])

        category = item["category"]
        image_name = item["image"]
        order = item.get("order") or 0
        pix = get_item_pixmap(image_name)
        inventory_amount = bottomless_amount = 0
        if in_bottomless_box:
            bottomless_amount = inventory_item.amount
        else:
            inventory_amount = inventory_item.amount

        new_item = QtGui.QStandardItem(label)
        new_item.setFlags(
            QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled
        )
        new_item.setData(upgrade_level, ITEM_LEVEL_ROLE)
        new_item.setData(infusion_icon, ITEM_INFUSION_ICON_ROLE)
        new_item.setData(order, ITEM_ORDER_ROLE)
        new_item.setData(inventory_amount, ITEM_AMOUNT_ROLE)
        new_item.setData(bottomless_amount, ITEM_STORAGE_BOX_AMOUNT_ROLE)
        new_item.setData(category, ITEM_CATEGORY_ROLE)
        new_item.setData(pix, ITEM_IMAGE_ROLE)
        return new_item


class InventoryProxyModel(QtCore.QSortFilterProxyModel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setSortCaseSensitivity(QtCore.Qt.CaseInsensitive)
        self.setFilterCaseSensitivity(QtCore.Qt.CaseInsensitive)
        self._category = None

    def set_category(self, category: str):
        if self._category == category:
            return
        self._category = category
        self.invalidateFilter()

    def lessThan(self, left, right):
        left_order = left.data(ITEM_ORDER_ROLE)
        right_order = right.data(ITEM_ORDER_ROLE)
        if left_order != right_order:
            return left_order < right_order
        return super().lessThan(left, right)

    def filterAcceptsRow(self, source_row, source_parent):
        if self._category is None:
            return True

        index = self.sourceModel().index(source_row, 0, source_parent)
        return index.data(ITEM_CATEGORY_ROLE) == self._category


class InventoryDelegate(QtWidgets.QStyledItemDelegate):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        stand_path = get_resource(
            "menu_icons", "dish.png"
        )
        inventory_bag_path = get_resource(
            "menu_icons", "menu_invetory.png"
        )
        bottomless_box_path = get_resource(
            "menu_icons", "menu_storage_box.png"
        )
        self._stand_pixmap = QtGui.QPixmap(stand_path)
        self._inventory_bag_pix = QtGui.QPixmap(inventory_bag_path)
        self._bottomless_box_pix = QtGui.QPixmap(bottomless_box_path)

    def sizeHint(self, option, index):
        # You can customize the size of each item here
        return QtCore.QSize(260, 80)

    def paint(self, painter, option, index):
        item_label = index.data(QtCore.Qt.DisplayRole)

        widget = option.widget
        style = widget.style()
        style_proxy = style.proxy()
        style_proxy.drawPrimitive(
            QtWidgets.QStyle.PE_PanelItemViewItem, option, painter, widget
        )

        pixmap = index.data(ITEM_IMAGE_ROLE)
        size = option.rect.height()
        pixmap = pixmap.scaled(
            size, size,
            QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation,
        )

        icon_rect = QtCore.QRect(option.rect)
        icon_rect.setWidth(pixmap.width())
        text_offset = icon_rect.right() + 20

        stand_height = 21
        stand_rect = QtCore.QRect(
            icon_rect.left() + 10,
            icon_rect.bottom() - stand_height,
            icon_rect.width() - 20,
            stand_height
        )

        painter.drawPixmap(stand_rect, self._stand_pixmap)
        painter.drawPixmap(icon_rect, pixmap)

        infusion_icon = index.data(ITEM_INFUSION_ICON_ROLE)
        if infusion_icon:
            infusion_size = int(option.rect.height() * 0.3)
            infusion_icon = infusion_icon.scaled(
                infusion_size, infusion_size,
                QtCore.Qt.KeepAspectRatio,
                QtCore.Qt.SmoothTransformation
            )
            infusion_rect = QtCore.QRect(
                icon_rect.right() - infusion_icon.width(),
                icon_rect.bottom() - (infusion_icon.height() - 5),
                infusion_icon.width(),
                infusion_icon.height()
            )
            painter.drawPixmap(infusion_rect, infusion_icon)

        level = index.data(ITEM_LEVEL_ROLE)
        if level:
            item_label = f"{item_label} + {level}"

        # Draw the text
        font = painter.font()
        font.setPointSize(12)
        painter.setFont(font)
        font_metrics = painter.fontMetrics()
        text_height = font_metrics.height()
        half_height = option.rect.height() // 2
        text_rect = option.rect.adjusted(
            text_offset,
            half_height - text_height,
            0, 0
        )
        text_rect.setHeight(text_height)
        painter.drawText(
            text_rect,
            QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter,
            item_label
        )

        inv_bag_pix = self._inventory_bag_pix.scaled(
            text_height, text_height,
            QtCore.Qt.KeepAspectRatio,
            QtCore.Qt.SmoothTransformation
        )
        btb_pix = self._bottomless_box_pix.scaled(
            text_height, text_height,
            QtCore.Qt.KeepAspectRatio,
            QtCore.Qt.SmoothTransformation
        )
        amount = index.data(ITEM_AMOUNT_ROLE)
        btm_amount = index.data(ITEM_STORAGE_BOX_AMOUNT_ROLE)

        pos = QtCore.QPoint(
            text_rect.x(),
            option.rect.top() + half_height + 10
        )
        painter.drawPixmap(pos, inv_bag_pix)
        pos.setX(pos.x() + inv_bag_pix.width() + 2)

        amount_rect = font_metrics.boundingRect("9999")
        amount_rect.moveTopLeft(pos)
        painter.drawText(
            amount_rect,
            QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter,
            str(amount)
        )
        pos.setX(pos.x() + amount_rect.width() + 5)

        painter.drawPixmap(pos, btb_pix)
        pos.setX(pos.x() + btb_pix.width() + 2)

        amount_rect.moveLeft(pos.x())
        painter.drawText(
            amount_rect,
            QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter,
            str(btm_amount)
        )


class CategoryButtonOverlay(QtWidgets.QWidget):
    def __init__(self, parent):
        super().__init__(parent)
        self.setAttribute(QtCore.Qt.WA_TransparentForMouseEvents)

        image_path = get_resource(
            "menu_icons", "inventory_overlay.png"
        )
        self._bg_pixmap = QtGui.QPixmap(image_path)

    def paintEvent(self, event):
        painter = QtGui.QPainter(self)
        painter.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)
        painter.drawPixmap(0, 0, self._bg_pixmap.scaled(
            self.width(), self.height(),
            QtCore.Qt.AspectRatioMode.KeepAspectRatio,
            QtCore.Qt.TransformationMode.SmoothTransformation
        ))


class CategoryButtons(QtWidgets.QWidget):
    category_changed = QtCore.Signal(str)

    def __init__(self, parent):
        super().__init__(parent)
        overlay_widget = CategoryButtonOverlay(self)

        layout = QtWidgets.QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        overlay_anim = QtCore.QVariantAnimation(self)
        overlay_anim.setDuration(100)

        btns_by_category = {}
        first_category = None
        # Example buttons for categories
        for category in CATEGORIES:
            btn = DS3InventoryCategoryButton(category, self)
            btn.stackUnder(overlay_widget)
            if first_category is None:
                first_category = category

            btns_by_category[category] = btn
            btn.clicked.connect(self.set_category)
            layout.addWidget(btn, 0)
        layout.addStretch(1)

        overlay_anim.valueChanged.connect(self._on_anim_value_change)
        overlay_anim.finished.connect(self._on_anim_finished)

        btns_by_category[first_category].set_selected(True)
        self._overlay_widget = overlay_widget
        self._btns_by_category = btns_by_category
        self._current_category = first_category
        self._overlay_anim = overlay_anim

    def showEvent(self, event):
        super().showEvent(event)
        btn = self._btns_by_category[self._current_category]
        self._overlay_widget.setGeometry(btn.geometry())
        self._overlay_widget.raise_()

    def resizeEvent(self, event):
        super().resizeEvent(event)
        btn = self._btns_by_category[self._current_category]
        self._overlay_widget.resize(btn.size())

    def get_category(self):
        return self._current_category

    def set_category(self, category: str):
        if category not in self._btns_by_category:
            return
        if self._current_category == category:
            return

        self._btns_by_category[self._current_category].set_selected(False)
        btn = self._btns_by_category[category]
        btn.set_selected(True)
        self._current_category = category
        self.category_changed.emit(category)
        if self._overlay_anim.state() == QtCore.QAbstractAnimation.State.Running:
            self._overlay_anim.stop()
        self._overlay_anim.setStartValue(self._overlay_widget.pos())
        self._overlay_anim.setEndValue(btn.pos())
        self._overlay_anim.start()

    def _on_anim_value_change(self, pos: QtCore.QPoint):
        self._overlay_widget.move(pos)

    def _on_anim_finished(self):
        pos = self._overlay_anim.endValue()
        self._overlay_widget.move(pos)


class InventoryWidget(QtWidgets.QWidget):
    def __init__(self, parent: QtWidgets.QWidget):
        super().__init__(parent)

        category_btns = CategoryButtons(self)
        view = QtWidgets.QListView(self)
        view.setObjectName("ds_list_view")
        view.setVerticalScrollMode(
            QtWidgets.QAbstractItemView.ScrollMode.ScrollPerPixel
        )
        vsb = view.verticalScrollBar()
        vsb.setSingleStep(15)

        delegate = InventoryDelegate(parent)

        model = InventoryModel(parent)
        proxy = InventoryProxyModel()
        proxy.setSourceModel(model)

        view.setModel(proxy)
        view.setItemDelegate(delegate)

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(category_btns, 0)
        layout.addWidget(view, 1)

        category_btns.category_changed.connect(self._on_category_change)
        proxy.set_category(category_btns.get_category())

        self._view = view
        self._model = model
        self._proxy = proxy
        self._delegate = delegate

    def set_char(self, char: DS3Character | None):
        self._model.set_char(char)
        self._proxy.sort(0, QtCore.Qt.AscendingOrder)

    def _on_category_change(self, category: str):
        self._proxy.set_category(category)
