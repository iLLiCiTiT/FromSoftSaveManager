import QtQuick
import QtQuick.Controls

Item {
    id: categoryButton
    width: 48
    height: 48

    property int category: 0
    property string categoryName: ""
    property bool isSelected: false
    property bool isHovered: mouseArea.containsMouse

    signal clicked(int category)

    Rectangle {
        id: bg
        anchors.fill: parent
        color: "transparent"
        visible: isSelected || isHovered

        Image {
            anchors.fill: parent
            source: "qrc:/dsr_images/inventory_overlay"
            opacity: 0.5
        }
    }

    Image {
        id: icon
        anchors.centerIn: parent
        width: 40
        height: 40
        source: isSelected || isHovered 
                ? "qrc:/dsr_images/inventory_" + categoryName + "_hover"
                : "qrc:/dsr_images/inventory_" + categoryName
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: categoryButton.clicked(category)
    }
}
