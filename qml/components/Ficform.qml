import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Window 2.2
import QtQuick.Layouts 1.1

import "Funcs.js" as Funcs
Rectangle {
    property string delTitle : title
    property string delAuthor : author
    property int delRow : rownum
    property int indexOfThisDelegate: index
    width: 850
    z: {
        var newZ = lvFics.currentIndex === index ? 10 : 1
        //print("changing z for: " + index + " to: " + newZ)
        return lvFics.currentIndex === index ? 10 : 1
    }
    //height: txtUpdated.y + 24
    height:mainLayout.height > (tagColumn.height + genreTagList1.height) ? mainLayout.height + 30 : (tagColumn.height + genreTagList1.height) + 10
    //anchors.bottom: tagColumn.bottom
    //anchors.top: lblTitle.top
    clip: false
    id: delegateItem
    color: "#B0E0E6FF"
    border.width: 2
    radius: 0
    border.color: Qt.rgba(0, 0, 1, 0.4)
    //    Component.onCompleted: {
    //        print("completed " + cbChapters.find(atChapter))
    //        print("molde size " + cbChapters.count)

    //        cbChapters.currentIndex = cbChapters.find(atChapter)
    //    }
    //signal chapterChanged(int chapter)



    ToolButton {
        id: tbAddGenre
        width: 24
        height: 24
        iconSource: "qrc:/icons/icons/add.png"
        enabled:false
        visible:false

    }

    Column {
        id: colControl
        x: 120
        y: 118

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

    }

    Column {
        id: tagColumn
        y: 277
        Text {
            id: txtPublished
            height: 24
            text: qsTr("Pub:") + Qt.formatDate(published, "dd/MM/yyyy")
            font.pixelSize: 12
        }

        Text {
            id: txtUpdated
            height: 24
            z: 1
            text: qsTr("Upd:") + Qt.formatDate(updated, "dd/MM/yyyy")
            font.pixelSize: 12
        }
    }


    function tagToggled(tag, state) {
        if(state)
            lvFics.tagAdded(tag,rownum)
        else
            lvFics.tagDeleted(tag,rownum)
    }
    function tagListActivated(activatedIndex) {
//        print("activation")
//        if(lvFics.previousIndex !== -1)
//            lvFics.children[lvFics.previousIndex].z = 1
//        print("original previndex: " + lvFics.previousIndex )
//        lvFics.previousIndex = activatedIndex
//        print("updated previndex: " + lvFics.previousIndex )
//        console.log(lvFics)
//        lvFics.children[activatedIndex].z = 10
    }

    ColumnLayout {
        id: mainLayout
        anchors.top: parent.top
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: 30
        anchors.right: genreTagList1.left
        anchors.rightMargin: 69

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
            onLinkActivated: Qt.openUrlExternally(link)
        }

        RowLayout {
            id: sumRow

            Row {
                id: rowIndicators

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

            Rectangle {
                id: lvFandoms
                width: 300
                height: 22
                color:delegateItem.color
                anchors.rightMargin: 2
                anchors.bottomMargin: -2

                Text {
                    id: text1
                    text: fandom
                    anchors.fill: parent
                    font.family: "Courier"
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                }
            }

            Text {
                id: txtChapters
                height: 24
                text: qsTr("At chapter:")
                verticalAlignment: Text.AlignVCenter

                font.pixelSize: 12
            }

            ComboBox {
                id: cbChapters
                height: 24
                model: chapters

                onActivated:  {
                    //print("Current text: " + index)
                    lvFics.chapterChanged(index, author, title)
                }
                onModelChanged: {currentIndex = atChapter}

            }

            Text {
                id: txtOf
                height: 24
                text: "Of: " + chapters
                verticalAlignment: Text.AlignVCenter

                font.pixelSize: 12
            }
        }

        Text {
            id: txtSummary
            width: 604
            height: 33
            text: summary
            Layout.fillWidth: true
            Layout.fillHeight: true
            wrapMode: Text.WordWrap
            font.pixelSize: 16
        }

        RowLayout {

            Image {
                id: imgRecommendations
                //property int _MS_PER_DAY: 1000 * 60 * 60 * 24;
                width: recommendations > 0 ? 20 : 0
                height: 24
                visible: recommendations > 0
                source: {
                        return "qrc:/icons/icons/heart.png"
                }
            }
            Text {
                id: txtRecCount
                width: recommendations > 0 ? 20 : 0
                height: 24
                text: { return recommendations}
                visible: recommendations > 0
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 16
            }
            Text {
                id: txtWords
                width: 70
                height: 24
                text: { return "Words:" + words}
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 16
            }

            Text {
                id: txtCharacters
                height: 24
                text: characters
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 16
            }
        }
    }

    GenreTagList {
        id: genreTagList1
        x: 888
        width: 134
        height: 76
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.top: mainLayout.top
        anchors.topMargin: 0
    }
}




