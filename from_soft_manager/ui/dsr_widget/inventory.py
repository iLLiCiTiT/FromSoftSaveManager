import os

from PySide6 import QtCore, QtGui, QtWidgets

from from_soft_manager.parse import DSRCharacter, ITEMS_BY_IDS, InventoryItem

from .resources import get_resource

ITEM_LEVEL_ROLE = QtCore.Qt.UserRole + 1
ITEM_INFUSION_ROLE = QtCore.Qt.UserRole + 2
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
        new_item.setData(inventory_item.infusion, ITEM_INFUSION_ROLE)
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
                model_item = self._create_unknown_item(item_type, item_id)
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

            name = item["name"]
            category = item["category"]
            image_name = item["image"]
            pix = get_item_pixmap(category, image_name)
            new_item = QtGui.QStandardItem(name)
            new_item.setFlags(
                QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled
            )
            new_item.setData(inventory_item.upgrade_level, ITEM_LEVEL_ROLE)
            new_item.setData(inventory_item.infusion, ITEM_INFUSION_ROLE)
            new_item.setData(inventory_item.order, ITEM_ORDER_ROLE)
            new_item.setData(inventory_item.in_botomless_box, IS_IN_BOTOMLESS_ROLE)
            new_item.setData(inventory_item.order, ITEM_ORDER_ROLE)
            new_item.setData(inventory_item.amount, ITEM_AMOUNT_ROLE)
            new_item.setData(inventory_item.durability, ITEM_DURABILITY_ROLE)
            new_item.setData(pix, ITEM_IMAGE_ROLE)
            new_items.append(new_item)

        if new_items:
            root_item.appendRows(new_items)


class InventoryDelegate(QtWidgets.QStyledItemDelegate):
    def sizeHint(self, option, index):
        # You can customize the size of each item here
        return QtCore.QSize(400, 100)

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
            pixmap = pixmap.scaled(
                option.rect.height(), option.rect.height(),
                QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation
            )

            icon_rect = QtCore.QRect(option.rect)
            icon_rect.setWidth(pixmap.width())
            text_offset = pixmap.width() + 10
            painter.drawPixmap(icon_rect, pixmap)

        level = index.data(ITEM_LEVEL_ROLE)
        if level:
            item_label = f"{item_label}+{level}"
        infusion = index.data(ITEM_INFUSION_ROLE)
        if infusion:
            item_label = f"{item_label} ({infusion})"
        # Draw the text
        text_rect = option.rect.adjusted(text_offset, 0, 0, 0)
        painter.drawText(
            text_rect,
            QtCore.Qt.AlignLeft | QtCore.Qt.AlignVCenter,
            item_label
        )


class InventoryWidget(QtWidgets.QWidget):
    def __init__(self, parent: QtWidgets.QWidget):
        super().__init__(parent)
        view = QtWidgets.QListView(self)

        model = InventoryModel(parent)
        delegate = InventoryDelegate(parent)

        view.setModel(model)
        view.setItemDelegate(delegate)

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(view, 1)

        self._view = view
        self._model = model
        self._delegate = delegate

    def set_char(self, char: DSRCharacter | None):
        self._model.set_char(char)
