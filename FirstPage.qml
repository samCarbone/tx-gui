
import QtQuick 2.12
import QtCharts 2.12
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.15


Item {

    id: page

    enum LogLevels { ALL, DEBUG, INFO, WARN, ERROR, FATAL, OFF }
    // enum Devices { PC, ESP, JV }
    enum Devices { PC, JV, ESP, FC }

    ChartView {
        title: "Altitude"
        antialiasing: true
        anchors.top: parent.top
        anchors.left: parent.left
        height: parent.height/2
        width: parent.width/2
        id: chartAltitude
        legend.alignment: Qt.AlignRight
        margins.bottom: 0
        margins.left: 10
        margins.top: 0
        margins.right: 0

        ScatterSeries {
            id: scatterRangeMeas
            name: "Alt Meas"
            axisX: xAxisAlt
            axisY: yAxisAlt
            markerSize: 4
            borderColor: "black"
            color: "darkgreen"
            borderWidth: 1
            useOpenGL: true
            property int numPoints: 0

        }

        ScatterSeries {
            id: scatterAltEst
            name: "Alt Est"
            axisX: xAxisAlt
            axisY: yAxisAlt
            markerSize: 3
            borderColor: "red"
            color: "red"
            borderWidth: 1
            useOpenGL: true
            property int numPoints: 0

        }

        ScatterSeries {
            id: scatterAltPropEst
            name: "Alt Prop Est"
            axisX: xAxisAlt
            axisY: yAxisAlt
            markerSize: 3
            borderColor: "blue"
            color: "blue"
            borderWidth: 1
            useOpenGL: true
            property int numPoints: 0

        }

        // Define x-axis to be used with the series instead of default one
        ValueAxis {
            id: xAxisAlt
            min: 0
            max: 1000
        }

        ValueAxis {
            id: yAxisAlt
            min: 0
            max: 2
        }
    }

    ChartView {
        title: "Velocity"
        antialiasing: true
        anchors.top: chartAltitude.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        // height: parent.height
        width: parent.width/2
        id: chartVelocity
        legend.alignment: Qt.AlignRight
        margins.bottom: 0
        margins.left: 10
        margins.top: 0
        margins.right: 0


        ScatterSeries {
            id: scatterVelEst
            name: "Vel Est "
            axisX: xAxisVelocity
            axisY: yAxisVelocity
            markerSize: 3
            borderColor: "red"
            color: "red"
            borderWidth: 1
            useOpenGL: true
            property int numPoints: 0

        }

        ScatterSeries {
            id: scatterVelPropEst
            name: "Vel Prop Est"
            axisX: xAxisVelocity
            axisY: yAxisVelocity
            markerSize: 4
            borderColor: "blue"
            color: "blue"
            borderWidth: 1
            useOpenGL: true
            property int numPoints: 0

        }

        // Define x-axis to be used with the series instead of default one
        ValueAxis {
            id: xAxisVelocity
            min: 0
            max: 1000
        }

        ValueAxis {
            id: yAxisVelocity
            min: -5
            max: 5
        }
    }


    ScrollView {
        // anchors.fill: parent
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.verticalCenter
        anchors.left: parent.horizontalCenter
        TextArea {

            id: logTextArea
            readOnly: true
            background: Rectangle {
                height: parent.height
                width: parent.width
                color: "ghostwhite"
                border.color: "darkgrey"
            }

        }
    }


    Button {
        id: selectFolderButton
        anchors.horizontalCenter: fileSaveGrid.horizontalCenter
        anchors.bottom: fileSaveGrid.top
        text: "Select Folder"
        width: 90
        height: 20
        state: "notHovered"

        property string colorHovered: "powderblue"
        property string colorNotHovered: "aliceblue"

        background: Rectangle {
            id: selectFolderButtonBackground
            border.color: "slategrey"
            radius: 2
        }

        contentItem: Text {
            text: parent.text
            font.pixelSize: 12
            anchors.fill: parent
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }

        states: [
            State {
                name: "hovered"
                PropertyChanges { target: selectFolderButtonBackground; color: selectFolderButton.colorHovered}
            },
            State {
                name: "notHovered"
                PropertyChanges { target: selectFolderButtonBackground; color: selectFolderButton.colorNotHovered}
            }
        ]

        onHoveredChanged: {
            state = hovered ? "hovered" : "notHovered"
        }

        onPressed: {
            if(recordButton.state != "enabled") {
                saveFolderDialog.open()
            }
        }

    }

    Grid {

        id: fileSaveGrid
        columns: 2
        spacing: 5
        padding: 5

        anchors.right: parent.right
        y: parent.height/2+50
        rightPadding: 10
        bottomPadding: 10

        horizontalItemAlignment: Grid.AlignHCenter
        verticalItemAlignment: Grid.AlignVCenter

        Text {
            id: suffixPrompt
//                anchors.verticalCenter: parent.verticalCenter
            text: qsTr("File suffix:")
            color: "black"
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
            color: "aliceblue"
            border.color: "slategrey"
            radius: 2
            width: 40
            height: 20
//                anchors.verticalCenter: parent.verticalCenter

            TextInput {
                id: suffixInput
                text: "temp"
                anchors.fill: parent
                autoScroll: true
                clip: true
                leftPadding: 2
                rightPadding: 3
                verticalAlignment: Text.AlignVCenter

                onEditingFinished: {
                    if(isValidSuffix(suffixInput.text)) {
                        recordButton.state = "disabled"
                        Transmitter.suffix = suffixInput.text.toString();
                    } else {
                        recordButton.state = "inactive";
                    }
                }

                readOnly: recordButton.state == "enabled"

            }
        }

        Text {
            id: recordingText
//                anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Recording:")
            color: "black"
            verticalAlignment: Text.AlignVCenter
        }

        ColorButton {
            id: recordButton
            fontSize: 12
            textEnabled: qsTr("Stop")
            textDisabled: qsTr("Start")
            width: 50
            height: 20
//                anchors.verticalCenter: parent.verticalCenter
            state: "inactive"

            onStateChanged: {
                if(state == "enabled") {
                    Transmitter.fileDirectory = saveFolderDialog.cleanPath;
                    if(!Transmitter.openFiles()) {
                        console.log("error opening files")
                    }

                }
                else if(state == "disabled") {
                    Transmitter.closeFiles();
                }
            }

        }
    }



    Text {
        id: ping
        text: "--"
    }

    FileDialog {
            id: saveFolderDialog
            selectExisting: false
            selectFolder: true
            folder: shortcuts.desktop
            title: qsTr("Select Data Save Folder")
            sidebarVisible: true
            property string cleanPath: convertToCleanPathMac(saveFolderDialog.folder.toString())

            onAccepted: {
                cleanPath = convertToCleanPathMac(saveFolderDialog.folder.toString());
            }

    }



    function isValidSuffix(inText) {
        var invalidChars = ["/", "<", ">", ":", "\"", "\\", "|", "?", "*"];
        var valid = true;
        for(var i=0; i<invalidChars.length; i++) {
            if(inText.includes(invalidChars[i])) {
                valid = false;
            }
        }
        return valid
    }

    function convertToCleanPathMac(inPath) {
        // remove prefixed "file:///" -- solution from StackOverflow:
        // https://stackoverflow.com/questions/24927850/get-the-path-from-a-qml-url
        inPath = inPath.replace(/^(file:\/{2})/,"");
        // unescape html codes like '%23' for '#'
        return decodeURIComponent(inPath);
    }

    Connections {
        target: Transmitter
        function onAltRangeReceived(timeEsp_ms, range) {
            range = range/1000;
            if(range > yAxisAlt.max) {
                yAxisAlt.max = range*1.05;
            }
            if(range < yAxisAlt.min) {
                yAxisAlt.min = range*1.05;
            }

            xAxisAlt.max = timeEsp_ms/1000.0;
            xAxisAlt.min = timeEsp_ms/1000.0 - 20;

            scatterRangeMeas.append(timeEsp_ms/1000, range);
            scatterRangeMeas.numPoints += 1;

            if(scatterRangeMeas.numPoints > 300) {
//                scatterRangeMeas.remove(0)
                scatterRangeMeas.removePoints(0, 50);
                scatterRangeMeas.numPoints -= 50;
            }

        }
    }

    Connections {
        target: Transmitter
        function onAltStateEstimate(timeEsp_ms, z, z_dot) {
            // Convert frame of reference
            var range = -z;
            var range_dot = -z_dot;
            if(range > yAxisAlt.max) {
                yAxisAlt.max = range*1.05;
            }
            if(range < yAxisAlt.min) {
                yAxisAlt.min = range*1.05;
            }

            if(range_dot > yAxisVelocity.max) {
                yAxisVelocity.max = range_dot*1.05;
            }
            if(range_dot < yAxisVelocity.min) {
                yAxisVelocity.min = range_dot*1.05;
            }

            xAxisAlt.max = timeEsp_ms/1000.0;
            xAxisAlt.min = timeEsp_ms/1000.0 - 20;
            xAxisVelocity.max = timeEsp_ms/1000.0;
            xAxisVelocity.min = timeEsp_ms/1000.0 - 20;

            scatterAltEst.append(timeEsp_ms/1000, range);
            scatterAltEst.numPoints += 1;
            scatterVelEst.append(timeEsp_ms/1000, range_dot);
            scatterVelEst.numPoints += 1;

            if(scatterAltEst.numPoints > 300) {
//                scatterAltEst.remove(0);
                scatterAltEst.removePoints(0, 50);
                scatterAltEst.numPoints -= 50;
            }
            if(scatterVelEst.numPoints > 300) {
//                scatterVelEst.remove(0);
                scatterVelEst.removePoints(0,50);
                scatterVelEst.numPoints -= 50;
            }

        }
    }


    Connections {
        target: Transmitter
        function onAltPropStateEstimate(timeEsp_ms, z, z_dot) {
            // Convert frame of reference
            var range = -z;
            var range_dot = -z_dot;
            if(range > yAxisAlt.max) {
                yAxisAlt.max = range*1.05;
            }
            if(range < yAxisAlt.min) {
                yAxisAlt.min = range*1.05;
            }

            if(range_dot > yAxisVelocity.max) {
                yAxisVelocity.max = range_dot*1.05;
            }
            if(range_dot < yAxisVelocity.min) {
                yAxisVelocity.min = range_dot*1.05;
            }

            xAxisAlt.max = timeEsp_ms/1000.0;
            xAxisAlt.min = timeEsp_ms/1000.0 - 20;
            xAxisVelocity.max = timeEsp_ms/1000.0;
            xAxisVelocity.min = timeEsp_ms/1000.0 - 20;

            scatterAltPropEst.append(timeEsp_ms/1000, range);
            scatterAltPropEst.numPoints += 1;
            scatterVelPropEst.append(timeEsp_ms/1000, range_dot);
            scatterVelPropEst.numPoints += 1;

            if(scatterAltPropEst.numPoints > 300) {
//                scatterAltPropEst.remove(0);
                scatterAltPropEst.removePoints(0,50);
                scatterAltPropEst.numPoints -= 50;
            }
            if(scatterVelPropEst.numPoints > 300) {
//                scatterVelPropEst.remove(0);
                scatterVelPropEst.removePoints(0,50);
                scatterVelPropEst.numPoints -= 50;
            }

        }
    }

    Connections {
        target: Transmitter
        function onPingReceived(pingLoopTime) { ping.text = pingLoopTime; }
    }


    Connections {
        target: Transmitter
        function onLogReceived(dev, logStr, level) {
            var dev_str = "[]";
            if(dev === SecondPage.Devices.ESP) { dev_str = "[ESP]"; }
            else if(dev === SecondPage.Devices.JV) { dev_str = "[JV] "; }

            var lvl_str = "[]";
            if(level === SecondPage.LogLevels.ALL) { lvl_str = "[ALL] "; }
            else if(level === SecondPage.LogLevels.DEBUG) { lvl_str = "[DEBUG] "; }
            else if(level === SecondPage.LogLevels.INFO) { lvl_str = "[INFO] "; }
            else if(level === SecondPage.LogLevels.WARN) { lvl_str = "[WARN] "; }
            else if(level === SecondPage.LogLevels.ERROR) { lvl_str = "[ERROR] "; }
            else if(level === SecondPage.LogLevels.FATAL) { lvl_str = "[FATAL] "; }
            else if(level === SecondPage.LogLevels.OFF) { lvl_str = "[OFF] "; }

            var cat_str = dev_str + lvl_str + logStr + "\n";

            logTextArea.insert(logTextArea.length, cat_str);
        }
    }

}



