
import QtQuick 2.12
import QtCharts 2.12
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.0


Item {

    id: page

    ChartView {
        title: "Scatters"
        antialiasing: true
        anchors.top: parent.top
        anchors.left: parent.left
        height: parent.height
        width: parent.width/2
        id: chart1

        ScatterSeries {
            id: scatter1
            name: "Altitude"
            axisX: valueXAxis
            axisY: valueYAxis
            markerSize: 2
            borderColor: "green"
            color: "green"
            borderWidth: 2
            useOpenGL: true
            property int numPoints: 0

        }

        ScatterSeries {
            id: scatter2
            name: "Altitude Est"
            axisX: valueXAxis
            axisY: valueYAxis
            markerSize: 2
            borderColor: "red"
            color: "red"
            borderWidth: 1
            useOpenGL: true
            property int numPoints: 0

        }

        ScatterSeries {
            id: scatter3
            name: "Altitude Fwd Est"
            axisX: valueXAxis
            axisY: valueYAxis
            markerSize: 2
            borderColor: "blue"
            color: "blue"
            borderWidth: 1
            useOpenGL: true
            property int numPoints: 0

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

    ChartView {
        title: "Scatters Velocity"
        antialiasing: true
        anchors.top: parent.top
        anchors.right: parent.right
        height: parent.height
        width: parent.width/2
        id: chartVelocity

        ScatterSeries {
            id: scatterVelEst
            name: "Velocity"
            axisX: xAxisVelocity
            axisY: yAxisVelocity
            markerSize: 4
            borderColor: "black"
            color: "green"
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
            min: -8000
            max: 8000
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
        anchors.top: parent.top
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
                        Transmitter.setSuffix(suffixInput.text.toString())
                        AltitudeController.setSuffix(suffixInput.text.toString())
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
                    Transmitter.setFileDirectory(saveFolderDialog.cleanPath)
                    AltitudeController.setFileDirectory(saveFolderDialog.cleanPath)
                    if(!(Transmitter.openFiles() && AltitudeController.openFiles())) {
                        console.log("error opening files")
                    }

                }
                else if(state == "disabled") {
                    Transmitter.closeFiles();
                    AltitudeController.closeFiles();
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
        function onAltitudeRangeReceived(timestamp, range) {
            if(range > valueYAxis.max) {
                valueYAxis.max = range*1.05;
            }

            valueXAxis.max = timestamp/1000;
            valueXAxis.min = timestamp/1000 - 20;

            scatter1.append(timestamp/1000, range);
            scatter1.numPoints += 1;

            var alt_est = AltitudeController.getAltitudeEstimate();
            scatter2.append(timestamp/1000, alt_est);
            scatter2.numPoints += 1;

            if(scatter1.numPoints > 1000) {
                scatter1.remove(0);
                scatter1.numPoints -= 1;
            }
            if(scatter2.numPoints > 1000) {
                scatter2.remove(0);
                scatter2.numPoints -= 1;
            }
        }
    }

    Connections {
        target: Transmitter
        function onAltitudeForwardEstimate(timestamp, range, velocity) {

            scatter3.append(timestamp/1000, range);
            scatter3.numPoints += 1;

            if(velocity > yAxisVelocity.max) {
                yAxisVelocity.max = velocity*1.05;
            }
            if(velocity < yAxisVelocity.min) {
                yAxisVelocity.min = velocity*1.05;
            }
            xAxisVelocity.max = timestamp/1000;
            xAxisVelocity.min = timestamp/1000 - 20;

            if(timestamp/1000 > valueXAxis.max) {
                valueXAxis.max = timestamp/1000;
                valueXAxis.min = timestamp/1000 - 20;
            }

            scatterVelEst.append(timestamp/1000, velocity);
            scatterVelEst.numPoints += 1;

            if(scatter3.numPoints > 1000) {
                scatter3.remove(0);
                scatter3.numPoints -= 1;
            }

            if(scatterVelEst.numPoints > 1000) {
                scatterVelEst.remove(0);
                scatterVelEst.numPoints -= 1;
            }
        }

    }

    Connections {
        target: Transmitter
        function onPingReceived(pingLoopTime) { ping.text = pingLoopTime; }
    }

}



