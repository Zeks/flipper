import QtQuick 2.0

Rectangle{
    id: root
    border.width: 3
    border.color: Qt.lighter(tagSelectorColor)
    width:parent.width
    signal activated(var index)
    signal tagToggled(var tag, var value)
    property string tagName
    property bool active: false
    property var allChoices
    property var currentChoices
    property bool appendVisible: true
    property bool selectionMode: false
    property bool canAdd: false
    property string tagSelectorColor: '#ABB6D3'
    function deactivate(index) {
        root.active = false
        //root.selectionMode = false;
        //list.model = allChoices;
    }
    function deactivateIntoAllChoices(index) {
        root.active = false
        root.selectionMode = false;
        list.model = allChoices;
    }
    function activate(index) {
        root.active = true
    }
    function showCurrrentTags() {
        root.selectionMode = false;
        list.model = currentChoices;
    }

    height: list.visible ? list.height + txtGenre.height + 6 : txtGenre.height + 6
    //height: list.height
    //height:40
    Rectangle{
        id:rect
        //rotation: root.rotation === 180 ? 180 : 0;
        height: 24//txtGenre.height + 6
        width:parent.width
        color: tagSelectorColor
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
                print("Current index: " + delegateItem.indexOfThisDelegate)
                //delegateItem.z = 10
                lvFics.currentIndex = delegateItem.indexOfThisDelegate
                active = true
                activated(delegateItem.indexOfThisDelegate)

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
            visible: canAdd//root.active && root.appendVisible
            width: appendVisible ? 24 : 0
            MouseArea{
                anchors.fill: parent
                onClicked: {
                    console.log(mainWindow.height);
                    //console.log(plus.mapToItem(mainWindow,0,0));

                    lvFics.currentIndex = delegateItem.indexOfThisDelegate
                    root.selectionMode = !root.selectionMode

                    if(root.selectionMode)
                        list.model = allChoices
                    else
                        list.model = currentChoices

                    active = true
                    activated(delegateItem.indexOfThisDelegate)
                    if(plus.mapToItem(mainWindow,0,0).y > mainWindow.height/2 )
                    {
                        //list.verticalLayoutDirection = ListView.BottomToTop;
                        //list.anchors.top = undefined;
                        //list.y = y - list.height;
                        console.log("list y: ", list.y);
                        console.log("list height: ", list.height);
                        root.parent.rotation =  180;
                        rect.rotation = 180;
                    }
                    else
                    {
                        root.parent.rotation =  0;
                        rect.rotation = 0;
                        //list.verticalLayoutDirection = ListView.TopToBottom;
                        //list.anchors.top =  rect.bottom
                    }
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
        //interactive : true
        //verticalLayoutDirection:ListView.BottomToTop
        height: {
            //print("Current count: " + count )
            //var taglimit = count >30 ? 30 : count;
            var taglimit = count;
            return spacing*4 + taglimit * (pixelSize + spacing + 3)
        }
        spacing: 2
        delegate: Rectangle{
            id:frame
            rotation: root.parent.rotation === 180 ? 180 : 0
            color: {
                var color
                if(!selectionMode)
                    color = tagText.text == "" ? "white" : "#F5F5DC10"
                else
                {
                    if(currentChoices.indexOf(modelData) !== -1)
                        color = "#F5F5DC10"
                    else
                        color = "lightGray"
                }
                //print("Color is: " + color)
                return color


            }
            z:2
            border.width: 1
            border.color: Qt.lighter(color)
            radius: 2
            anchors.leftMargin: 2
            anchors.rightMargin: 2
            height: tagText.height
            width: root.width - 8
            x: 3
            Text{
                id:tagText
                text: modelData
                font.pixelSize: list.pixelSize
                width: root.width - 4
                verticalAlignment: Text.AlignVCenter
                z:1
                //horizontalAlignment: Text.AlignHCenter
            }
            MouseArea{
                anchors.fill: parent
                onClicked: {

                    if(!selectionMode)
                        return

                    if(currentChoices.indexOf(modelData) === -1)
                    {
                        tagToggled(modelData, true)
                        root.currentChoices.push(modelData)
                        currentChoices = currentChoices
                    }
                    else
                    {
                        currentChoices = currentChoices.filter(function(e){return e!==modelData})
                        tagToggled(modelData, false)
                    }
                }
//                onWheel: {

//                    var angleDelta = root.parent.rotation === 0 ? -1* wheel.angleDelta.y : wheel.angleDelta.y;

//                    if(angleDelta > 0)
//                    {
//                        list.positionViewAtIndex(list.currentIndex+1,ListView.Beginning )
//                        if(list.currentIndex < list.count)
//                            list.currentIndex=list.currentIndex+1
//                    }
//                    else{
//                        list.positionViewAtIndex(list.currentIndex-1,ListView.Beginning )
//                        if(list.currentIndex >=0)
//                            list.currentIndex=list.currentIndex-1
//                    }
//                }

            }
        }

    }
}
