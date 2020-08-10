import QtQuick 2.0

Item {

    id: root
    width: 150
    height: 150

    // Channel values
    property int valueX: 0
    property int valueY: 0

    // Secondary channel values
    property int secondaryValueX: 0
    property int secondaryValueY: 0

    // If an addition point is displayed
    property bool secondary: false

    // Width of the point circle
    property alias circleWidth: canvas.circleWidth
    property alias xLabel: horizAxisLabel.text
    property alias yLabel: vertAxisLabel.text

    onValueXChanged: {
        canvas.requestPaint();
    }

    onValueYChanged: {
        canvas.requestPaint();
    }

    onSecondaryValueXChanged: {
        canvas.requestPaint();
    }

    onSecondaryValueYChanged: {
        canvas.requestPaint();
    }

    Rectangle {
        id: rectangle
        width: parent.width - vertText.width
        height: parent.height - horizText.height
        anchors.right: parent.right
        anchors.top: parent.top
        color: "aliceblue"
        border.color: "slategrey"
        border.width: 2

        Canvas {
            id: canvas
            anchors.fill: parent

            // Width of the point circle
            property int circleWidth: 8

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.fillStyle = "black"
                ctx.strokeStyle = "black"
                ctx.lineWidth = 1

                // draw crosshairs
                ctx.beginPath()
                ctx.moveTo(canvas.width/2, 0)
                ctx.lineTo(canvas.width/2, canvas.height)
                ctx.moveTo(0, canvas.height/2)
                ctx.lineTo(canvas.width, canvas.height/2)
                ctx.stroke()

                // Draw secondary channel circle
                if(root.secondary) {
                    ctx.fillStyle = "seagreen"
                    ctx.strokeStyle = "darkgrey"
                    ctx.beginPath()
                    var pxlX2 = root.secondaryValueX/100*canvas.width/2 + canvas.width/2
                    var pxlY2 = canvas.height/2 - root.secondaryValueY/100*canvas.height/2
                    ctx.ellipse(pxlX2-circleWidth/2-1, pxlY2-circleWidth/2-1, circleWidth+1, circleWidth+1)
                    ctx.fill()
                    ctx.fillStyle = "black"
                    ctx.strokeStyle = "black"
                }

                // Draw primary channel dot
                ctx.beginPath()
                var pxlX = root.valueX/100*canvas.width/2 + canvas.width/2
                var pxlY = canvas.height/2 - root.valueY/100*canvas.height/2
                ctx.ellipse(pxlX-circleWidth/2, pxlY-circleWidth/2, circleWidth, circleWidth)
                ctx.fill()

            }

        }

    }

    Text {
        id: vertText
        width: 30
        height: 15
        anchors.verticalCenter: rectangle.verticalCenter
        anchors.left: parent.left
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 15
        text: root.valueY

    }

    Text {
        id: vertAxisLabel
        width: 30
        height: 15
        anchors.bottom: vertText.top
        anchors.bottomMargin: 2
        anchors.left: parent.left
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 15
        text: "chx"

    }

    Text {
        id: horizText
        width: 30
        height: 15
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: rectangle.horizontalCenter
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 15
        text: root.valueX
    }

    Text {
        id: horizAxisLabel
        width: 30
        height: 15
        anchors.bottom: parent.bottom
        anchors.right: horizText.left
        anchors.rightMargin: 2
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 15
        text: "chx"

    }

}
