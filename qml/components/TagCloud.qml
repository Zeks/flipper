import QtQuick 2.0
import QtQuick.Controls 1.4
Rectangle {
    id: tagCloud

    SystemPalette { id: palette }   //we get the system default colors from this

    //public API
    property variant model
    property color baseColor: palette.base
    property color textColor: palette.text
    property int   textFontSize: 16

    color: baseColor

    Flow {
        id: flow
        flow:Flow.LeftToRight
        width: parent.width
        spacing: 5
        anchors.margins: 4
        anchors.verticalCenter: parent.verticalCenter
        property int maxHeight: 0

        Repeater {
            id: repeater
            model: tagCloud.model
            Text {
                id: textBlock
                text: modelData
                font.pointSize: tagCloud.textFontSize;
                onTextChanged: {
                    if(y > flow.maxHeight)
                    {
                        flow.maxHeight = y
                        print("Max:" + flow.maxHeight )
                    }
                }
            }
        }
    }
}
