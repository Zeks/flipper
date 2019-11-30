/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
import QtQuick 2.5
import QtQuick.Controls 2.4
import QtQuick.Window 2.2
import QtQuick.Layouts 1.11

Rectangle{
    id: root
    border.width: 3
    border.color: Qt.lighter(tagSelectorColor)
    width:parent.width
    signal activated(var index)
    signal tagToggled(var tag, var value)
    property string tagName
    property string tooltip
    property string plusTooltip
    property bool active: false
    property var allChoices
    //property string minSlashLevel
    property var currentChoices
    property bool appendVisible: true
    property bool selectionMode: false
    property bool canAdd: false
    property string tagSelectorColor: '#ABB6D3'
    function actOnAFic(ficIndex, ficUrl) {
        //console.log("acting on a fic")
        mainWindow.selectedIndex = ficIndex
        mainWindow.selectedUrl = ficUrl
        mainWindow.actionTakenSinceNavigation = true
    }
    function deactivate(index) {
        console.log("deactivating" + tagName);
        root.active = false
        //rect.rotation = root.rotation != 0 ? 180 : 0
        //root.selectionMode = false;
        //list.model = allChoices;
    }
    function restore() {
        rect.rotation = 0
        //root.selectionMode = false;
        //list.model = allChoices;
    }
    function deactivateIntoAllChoices(index) {
        console.log("deactivating" + tagName);
        root.active = false
        root.selectionMode = false;
        list.model = allChoices;
    }
    function activate(index) {
        console.log("activating" + tagName);
        root.active = true
    }
    function showCurrrentTags() {
        console.log("showing " + tagName);
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
        Image{
            id:slashIndicator
            height:rect.height
            visible: tagName === "Genre" && minSlashLevel > 0
            source: {
                //console.log("minSlashLevel is", minSlashLevel);
                if(minSlashLevel == 3)
                    return "qrc:/icons/icons/slash_uncertain.png"
                if(minSlashLevel == 2)
                    return "qrc:/icons/icons/slash_simple.png"
                else
                    return "qrc:/icons/icons/slash_certain.png"
            }
            width: appendVisible ? 24 : 0
        }
        Text{
            id:txtGenre
            width:parent.width - plus.width
            anchors.top: parent.top
            anchors.topMargin: (rect.height - height) / 2
            text: root.tagName
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                ToolTip.delay: 1000
                ToolTip.visible: containsMouse
                ToolTip.text: tooltip

                onDoubleClicked: {
                    lvFics.detailedGenreMode = !lvFics.detailedGenreMode
                }
                onClicked: {
                    actOnAFic(indexOfThisDelegate, url)
                    print("Current index: " + delegateItem.indexOfThisDelegate)
                    lvFics.currentIndex = delegateItem.indexOfThisDelegate
                    active = true
                    activated(delegateItem.indexOfThisDelegate)
                    root.parent.rotation =  0;
                    rect.rotation = 0;
                }
            }
        }

        Image{
            id:plus
            height:rect.height
            source: {
                return "qrc:/icons/icons/add.png"
            }
            anchors.left: txtGenre.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: 3
            visible: canAdd
            width: appendVisible ? 24 : 0
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                ToolTip.delay: 1000
                ToolTip.visible: containsMouse
                ToolTip.text: plusTooltip
                onClicked: {
                    actOnAFic(indexOfThisDelegate, url)
                    if(canAdd)
                    {
                        console.log(" height  is" );
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
                        if(list.model === currentChoices)
                        {
                            root.parent.rotation =  0;
                        }
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

            Image {
                id: imgMagnetGrab
                width: 20
                height: 20
                visible: tagName !== "Genre"
                z:120
                anchors.right: frame.right
                sourceSize.height: 20
                sourceSize.width: 20
                opacity: tags.indexOf(mainWindow.magnetTag) !== -1  ? 1 : 0.5
                source: {
                    return mainWindow.magnetTag === modelData ? "qrc:/icons/icons/magnet.png" : "qrc:/icons/icons/magnet_gray.png"
                }

                MouseArea{
                    hoverEnabled: true
                    anchors.fill : parent
                    propagateComposedEvents : false
                    onClicked : {
                        actOnAFic(indexOfThisDelegate, url)
                        mainWindow.magnetTag = modelData
                    }
                }
            }
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
            Row{
                id: rrr
                spacing: 4
                width: root.width - 4 - 24
                Rectangle{
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width:1
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
                        return color
                    }

                }
                Image{
                    anchors.bottomMargin: 4
                    anchors.bottom: rrr.bottom
                    id:genreRelevance
                    visible: {
                        if(!lvFics.detailedGenreMode || !delegateItem.hasRealGenres)
                            return false;
                        return tagName === "Genre";
                    }
                    height: 12
                    width: 12
                    source: {
                        var img = "qrc:/icons/icons/bit2.png";
                        var cut = modelData.substring(0,3);
                        if(cut === "#c#")
                            img = "qrc:/icons/icons/full.png"
                        if(cut === "#p#")
                            img = "qrc:/icons/icons/partial.png"
                        if(cut === "#b#")
                            img = "qrc:/icons/icons/bit2.png"
                        return img;
                    }
                }
                Text{
                    id:tagText
                    text: {
                        var realText;
                        if(tagName === "Genre")
                        {
                            if(lvFics.detailedGenreMode && delegateItem.hasRealGenres)
                                realText = modelData.substring(3)
                            else
                                realText = modelData
                        }
                        else
                            realText = modelData

                        return realText;
                    }
                    font.pixelSize: list.pixelSize
                    //z:1
                    //horizontalAlignment: Text.AlignHCenter
                }

            }
            MouseArea{
                anchors.fill: parent
                onClicked: {
                    actOnAFic(indexOfThisDelegate, url)
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

            }
        }

    }
}
