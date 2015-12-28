import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Window 2.2

import "Funcs.js" as Funcs
Rectangle {
    property string delTitle : title
    property string delAuthor : author
    property int delRow : rownum
    width: 800
    height: txtUpdated.y + 24
    //anchors.bottom: txtUpdated.bottom
    clip: false
    id: delegateItem
    color: "#B0E0E6FF"
    border.width: 2
    radius: 4
    border.color: Qt.rgba(0, 0, 1, 0.4)
//    Component.onCompleted: {
//        print("completed " + cbChapters.find(atChapter))
//        print("molde size " + cbChapters.count)

//        cbChapters.currentIndex = cbChapters.find(atChapter)
//    }
    //signal chapterChanged(int chapter)
    Text {
        id: txtSummary
        text: summary
        wrapMode: Text.WordWrap
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.left: colControl.right
        anchors.leftMargin: 8
        anchors.top: lvFandoms.bottom
        anchors.topMargin: 2
        font.pixelSize: 16
    }

    ComboBox {
        id: cbChapters
        height: 24
        anchors.right: txtChapters.left
        anchors.rightMargin: -164
        anchors.left: txtChapters.right
        anchors.leftMargin: 6
        anchors.top: txtSummary.bottom
        anchors.topMargin: 12
        model: chapters

        onActivated:  {
            //print("Current text: " + index)
            lvFics.chapterChanged(index, author, title)
        }
        onModelChanged: {currentIndex = atChapter}

    }

    Text {
        id: txtChapters
        height: 24
        text: qsTr("At chapter:")
        verticalAlignment: Text.AlignVCenter
        anchors.left: lvGenres.right
        anchors.leftMargin: 16
        anchors.top: txtSummary.bottom
        anchors.topMargin: 13
        font.pixelSize: 12
    }

    Text {
        id: txtOf
        height: 24
        text: "Of: " + chapters
        verticalAlignment: Text.AlignVCenter
        anchors.rightMargin: -6
        anchors.leftMargin: 6
        anchors.topMargin: 12
        anchors.right: txtSummary.right
        anchors.left: cbChapters.right
        anchors.top: txtSummary.bottom
        font.pixelSize: 12
    }

    Rectangle{
        id: lvGenres
        width: 294
        height: 24
        anchors.left: txtSummary.left
        anchors.leftMargin: 2
        anchors.top: txtSummary.bottom
        anchors.topMargin: 10
        color:delegateItem.color
        HighlightedListview {
            id: lvGenreInternal
            height: 24
            tagModelInternal: genre
            //orientation: ListView.Horizontal
            spacing: 5

        }
    }

    Text {
        id: lblTitle
        width: 517
        height: 21
        textFormat: Text.RichText;
        text: " <html><style>a:link{ color: 	#CD853F33      ;}</style><a href=\"http://www.fanfiction.net/" + url + "\">" + title + "</a></body></html>"
        verticalAlignment: Text.AlignVCenter
        style: Text.Raised
        font.pointSize: 16
        font.family: "Verdana"
        font.bold: true
        color: "red"
        anchors.left: colControl.right
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 10
        onLinkActivated: Qt.openUrlExternally(link)
    }

    Flickable{
        id: lvTags
        height: 24
        anchors.right: parent.right
        anchors.rightMargin: 70
        anchors.left: txtSummary.left
        anchors.leftMargin: 2
        anchors.top: lvGenres.bottom
        anchors.topMargin: 17
        contentHeight: lvTagFlow.height


        Rectangle{
            //color: "#FFE6D1CC"
            //border.width: 1
            //border.color: "gray"
            color:delegateItem.color
            anchors.fill: parent
            radius: 3

        HighlightedListview {
            tagModelInternal:tags
            useDefaultColor: false
            allChoices: tagModel
            modelChoices: tags
            id:lvTagFlow
            spacing: 5
            width:lvTags.width
        }
        }
        states: [
                State {
                    name: "tagSelectionMode"
                    PropertyChanges { target: lvTagFlow; tagModelInternal: tagModel }
                    PropertyChanges { target: lvTagFlow; highlightTags: true}
                    PropertyChanges { target: lvTags; contentHeight: lvTagFlow.height }
                    PropertyChanges { target: delegateItem ; z: 8 }
                    PropertyChanges { target: txtUpdated; visible: false }
                    PropertyChanges { target: txtPublished; visible: false }
                },
                State {
                name: "baseMode"
                PropertyChanges { target: lvTagFlow; tagModelInternal:{
                        print(modelChoices)
                        return modelChoices}}
                PropertyChanges { target: lvTags; height: 24; }
                PropertyChanges { target: lvTagFlow; highlightTags: false}
                PropertyChanges { target: delegateItem; z: 1 }
                PropertyChanges { target: txtUpdated; visible: true}
                PropertyChanges { target: txtPublished; visible: true}
            }
            ]
    }



    ToolButton {
        id: tbAddGenre
        width: 24
        height: 24
        anchors.top: txtSummary.bottom
        anchors.topMargin: 10
        anchors.left: colControl.left
        anchors.leftMargin: 11
        iconSource: "qrc:/icons/icons/add.png"
        enabled:false
        visible:false

    }

    ToolButton {
        id: tbAddTag

        width: 24
        height: 24
        anchors.top: tbAddGenre.bottom
        anchors.topMargin: 17
        anchors.left: colControl.left
        anchors.leftMargin: 11
        iconSource: "qrc:/icons/icons/add.png"

        MouseArea{
            anchors.fill: parent
         onClicked:{
             var newState = lvTags.state === "tagSelectionMode" ? "baseMode": "tagSelectionMode"
             lvTags.state = newState
        }
    }
    }

    Rectangle {
        id: lvFandoms
        width: 331
        height: 22
        color:delegateItem.color
        anchors.top: lblTitle.bottom
        anchors.topMargin: 8
        anchors.rightMargin: 2
        anchors.bottomMargin: -2
        anchors.left: parent.left
        anchors.leftMargin: 120

        Text {
            id: text1
            text: fandom
            font.family: "Courier"
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
            font.pixelSize: 13
        }
    }

    Text {
        id: txtPublished
        x: 421
        height: 24
        text: qsTr("Pub:") + Qt.formatDate(published, "dd/MM/yyyy")
        anchors.right: lvTags.right
        anchors.rightMargin: 0
        anchors.top: cbChapters.top
        anchors.topMargin: 76
        font.pixelSize: 12
    }

    Text {
        id: txtUpdated
        height: 24
        z: 1
        text: qsTr("Upd:") + Qt.formatDate(updated, "dd/MM/yyyy")
        anchors.top: cbChapters.top
        anchors.topMargin: 76
        anchors.left: lvTags.left
        anchors.leftMargin: 0
        font.pixelSize: 12
    }

    Text {
        id: txtWords
        x: 525
        width: 70
        height: 24
        text: "Words:" + words
        verticalAlignment: Text.AlignVCenter
        anchors.right: parent.right
        anchors.rightMargin: 50
        anchors.top: lblTitle.bottom
        anchors.topMargin: 7
        font.pixelSize: 16
    }

    Column {
        id: colControl
        anchors.left: parent.left
        anchors.leftMargin: 5
        anchors.top: parent.top
        anchors.topMargin: 30

        ToolButton {
            id: tbTrack
            enabled:false
        }

        ToolButton {
            id: tbHide
            enabled:false
        }

        ToolButton {
            id: tbUpdate
            enabled:false
        }
    }

    Row {
        id: rowIndicators
        anchors.top: lvFandoms.top
        anchors.topMargin: 0
        anchors.left: lblTitle.left
        anchors.leftMargin: 0

        Label {
            id: image2
            width: 24
            height: 24
            color: "#000000"
            text: rated
            verticalAlignment: Text.AlignVCenter
            style: Text.Raised
            font.bold: true
            font.pointSize: 14
            font.family: "Verdana"
        }

        Image {
            id: imgUpdateState
            property int _MS_PER_DAY: 1000 * 60 * 60 * 24;
            width: 24
            height: 24
            source: {
                
                var count = Funcs.dateDiffInDays(updated,new Date, _MS_PER_DAY)
                if(count < 30)
                    return "qrc:/icons/icons/updating_fresh.png"
                else if(count < 365)
                    return "qrc:/icons/icons/updating_normal.png"
                else
                    return "qrc:/icons/icons/updating_old.png"

            }
        }


        Image {
            id: imgFinished
            width: 24
            height: 24
            source: { if(complete == 1)
                        return "qrc:/icons/icons/ok.png"
                      return "qrc:/icons/icons/ok_grayed.png"
            }
        }
    }

    Window {
        id: splash
        title: "Splash Window"
        modality: Qt.ApplicationModal
        flags: Qt.FramelessWindowHint
        width:300
        height:flow.height

        property int timeoutInterval: 2000
//        Flickable{
//            anchors.fill: parent
        Flow{
            anchors.left: parent.left
            anchors.right: parent.right
            spacing : 5
            id:flow
            width:300
            flow:Flow.LeftToRight

            Repeater{
        model:200
        Text {
            id: textBlock
            text: modelData + " "
            font.pointSize: 12;
        //}
        }
        }
        }
//        ScrollView{
//            verticalScrollBarPolicy: Qt.ScrollBarAsNeeded
//            anchors.fill: parent
////            ListView{
////                model:1000
////                spacing: 1
////                anchors.fill: parent

////                delegate:Text {
////                    id: name
////                    text: modelData
////                }
////            }
////            TagCloud{
////                model:tagModel
////                anchors.fill: parent
////            }

//            }
        }
    }




