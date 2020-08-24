import QtQuick 2.0
import QtQuick.Controls 2.15

Item {

    id: root

    enum LogLevels { ALL, DEBUG, INFO, WARN, ERROR, FATAL, OFF }
    enum Devices { ESP, JV }

    ScrollView {
        anchors.fill: parent
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
