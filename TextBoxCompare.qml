import QtQuick 2.0

Item {

    property alias textLeft: modeLeft.text
    property alias textRight: modeRight.text
    property string textErr: "err"
    property string colorGood: "seagreen"
    property string colorBad: "tomato"

    id: root
    width: modeLeft.width + rectangle.width + 5
    height: rectangle.height

    Row {

        anchors.verticalCenter: parent.verticalCenter

        Text {
            id: modeLeft
            font.bold: true
            text: "--"
            width: contentWidth
            height: contentHeight
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: ":"
            font.bold: true
            width: contentWidth
            height: contentHeight
            anchors.verticalCenter: parent.verticalCenter
        }

        Rectangle {

            // Green if they match, red if they are different
            // Also red if they are both the error state
            id: rectangle
            color: modeLeft.text != modeRight.text ? colorBad : (modeLeft.text == textErr ? colorBad : colorGood)
            width: modeRight.width*1.2
            height: modeRight.height*1.2
            radius: 2
            anchors.verticalCenter: parent.verticalCenter


            Text {
                id: modeRight
                font.bold: true
                text: "--"
                width: contentWidth
                height: contentHeight
                anchors.centerIn: parent
//                color: "white"
            }

        }
    }

}
