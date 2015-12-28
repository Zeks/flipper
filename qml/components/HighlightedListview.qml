import QtQuick 2.4
import QtQuick.Controls 1.4
import "qrc:Funcs.js" as Funcs


Flow{
    id:lvCharactersInternal
    //anchors.fill: parent
    width: parent.width
    //height:24

    property var tagModelInternal
    property var allChoices
    property var modelChoices
    property bool highlightTags : false
    property bool useDefaultColor : true

    Repeater
    {
        id:tagRepeater
        model:tagModelInternal
        Rectangle{
            id:host
            radius: 7
        height: 24
        color: {
            if(useDefaultColor)
                return "#D8BFD8AA"
            var inAlltags = modelChoices.indexOf(modelData)
            return useDefaultColor || !highlightTags || (inAlltags !== -1) ? "#D8BFD8AA" : "#F5F5F5FF"
        }
        border.color: Qt.darker(color)
        border.width: 2

        width: txtText.width + 6
        Text{
        id:txtText
            text:modelData
            //anchors.fill: parent
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
            height:parent.height
            anchors.left: host.left
            anchors.leftMargin: {
                return (host.width - width)/2
            }
           }

        MouseArea {
            id: maCharacters
            z: 1
            hoverEnabled: false
            anchors.fill: parent
            onClicked: {
                if(!highlightTags)
                    return
                if(modelChoices.indexOf(modelData) === -1)
                {

                    lvFics.tagAdded(modelData, delegateItem.delRow)

                    if(lvCharactersInternal.modelChoices.length === 0)
                    {
                        print("nothing in tags, adding")
                        modelChoices = [modelData]
                    }
                    else
                    {
                        print("array is not empty, adding")
                        modelChoices.push(modelData)
                    }
                    print(modelChoices)

                    host.color = Qt.rgba(0.0, 1, 0, 0.3)
                }
                else
                {
                    lvFics.tagDeleted(modelData, delegateItem.delRow)
                    lvCharactersInternal.modelChoices = modelChoices.filter(function(e){return e!==modelData})
                    console.log(lvCharactersInternal.modelChoices)
                    host.color = Qt.rgba(0.0, 0, 1, 0.3)
                    //tagModelInternal=tagModelInternal.filter(function(e){return e!==modelData})
                }
            }
        }
    }
    }

}
