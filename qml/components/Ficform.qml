import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Window 2.2
import QtQuick.Layouts 1.1

import "Funcs.js" as Funcs
Rectangle {
    property string delTitle : title
    property string delAuthor : author
    //property string delFicId: ficId
    property int delRow : rownum
    property bool hasRealGenres : false
    property int indexOfThisDelegate: index
    signal mouseClicked
    MouseArea{
        anchors.fill : parent
        propagateComposedEvents : true

        onClicked : {
            delegateItem.mouseClicked();
        }
    }

    width: 850
    z: {
        var newZ = lvFics.currentIndex === index ? 10 : 1
        //print("changing z for: " + index + " to: " + newZ)
        return lvFics.currentIndex === index ? 10 : 1
    }
    height: mainLayout.height > (tagColumn.height + genreTagList1.height) ? mainLayout.height + 30 : (tagColumn.height + genreTagList1.height) + 10
    clip: false
    id: delegateItem
    color: "#B0E0E6FF"
    border.width: 2
    radius: 0
    border.color: Qt.rgba(0, 0, 1, 0.4)

    ToolButton {
        id: tbAddGenre
        width: 16
        height: 16
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
            visible: {
                var result
                result = updated.getYear() > 70
                //console.debug('year: ' + updated.getYear())
                return result
            }
            text: qsTr("Upd:") + Qt.formatDate(updated, "dd/MM/yyyy")
            font.pixelSize: 12
        }
    }


    function tagToggled(tag, state) {
        if(state)
            lvFics.tagAddedInTagWidget(tag,rownum)
        else
            lvFics.tagDeletedInTagWidget(tag,rownum)
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
        width: 600
        //        anchors.right: genreTagList1.left
        //        anchors.rightMargin: 69
        RowLayout {
            id: rowTitle

            Image {
                id: imgCopy
                width: 24
                height: 24
                verticalAlignment: Text.AlignVCenter
                source: "qrc:/icons/icons/copy.png"
                visible: lvFics.showUrlCopyIcon
                MouseArea{
                    anchors.fill : parent
                    propagateComposedEvents : true

                    onClicked : {
                        lvFics.urlCopyClicked("http://www.fanfiction.net/s/" + url);
                    }
                }
            }
            Image {
                id: imgFindSimilar
                width: 24
                height: 24

                verticalAlignment: Text.AlignVCenter
                source: "qrc:/icons/icons/scan_small.png"
                visible: false
                //visible: lvFics.showScanIcon
                MouseArea{
                    anchors.fill : parent
                    propagateComposedEvents : true
                    onClicked : {
                        lvFics.findSimilarClicked(url);
                    }
                }
            }
            Text {
                id: lblTitle
                width: 517
                height: 21
                textFormat: Text.RichText;
                text: " <html><style>a:link{ color: 	#CD853F33      ;}</style><a href=\"http://www.fanfiction.net/s/" + url + "\">" + title + "</a></body></html>"
                verticalAlignment: Text.AlignVCenter
                style: Text.Raised
                font.pointSize: 16
                font.family: "Verdana"
                font.bold: true
                color: "red"
                onLinkActivated: Qt.openUrlExternally(link)
            }
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

                        var countUpdated = Funcs.dateDiffInDays(updated,new Date, _MS_PER_DAY)
                        var countPublished = Funcs.dateDiffInDays(published,new Date, _MS_PER_DAY)
                        var count = Math.min(countUpdated,countPublished)
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
                    MouseArea{
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            lvFics.fandomToggled(index);
                        }
                    }
                }
            }

            Text {
                id: txtChapters
                height: 24
                text: qsTr("At chapter:")
                verticalAlignment: Text.AlignVCenter

                font.pixelSize: 12
                visible: false
            }

            ComboBox {
                id: cbChapters
                height: 24
                model: chapters

                onActivated:  {
                    lvFics.chapterChanged(index, ID)
                }
                onModelChanged: {currentIndex = atChapter}
                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: true
                    onWheel: {
                        // do nothing
                    }
                    onPressed: {
                        mouse.accepted = false
                    }
                }
                visible: false
            }

            Text {
                id: txtOf
                height: 24
                text: "Of: " + String(parseInt(chapters) - 1)
                //text: "" + String(parseInt(chapters) - 1)
                verticalAlignment: Text.AlignVCenter

                font.pixelSize: 12
                visible: false
            }
        }
        Item{
            height: txtSummary.height
            width: parent.width
            Text {
                id: txtSummary
                text: summary
                width: parent.width
                horizontalAlignment: Text.AlignJustify
                Layout.fillWidth: true
                Layout.fillHeight: true
                wrapMode: Text.WordWrap
                font.pixelSize: 16
            }
        }

        RowLayout {

            Image {
                id: imgRecommendations
                //property int _MS_PER_DAY: 1000 * 60 * 60 * 24;
                width: recommendations > 0 ? 20 : 0
                height: 24
                sourceSize.height: 24
                sourceSize.width: 24
                visible: recommendations > 0
                source: "qrc:/icons/icons/heart.png"
                MouseArea{
                    anchors.fill : parent
                    propagateComposedEvents : true
                    onClicked : {
                        lvFics.recommenderCopyClicked("http://www.fanfiction.net/s/" + url);
                    }
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
            Image {
                id: imgLike
                width: 20
                height: 24
                sourceSize.height: 20
                sourceSize.width: 20
                opacity: ((tags.indexOf("Disliked") === -1 && tags.indexOf("Hide") === -1) || tags.indexOf("Liked")  !== -1) ?  1 : 0.3
                source: tags.indexOf("Liked") !== -1 ? "qrc:/icons/icons/like_green.png" :  "qrc:/icons/icons/like.png"

                MouseArea{
                    anchors.fill : parent
                    propagateComposedEvents : true
                    onClicked : {
                        if(tags.indexOf("Liked") === -1)
                        {
                            lvFics.tagAdded("Liked",indexOfThisDelegate)
                            lvFics.tagDeleted("Disliked",indexOfThisDelegate)
                        }
                        else
                            lvFics.tagDeleted("Liked",indexOfThisDelegate)
                    }
                }
            }
            Image {
                id: imgDislike
                width: 20
                height: 24
                sourceSize.height: 20
                sourceSize.width: 20
                opacity: ((tags.indexOf("Liked") === -1 && tags.indexOf("Hide") === -1) || tags.indexOf("Disliked")  !== -1) ?  1 : 0.3
                source: tags.indexOf("Disliked") !== -1 ? "qrc:/icons/icons/dislike_red.png" :  "qrc:/icons/icons/dislike.png"
                MouseArea{
                    anchors.fill : parent
                    propagateComposedEvents : true
                    onClicked : {
                        if(tags.indexOf("Disliked") === -1)
                        {
                            lvFics.tagAdded("Disliked",indexOfThisDelegate)
                            lvFics.tagDeleted("Liked",indexOfThisDelegate)
                        }
                        else
                            lvFics.tagDeleted("Disliked",indexOfThisDelegate)
                    }
                }
            }
            Image {
                id: imgDiscard
                width: 20
                height: 24
                sourceSize.height: 20
                sourceSize.width: 20
                opacity: ((tags.indexOf("Liked") === -1 && tags.indexOf("Disliked") === -1) || tags.indexOf("Hide")  !== -1) ?  1 : 0.3
                source: tags.indexOf("Hide") !== -1 ? "qrc:/icons/icons/trash_darker.png" :  "qrc:/icons/icons/trash.png"
                MouseArea{
                    anchors.fill : parent
                    propagateComposedEvents : true
                    onClicked : {
                        if(tags.indexOf("Hide") === -1)
                            lvFics.tagAdded("Hide",indexOfThisDelegate)
                        else
                            lvFics.tagDeleted("Hide",indexOfThisDelegate)
                    }
                }
            }
            Text {
                id: txtWords
                width: 70
                height: 24
                text: { return "Words: " + Funcs.thousandSeparator(words)}
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 16
            }

            Item{
                height: txtCharacters.height
                width: parent.width
                Text {
                    id: txtCharacters
                    text: characters
                    width: 350
                    horizontalAlignment: Text.AlignJustify
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    elide: Text.ElideRight
                    font.pixelSize: 16
                }
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




