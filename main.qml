import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.15
import QtGamepad 1.12
import QtQuick.Layouts 1.12

ApplicationWindow {
    property int currentJoystick: 0

    id: root
    visible: true
    width: 860
    height: 640
    title: qsTr("Joystick")

    TabBar {
        id: bar
        width: parent.width
        anchors.left: parent.left
        anchors.top: parent.top
        topPadding: 5
        leftPadding: 5

        TabButton {
            text: qsTr("Home")
            padding: 5
            width: implicitWidth
            height: implicitHeight
        }
        TabButton {
            text: qsTr("Logging")
            padding: 5
            width: implicitWidth
            height: implicitHeight
        }
        TabButton {
            text: qsTr("Activity")
            padding: 5
            width: implicitWidth
            height: implicitHeight
        }
    }

    StackLayout {
        width: bar.width
        anchors.horizontalCenter: bar.horizontalCenter
        anchors.top: bar.bottom
        anchors.bottom: bottomRow.top
        Layout.bottomMargin: 5
        currentIndex: bar.currentIndex

        Item {
            id: homeTab
            Layout.fillWidth: true
            Layout.fillHeight: true

            FirstPage {anchors.fill: parent}

        }

        Item {
            id: activityTab
            // SecondPage {width: parent.width/2; height: parent.height/2}
        }

        Item {
            id: discoverTab
        }

    }


    Row {
        id: bottomRow
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        bottomPadding: 10
        spacing: 20

        Joystick {id: joyLeft; xLabel: "Rud"; yLabel: "Thr"; secondary: rectActiveCtrl.state == "enabled";}

        Column {

            spacing: 2

            Row {
                id: channelBarRow
                spacing: 4
                width: 4*30+12
                height: 75
                anchors.horizontalCenter: parent.horizontalCenter

                ChannelBar { id: chA; name: "Arm"}
                ChannelBar { id: chB; name: "Mod"}
                ChannelBar { id: chC; name: "Ch7"}
                ChannelBar { id: chD; name: "Ch8"}

            }

            ColorButton {
                id: txEnableButton
                textEnabled: qsTr("Disable Tx")
                textDisabled: qsTr("Enable Tx")
                anchors.horizontalCenter: parent.horizontalCenter

                onStateChanged: {
                    if(txEnableButton.state === "enabled") {
                        Transmitter.txTransmit = true;
                    }
                    else if(txEnableButton.state === "disabled") {
                        Transmitter.txTransmit = false;
                    }
                }
            }

            TextBoxCompare {
                id: modeBox
                textLeft: getModeText(Transmitter.desiredEspMode)
                textRight: getModeText(Transmitter.currentEspMode)
                textErr: getModeText(0)
                anchors.horizontalCenter: parent.horizontalCenter
            }


        }

        Joystick {id: joyRight; xLabel: "Ail"; yLabel: "Ele"}

        Grid {
            columns: 1
            id: controllerGrid
            spacing: 2

            horizontalItemAlignment: Grid.AlignHCenter
            verticalItemAlignment: Grid.AlignVCenter

            CheckBox {
                width: implicitWidth
                height: implicitHeight
                id: checkboxCtrl
                checked: false
                indicator.width: 20
                indicator.height: 20
                text: qsTr("Standby Alt Controller")

                onToggled: {
                    Transmitter.controllerStandby = checked;
                }
            }

            Item {
                width: textActiveCtrl.width + rectActiveCtrl.width + 5
                height: rectActiveCtrl.height

                Text {
                    id: textActiveCtrl
                    text: "PC Alt Controller"
                    width: contentWidth + 16
                    height: contentHeight
                    leftPadding: 5
                }

                Rectangle {
                    // Green if active, red if inactive
                    id: rectActiveCtrl
                    state: "disabled"
                    height: textActiveCtrl.height*1.2
                    width: height
                    radius: 1
                    anchors.right: textActiveCtrl.left
                    anchors.verticalCenter: textActiveCtrl.verticalCenter

                    states: [
                        State {
                            name: "enabled"
                            PropertyChanges { target: rectActiveCtrl; color: "seagreen"}
                        },
                        State {
                            name: "disabled"
                            PropertyChanges { target: rectActiveCtrl; color: "tomato"}
                        }
                    ]
                }

            }

            Item {
                width: textJVCtrl.width + rectJVCtrl.width + 5
                height: rectJVCtrl.height

                Text {
                    id: textJVCtrl
                    text: "JV Alt Controller"
                    width: contentWidth + 16
                    height: contentHeight
                    leftPadding: 5
                }

                Rectangle {
                    // Green if active, red if inactive
                    id: rectJVCtrl
                    state: "error"
                    height: textJVCtrl.height*1.2
                    width: height
                    radius: 1
                    anchors.right: textJVCtrl.left
                    anchors.verticalCenter: textJVCtrl.verticalCenter

                    states: [
                        State {
                            name: "enabled"
                            PropertyChanges { target: rectJVCtrl; color: "seagreen"}
                        },
                        State {
                            name: "disabled"
                            PropertyChanges { target: rectJVCtrl; color: "tomato"}
                        },
                        State {
                            name: "error"
                            PropertyChanges { target: rectJVCtrl; color: "lightgrey"}
                        }
                    ]
                }

            }



            Item {
                width: textJVLand.width + rectJVLand.width + 5
                height: rectJVLand.height

                Text {
                    id: textJVLand
                    text: "JV Landing"
                    width: contentWidth + 16
                    height: contentHeight
                    leftPadding: 5
                }

                Rectangle {
                    // Green if active, red if inactive
                    id: rectJVLand
                    state: "error"
                    height: textJVLand.height*1.2
                    width: height
                    radius: 1
                    anchors.right: textJVLand.left
                    anchors.verticalCenter: textJVLand.verticalCenter

                    states: [
                        State {
                            name: "enabled"
                            PropertyChanges { target: rectJVLand; color: "seagreen"}
                        },
                        State {
                            name: "disabled"
                            PropertyChanges { target: rectJVLand; color: "tomato"}
                        },
                        State {
                            name: "error"
                            PropertyChanges { target: rectJVLand; color: "lightgrey"}
                        }
                    ]
                }

            }




            ColorButton_no {
                id: landButton
                textEnabled: qsTr("Land")
                textDisabled: qsTr("Land")
                colorEnabled: "seagreen"
                colorDisabled: "tomato"
                state: "inactive"

                onClicked: {
                    if(state != "inactive") {
                        if(state === "disabled") {
                            Transmitter.setDesiredJvLanding(1);
                        }
                        else if(state === "enabled") {
                            Transmitter.setDesiredJvLanding(2);
                        }
                    }
                }
            }

        }

    }







//    Connections {
//        target: QJoysticks
//        function onAxisChanged(js, axis, value) {
//            if (currentJoystick === js && axis === 2)
//                joyLeft.valueX = Math.round(QJoysticks.getAxis (js, axis) * 100)
//            if (currentJoystick === js && axis === 0)
//                joyLeft.valueY = Math.round(QJoysticks.getAxis (js, axis) * 100)
//            if (currentJoystick === js && axis === 1)
//                joyRight.valueX = Math.round(QJoysticks.getAxis (js, axis) * 100)
//            if (currentJoystick === js && axis === 4)
//                joyRight.valueY = Math.round((QJoysticks.getAxis (js, axis)-0.5) *2* 100)
//            if (currentJoystick === js && axis === 3)
//                ch1.value = Math.round(QJoysticks.getAxis (js, axis) * 100)
//            if (currentJoystick === js && axis === 5)
//                ch2.value = Math.round(QJoysticks.getAxis (js, axis) * 100)
//        }
//    }


    function updateAxes() {
        joyLeft.valueX = Math.round(Transmitter.getJoyChannelValue(3))
        joyLeft.valueY = Math.round(Transmitter.getJoyChannelValue(2))
        joyRight.valueX = Math.round(Transmitter.getJoyChannelValue(0))
        joyRight.valueY = Math.round(Transmitter.getJoyChannelValue(1))
        chA.value = Math.round(Transmitter.getJoyChannelValue(4))
        chB.value = Math.round(Transmitter.getJoyChannelValue(5))
        chC.value = Math.round(Transmitter.getJoyChannelValue(6))
        chD.value = Math.round(Transmitter.getJoyChannelValue(7))
    }

    Connections {
        target: Transmitter
        function onJoyChannelChanged(axis, value) { updateAxes() }
    }

    Connections {
        target: Transmitter
        function onControllerChannelChanged(axis, value) {

            if(axis === 2) { joyLeft.secondaryValueY = Math.round(value); }
        }
    }

//    // Periodically send the channel values over UDP
//    Timer {
//        interval:120 // ms
//        running: txEnableButton.state == "enabled"
//        repeat: true
//        onTriggered: Transmitter.sendChannelsWithMode();
//    }

//    // Periodically send a ping to the esp
//    Timer {
//        interval: 501 // ms
//        running: true
//        repeat: true
//        onTriggered: Transmitter.sendPing(true);
//    }

    function getModeText(mode) {
        var resp = "err";
        if(mode === 0) {resp = "err";}
        else if(mode === 1) {resp = "pc";}
        else if(mode === 2) {resp = "jv";}
        return resp;
    }

    Connections {
        target: Transmitter
        function onEspModeChanged(mode) { modeBox.textRight = getModeText(mode) }
    }

    Connections {
        target: Transmitter
        function onDesiredEspModeChanged(mode) { modeBox.textLeft = getModeText(mode) }
    }

    Connections {
        target: Transmitter
        function onControllerActiveChanged(isActive) { rectActiveCtrl.state = isActive ? "enabled" : "disabled" }
    }

    Connections {
        target: Transmitter
        function onJvControllerChanged(ctrl_mode) {
            if(ctrl_mode === 0) {
                rectJVCtrl.state = "error";
            }
            else if(ctrl_mode === 1) {
                rectJVCtrl.state = "enabled";
            }
            else if(ctrl_mode === 2) {
                rectJVCtrl.state = "disabled";
            }
        }
    }

    Connections {
        target: Transmitter
        function onJvLandingChanged(land_mode) {
            if(land_mode === 0) {
                rectJVLand.state = "error";
            }
            else if(land_mode === 1) {
                rectJVLand.state = "enabled";
            }
            else if(land_mode === 2) {
                rectJVLand.state = "disabled";
            }
        }
    }

    Connections {
        target: Transmitter
        function onDesiredJvControllerChanged(ctrl_mode) {
            if(ctrl_mode === 0) {
                landButton.state = "inactive";
            }
            else if(ctrl_mode === 1) {
                if(landButton.state === "inactive") {
                    landButton.state = "disabled";
                }
            }
            else if(ctrl_mode === 2) {
                landButton.state = "inactive";
            }
        }
    }

    Connections {
        target: Transmitter
        function onDesiredJvLandingChanged(land_mode) {
            if(land_mode === 0) {
                if(landButton.state != "inactive") {
                    landButton.state = "disabled";
                }
            }
            else if(land_mode === 1) {
                landButton.state = "enabled";
            }
            else if(land_mode === 2) {
                if(landButton.state != "inactive") {
                    landButton.state = "disabled";
                }
            }
        }
    }

}
