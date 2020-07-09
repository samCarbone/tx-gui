import QtQuick 2.0
import QtQuick.Controls 2.0

// Button with two states: enabled, disabled
// Changes text and background color with the states

Button {

    id: button
    state: "disabled"
    width: 100
    height: 40

    property alias borderColor: back.border.color
    property alias textColor: label.color
    property alias fontSize: label.font.pixelSize
    property string textEnabled: qsTr("Disable")
    property string textDisabled: qsTr("Enable")
    property string colorEnabled: "seagreen"
    property string colorDisabled: "tomato"

    contentItem: Text {
        id: label
        text: button.text
        // font: sendButton.font
        color: "white"
        font.bold: true
        font.pixelSize: 15
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        id: back
        implicitWidth: 100
        implicitHeight: 40
        border.width: 1
        radius: 2
    }

    states: [
        State {
            name: "enabled"
            PropertyChanges { target: button; text: textEnabled}
            PropertyChanges { target: back; color: button.colorEnabled}
        },
        State {
            name: "disabled"
            PropertyChanges { target: button; text: textDisabled}
            PropertyChanges { target: back; color: button.colorDisabled}
        }

    ]

    onClicked: state = (state == "disabled"? "enabled" : "disabled")

}
