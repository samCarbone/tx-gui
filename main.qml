import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.0
import QtGamepad 1.12
import QtCharts 2.12

Window {
    id: root
    visible: true
    width: 640
    height: 480
    title: qsTr("Joystick")

    property int currentJoystick: 0

    Row {
        id: bottomRow
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        spacing: 20

        Joystick {id: joyLeft; xLabel: "Rud"; yLabel: "Thr"}

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
                ChannelBar { id: chC; name: "Ch3"}
                ChannelBar { id: chD; name: "Ch4"}

            }

            ColorButton {
                id: txEnableButton
                textEnabled: qsTr("Disable Tx")
                textDisabled: qsTr("Enable Tx")
                anchors.horizontalCenter: parent.horizontalCenter
            }

            TextBoxCompare {
                id: modeBox
                textLeft: getModeText(Transmitter.desiredEspMode)
                textRight: getModeText(Transmitter.espMode)
                textErr: getModeText(0)
                anchors.horizontalCenter: parent.horizontalCenter
            }

        }


        Joystick {id: joyRight; xLabel: "Ail"; yLabel: "Ele"}

    }

    Item {
        Text {
            id: ping
            text: "--"
        }
    }

    ChartView {
        title: "Scatters"
        antialiasing: true
        width: 400
        height: 300
        id: chart1

        ScatterSeries {
            id: scatter1
            name: "Altitude"
            axisX: valueXAxis
            axisY: valueYAxis
            markerSize: 2
            borderColor: "green"
            borderWidth: 2

        }

        // Define x-axis to be used with the series instead of default one
        ValueAxis {
            id: valueXAxis
            min: 0
            max: 1000
        }

        ValueAxis {
            id: valueYAxis
            min: 0
            max: 2000
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
        joyLeft.valueX = Math.round(Transmitter.getChannelValue(3))
        joyLeft.valueY = Math.round(Transmitter.getChannelValue(2))
        joyRight.valueX = Math.round(Transmitter.getChannelValue(0))
        joyRight.valueY = Math.round(Transmitter.getChannelValue(1))
        chA.value = Math.round(Transmitter.getChannelValue(4))
        chB.value = Math.round(Transmitter.getChannelValue(5))
        chC.value = Math.round(Transmitter.getChannelValue(6))
        chD.value = Math.round(Transmitter.getChannelValue(7))
    }

    Connections {
        target: Transmitter
        function onChannelChanged(axis, value) { updateAxes() }
    }

    // Periodically send the channel values over UDP
    Timer {
        interval:50 // ms
        running: txEnableButton.state == "enabled"
        repeat: true
        onTriggered: Transmitter.sendChannelsWithMode();
    }

    // Periodically send a ping to the esp
    Timer {
        interval: 501 // ms
        running: true
        repeat: true
        onTriggered: Transmitter.sendPing();
    }

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
        function onAltitudeRangeReceived(timestamp, range) {
            if(range > valueYAxis.max) {
                valueYAxis.max = range*1.05;
            }

            valueXAxis.max = timestamp/1000;
            valueXAxis.min = timestamp/1000 - 20;

            scatter1.append(timestamp/1000, range);
        }
    }

    Connections {
        target: Transmitter
        function onPingReceived(pingLoopTime) { ping.text = pingLoopTime; }
    }

}
