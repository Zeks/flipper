import QtQuick 2.0

Item{
    id: root
    width:parent.width
    signal activated()
    signal tagToggled(var tag, var value)
    property string tagName
    property bool active: false
    property var allChoices
    property var currentChoices
    property bool appendVisible: true
    property bool selectionMode: false
    function deactivate() {
        root.active = false
    }
    height: list.visible ? list.height + txtGenre.height + 6 : txtGenre.height + 6
    //height: list.height
    //height:40
    Rectangle{
        id:rect
        height: 24//txtGenre.height + 6
        width:parent.width
        color: "brown"
        Text{
            id:txtGenre
            width:parent.width - plus.width
            anchors.top: parent.top
            anchors.topMargin: (rect.height - height) / 2
            text: root.tagName
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
        MouseArea{
            anchors.fill: parent
            onClicked: {
                //print("activated")
                active = true
                activated()
            }
        }
        Image{
            id:plus
            height:rect.height
            source: "qrc:/icons/icons/add.png"
            anchors.left: txtGenre.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: 3
            visible:root.active && root.appendVisible
            width: appendVisible ? 24 : 0
            MouseArea{
                anchors.fill: parent
                onClicked: {
                    root.selectionMode = !root.selectionMode
                    if(root.selectionMode)
                        list.model = allChoices
                    else
                        list.model = currentChoices
                }
            }
        }
    }
    ListView
    {
        id: list
        visible:root.active
        anchors.top: rect.bottom
        anchors.topMargin: 2
        property int pixelSize : 16
        model:currentChoices
        height: {
            //print("Current count: " + count )
            return count * (pixelSize + 4 + spacing)
        }
        spacing: 2
        delegate: Rectangle{
            id:frame
            color: {
                var color
                if(!selectionMode)
                    color = tagText.text == "" ? "white" : "#F5F5DC10"
                else
                {
                    if(currentChoices.indexOf(modelData) !== -1)
                        color = "#F5F5DC10"
                    else
                        color = "white"
                }
                //print("Color is: " + color)
                return color

            }
            border.width: 1
            border.color: Qt.lighter("green")
            radius: 2
            height: tagText.height
            width: root.width
            Text{
                id:tagText
                text: modelData
                font.pixelSize: list.pixelSize
            }
            MouseArea{
                anchors.fill: parent
                onClicked: {
                    if(!selectionMode)
                        return

                    if(currentChoices.indexOf(modelData) === -1)
                    {
                        currentChoices.push(modelData)
                        tagToggled(modelData, true)
                    }
                    else
                    {
                        currentChoices = currentChoices.filter(function(e){return e!==modelData})
                        tagToggled(modelData, false)
                    }
                }
            }
        }

    }
}
