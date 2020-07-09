import QtQuick 2.0

Item {
    id: root
    width: 30
    height: 70

    property int value: 0
    property string name: "Chx"

    onValueChanged: {
        barCanvas.requestPaint();
    }

    Column {
        id: column
        anchors.fill: parent
        spacing: 2

        Text {
            id: nameField
            width: parent.width
            height: 15
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 15
            text: root.name
            clip: true

        }

        Text {
            id: valueField
            width: parent.width
            height: 15
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 15
            text: root.value

        }


        Canvas {
            id: barCanvas
            anchors.horizontalCenter: parent.horizontalCenter
            height: parent.height-valueField.height - nameField.height - parent.spacing*2
            width: parent.width/3

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.fillStyle = "green"
                ctx.strokeStyle = "green"
                ctx.lineWidth = 2

//                // draw crosshairs
//                ctx.beginPath()
//                ctx.moveTo(canvas.width/2, 0)
//                ctx.lineTo(canvas.width/2, canvas.height)
//                ctx.moveTo(0, canvas.height/2)
//                ctx.lineTo(canvas.width, canvas.height/2)
//                ctx.stroke()

//                ctx.beginPath()
//                var pxlX = root.valueX/100*canvas.width
//                var pxlY = canvas.height - root.valueY/100*canvas.height
//                ctx.ellipse(pxlX-circleWidth/2, pxlY-circleWidth/2, circleWidth, circleWidth)
//                ctx.fill()
                ctx.beginPath()
                ctx.moveTo(0, height/2)
                ctx.lineTo(width, height/2)
                ctx.stroke()
                var bar_height = -height/2*root.value/100
                ctx.fillRect(0, height/2, width, bar_height)
                ctx.strokeStyle = "black"
                ctx.strokeRect(0, 0, width, height)

            }

        }

    }
}
