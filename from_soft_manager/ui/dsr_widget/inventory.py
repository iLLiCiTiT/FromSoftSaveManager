import os

from PySide6 import QtCore, QtGui, QtWidgets

from from_soft_manager.parse import DSRCharacter, ITEMS_BY_IDS, InventoryItem

from .resources import get_resource

ITEM_LEVEL_ROLE = QtCore.Qt.UserRole + 1
ITEM_INFUSION_ICON_ROLE = QtCore.Qt.UserRole + 2
ITEM_ORDER_ROLE = QtCore.Qt.UserRole + 3
ITEM_AMOUNT_ROLE = QtCore.Qt.UserRole + 4
ITEM_DURABILITY_ROLE = QtCore.Qt.UserRole + 5
IS_IN_BOTOMLESS_ROLE = QtCore.Qt.UserRole + 6
ITEM_IMAGE_ROLE = QtCore.Qt.UserRole + 7


def get_item_pixmap(category: str, image_name: str) -> QtGui.QPixmap | None:
    path = get_resource(category, f"{image_name}.png")
    if not os.path.exists(path):
        path = get_resource("unknown.png")
    return QtGui.QPixmap(path)


class InventoryModel(QtGui.QStandardItemModel):
    def __init__(self, parent):
        super().__init__(parent)

    def _create_unknown_item(self, inventory_item: InventoryItem) -> QtGui.QStandardItem:
        item_name = f"NA {inventory_item.item_type} {inventory_item.item_id}"
        new_item = QtGui.QStandardItem(item_name)
        new_item.setData(inventory_item.upgrade_level, ITEM_LEVEL_ROLE)
        new_item.setData(inventory_item.in_botomless_box, IS_IN_BOTOMLESS_ROLE)
        new_item.setData(inventory_item.order, ITEM_ORDER_ROLE)
        new_item.setData(inventory_item.amount, ITEM_AMOUNT_ROLE)
        new_item.setData(inventory_item.durability, ITEM_DURABILITY_ROLE)
        new_item.setData(
            get_item_pixmap("unknown", "unknown"),
            ITEM_IMAGE_ROLE
        )
        return new_item

    def set_char(self, char: DSRCharacter | None):
        root_item = self.invisibleRootItem()
        root_item.removeRows(0, root_item.rowCount())
        if char is None:
            return

        new_items = []
        for inventory_item in char.inventory_items:
            item_type = inventory_item.item_type
            item_id = inventory_item.item_id
            items_by_id = ITEMS_BY_IDS.get(item_type, {})
            item = items_by_id.get(item_id)
            if not item:
                model_item = self._create_unknown_item(inventory_item)
                new_items.append(model_item)
                continue

            # Skip fists
            if item["type"] == 0 and item["id"] == 900000:
                continue

            # Skip "no armor"
            if item["type"] == 268435456 and item["id"] in (
                900000, 901000, 902000, 903000
            ):
                continue

            upgrade_level = inventory_item.upgrade_level

            infusion_name = None
            if inventory_item.infusion == 0:
                pass
            elif inventory_item.infusion == 100:
                infusion_name = "crystal"
            elif inventory_item.infusion == 200:
                infusion_name = "lightning"
            elif inventory_item.infusion == 300:
                infusion_name = "raw"
            elif inventory_item.infusion == 400:
                infusion_name = "magic"
                if upgrade_level >= 5:
                    infusion_name += "_2"
            elif inventory_item.infusion == 500:
                infusion_name = "enchanted"
            elif inventory_item.infusion == 600:
                infusion_name = "divine"
                if upgrade_level >= 5:
                    infusion_name += "_2"
            elif inventory_item.infusion == 700:
                infusion_name = "occult"
            elif inventory_item.infusion == 800:
                infusion_name = "fire"
                if upgrade_level >= 5:
                    infusion_name += "_2"
            elif inventory_item.infusion == 900:
                infusion_name = "chaos"

            infusion_icon = None
            if infusion_name:
                infusion_icon = QtGui.QPixmap(
                    get_resource("infusions", f"{infusion_name}.png")
                )
            name = item["name"]
            category = item["category"]
            image_name = item["image"]
            pix = get_item_pixmap(category, image_name)
            new_item = QtGui.QStandardItem(name)
            new_item.setFlags(
                QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled
            )
            new_item.setData(upgrade_level, ITEM_LEVEL_ROLE)
            new_item.setData(infusion_icon, ITEM_INFUSION_ICON_ROLE)
            new_item.setData(inventory_item.order, ITEM_ORDER_ROLE)
            new_item.setData(inventory_item.in_botomless_box, IS_IN_BOTOMLESS_ROLE)
            new_item.setData(inventory_item.order, ITEM_ORDER_ROLE)
            new_item.setData(inventory_item.amount, ITEM_AMOUNT_ROLE)
            new_item.setData(inventory_item.durability, ITEM_DURABILITY_ROLE)
            new_item.setData(pix, ITEM_IMAGE_ROLE)
            new_items.append(new_item)

        if new_items:
            root_item.appendRows(new_items)


class InventoryProxyModel(QtCore.QSortFilterProxyModel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setSortCaseSensitivity(QtCore.Qt.CaseInsensitive)
        self.setFilterCaseSensitivity(QtCore.Qt.CaseInsensitive)

    def lessThan(self, left, right):
        left_order = left.data(ITEM_ORDER_ROLE)
        right_order = right.data(ITEM_ORDER_ROLE)
        if left_order != right_order:
            return left_order < right_order
        return super().lessThan(left, right)


class InventoryDelegate(QtWidgets.QStyledItemDelegate):
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
        text_offset = 0
        if pixmap:
            size = option.rect.height() - 20
            pixmap = pixmap.scaled(
                size, size,
                QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation
            )

            icon_rect = QtCore.QRect(option.rect)
            icon_rect.setWidth(pixmap.width())
            icon_rect.adjust(10, 10, 10, -10)
            text_offset = pixmap.width() + 20
            painter.drawPixmap(icon_rect, pixmap)

        level = index.data(ITEM_LEVEL_ROLE)
        if level:
            item_label = f"{item_label}+{level}"
        infusion_icon = index.data(ITEM_INFUSION_ICON_ROLE)
        text_rect = option.rect.adjusted(text_offset, 0, 0, 0)
        text_size_pt = 12
        if infusion_icon:
            icon_size = text_size_pt * 2
            infusion_icon = infusion_icon.scaled(
                icon_size, icon_size,
                QtCore.Qt.KeepAspectRatio,
                QtCore.Qt.SmoothTransformation
            )
            icon_rect = QtCore.QRect(text_rect)
            icon_rect.setWidth(icon_size)
            icon_rect.setHeight(icon_size)
            painter.drawPixmap(icon_rect, infusion_icon)
            text_rect.adjust(icon_size + 2, 0, 0, 0)

        # Draw the text
        font = painter.font()
        font.setPointSize(text_size_pt)
        painter.setFont(font)
        painter.drawText(
            text_rect,
            QtCore.Qt.AlignLeft | QtCore.Qt.AlignTop,
            item_label
        )


class InventoryWidget(QtWidgets.QWidget):
    def __init__(self, parent: QtWidgets.QWidget):
        super().__init__(parent)
        view = QtWidgets.QListView(self)

        delegate = InventoryDelegate(parent)

        model = InventoryModel(parent)
        proxy = InventoryProxyModel()
        proxy.setSourceModel(model)

        view.setModel(proxy)
        view.setItemDelegate(delegate)

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(view, 1)

        self._view = view
        self._model = model
        self._proxy = proxy
        self._delegate = delegate

    def set_char(self, char: DSRCharacter | None):
        self._model.set_char(char)
        self._proxy.sort(0, QtCore.Qt.AscendingOrder)
