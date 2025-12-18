import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: 300
    height: 600

    property var inventoryModel: null
    property int currentCategory: 7 // Default to Spells based on C++ code

    Column {
        anchors.fill: parent
        spacing: 10

        // Category Buttons
        Row {
            id: categoryRow
            width: parent.width
            height: 50
            spacing: 5
            leftPadding: 5

            readonly property var categories: [
                { id: 0, name: "consumables" },
                { id: 1, name: "materials" },
                { id: 2, name: "key_items" },
                { id: 3, name: "ammunition" },
                { id: 4, name: "weapons_shields" },
                { id: 5, name: "rings" },
                { id: 6, name: "armor" },
                { id: 7, name: "spells" }
            ]

            Repeater {
                model: categoryRow.categories
                CategoryButton {
                    category: modelData.id
                    categoryName: modelData.name
                    isSelected: root.currentCategory === modelData.id
                    onClicked: (cat) => root.currentCategory = cat
                }
            }
        }

        // Inventory List
        ListView {
            id: inventoryList
            width: parent.width
            height: parent.height - categoryRow.height - 10
            clip: true
            model: root.inventoryModel

            delegate: Item {
                width: inventoryList.width
                height: 80
                visible: model.category === root.currentCategory

                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.color: "#333"
                    border.width: 1

                    Row {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 10

                        // Item Icon
                        Item {
                            width: 64
                            height: 64
                            anchors.verticalCenter: parent.verticalCenter
                            
                            Image {
                                anchors.fill: parent
                                source: model.itemImage ? model.itemImage : ""
                                fillMode: Image.PreserveAspectFit
                            }
                            
                            Image {
                                anchors.bottom: parent.bottom
                                anchors.right: parent.right
                                width: 24
                                height: 24
                                source: model.infusionIcon ? model.infusionIcon : ""
                                visible: !!model.infusionIcon
                            }
                        }

                        // Item Details
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 4

                            Text {
                                text: model.display + (model.level > 0 ? " +" + model.level : "")
                                color: "white"
                                font.pixelSize: 16
                            }

                            Row {
                                spacing: 15
                                
                                Row {
                                    spacing: 4
                                    Image {
                                        source: "qrc:/dsr_images/inventory_bag"
                                        width: 16
                                        height: 16
                                    }
                                    Text {
                                        text: model.amount
                                        color: "#aaa"
                                        font.pixelSize: 14
                                    }
                                }

                                Row {
                                    spacing: 4
                                    Image {
                                        source: "qrc:/dsr_images/bottomless_box"
                                        width: 16
                                        height: 16
                                    }
                                    Text {
                                        text: model.bottomlessBoxAmount
                                        color: "#aaa"
                                        font.pixelSize: 14
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
