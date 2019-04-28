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
import QtCharts 2.0
import QtGraphicalEffects 1.0

import "Funcs.js" as Funcs
Rectangle{
    id: delegateItem
    width: 850
    z: {
        var newZ = lvFics.currentIndex === index ? 10 : 1
        //print("changing z for: " + index + " to: " + newZ)
        return lvFics.currentIndex === index ? 10 : 1
    }
    height: {
        var height = ficSheet.height
        if(snoozeExpired)
            return height;

        if((tags.indexOf("Snoozed") !== -1) && mainWindow.displaySnoozed)
            height = height + snoozePart.height
        if(rectNotes.active)
            height = height + 200

        return height
    }
    property string delTitle : title
    property string delAuthor : author
    //property string delFicId: ficId
    property int delRow : rownum
    property bool hasRealGenres : false
    property bool displayNotes: false
    property int indexOfThisDelegate: index
    property bool snoozed: tags.indexOf("Snoozed") !== -1
    signal mouseClicked
    clip: false



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
    function getColor(index) {
        switch (index) {
        case 4 :
            return "blue"
        case 3:
            return "green"
        case 2:
            return "yellow";
        case 1:
            return "orange";
        case 0:
            return "red";
        }
    }

    Rectangle {
        id: ficSheet
        MouseArea{
            anchors.fill : parent
            propagateComposedEvents : true
            onClicked : {
                delegateItem.mouseClicked();
            }
        }

        width: parent.width
        z: parent.z
        height: mainLayout.height > (tagColumn.height + genreTagList1.height) ? mainLayout.height + 30 : (tagColumn.height + genreTagList1.height) + 10


        color: {
            var color;
            color = purged == 0 ?  "#e4f4f6FF" : "#B0FFE6FF"
            if(snoozeExpired)
                color = "#1100FF00"
            return color;
        }

        radius: 0
        border.width: 2
        border.color: Qt.rgba(0, 0, 1, 0.4)

        ToolButton {
            id: tbAddGenre
            width: 16
            height: 16
            //icon: "qrc:/icons/icons/add.png"
            enabled:false
            visible:false
            Image {
                id: imgGenreAdd
                fillMode: Image.PreserveAspectFit
                anchors.centerIn: parent
                sourceSize.height: tbAddGenre.background.height - 6
                height: sourceSize.height
                source: "qrc:/icons/icons/add.png"
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
                        id: maCopy
                        anchors.fill : parent
                        propagateComposedEvents : true
                        hoverEnabled :true
                        onClicked : {
                            lvFics.urlCopyClicked("http://www.fanfiction.net/s/" + url);
                        }
                        onEntered: {
                            console.log("Entered copy image")
                            lvFics.newQRSource(indexOfThisDelegate);

                            var point = imgCopy.mapToGlobal(maCopy.mouseX,maCopy.mouseY)
                            var mappedPoint = mainWindow.mapFromGlobal(point.x,point.y)

                            if(imgCopy.mapToItem(mainWindow,0,0).y > mainWindow.height/2)
                            {
                                mappedPoint.y = mappedPoint.y - 300
                            }
                            imgQRCode.x = mappedPoint.x + 20
                            imgQRCode.y = mappedPoint.y

                            mainWindow.qrDisplay = true
                        }
                        onExited: {
                            mainWindow.qrDisplay = false
                        }
                    }
                }
                Image {
                    id: imgFindSimilar
                    width: 24
                    height: 24

                    verticalAlignment: Text.AlignVCenter
                    source: "qrc:/icons/icons/scan_small.png"
                    visible: true
                    //visible: lvFics.showScanIcon
                    MouseArea{
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            lvFics.findSimilarClicked(url);
                        }
                    }
                }
                Image {
                    id: imgMagnify2
                    width: 24;
                    visible: author_id > 0 && !lvFics.displayAuthorName;
                    height: 24
                    source: lvFics.authorFilterActive === true ? "qrc:/icons/icons/magnify_res_canc.png" : "qrc:/icons/icons/magnify_res.png"
                    MouseArea{
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(lvFics.authorFilterActive === false)
                            {
                                lvFics.authorFilterActive = true;
                                lvFics.authorToggled(index, true);
                            }
                            else
                            {
                                lvFics.authorFilterActive = false;
                                lvFics.authorToggled(index, false);
                            }

                        }
                    }
                }
                Text {
                    id: lblTitle
                    width: 517
                    height: 21
                    textFormat: Text.RichText;
                    text: " <html><style>a:link{ color: 	#CD853F33      ;}</style><a href=\"http://www.fanfiction.net/s/" + url + "\">" + indexOfThisDelegate + "."  + title + "</a></body></html>"
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
                    color:ficSheet.color
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
            }
            RowLayout{
                Image {
                    id: imgMagnify
                    width: 24;
                    visible: lvFics.displayAuthorName;
                    height: 24
                    source:{
                        if(author_id > 0 && lvFics.displayAuthorName)
                            return lvFics.authorFilterActive === true ? "qrc:/icons/icons/magnify_res_canc.png" : "qrc:/icons/icons/magnify_res.png"
                        else
                            return "qrc:/icons/icons/magnify_inactive.png"
                    }
                    MouseArea{
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(lvFics.authorFilterActive === false)
                            {
                                lvFics.authorFilterActive = true;
                                lvFics.authorToggled(index, true);
                            }
                            else
                            {
                                lvFics.authorFilterActive = false;
                                lvFics.authorToggled(index, false);
                            }

                        }
                    }
                }
                Text {
                    id: txtAuthor
                    width: 517
                    height: 21
                    visible: lvFics.displayAuthorName
                    textFormat: Text.RichText;
                    text: " <html><style>a:link{ color: 	#99853F33      ;}</style><a href=\"http://www.fanfiction.net/u/" +  author_id.toString() + "\">" +"By: " + author + "</a></body></html>"
                    verticalAlignment: Text.AlignVCenter
                    style: Text.Raised
                    font.pointSize: 12
                    font.family: "Verdana"
                    font.bold: false
                    onLinkActivated: Qt.openUrlExternally(link)
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
                    width: recommendations > 0 ? 20 : 0
                    height: 24
                    sourceSize.height: 24
                    sourceSize.width: 24
                    property bool chartVisible: false
                    visible: recommendations > 0
                    source: {
                        if(likedAuthor > 0)
                            return "qrc:/icons/icons/heart_half.png"
                        else
                            return "qrc:/icons/icons/heart.png"
                    }
                    MouseArea{
                        id: maRecs
                        hoverEnabled :true
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            lvFics.recommenderCopyClicked("http://www.fanfiction.net/s/" + url);
                            console.log("Clicked heart icon")
                        }
                        onDoubleClicked: {
                            lvFics.heartDoubleClicked(indexOfThisDelegate)
                        }

                        onEntered: {

                            mainWindow.chartValueCommon = parseInt(roleBreakdown[0],10)
                            mainWindow.chartValueUncommon = parseInt(roleBreakdown[1],10)
                            mainWindow.chartValueRare= parseInt(roleBreakdown[2],10)
                            mainWindow.chartValueUnique= parseInt(roleBreakdown[3],10)

                            mainWindow.chartValueCountCommon = roleBreakdownCount[0]
                            mainWindow.chartValueCountUncommon = roleBreakdownCount[1]
                            mainWindow.chartValueCountRare= roleBreakdownCount[2]
                            mainWindow.chartValueCountUnique= roleBreakdownCount[3]
                            //                        console.log(maRecs.mouseX)
                            //                        console.log(maRecs.mouseY)
                            //console.log(roleBreakdown)
                            var point = imgRecommendations.mapToGlobal(maRecs.mouseX,maRecs.mouseY)
                            var mappedPoint = mainWindow.mapFromGlobal(point.x,point.y)

                            //console.log("Mapped Y: ", imgRecommendations.mapToItem(mainWindow,0,0).y)
                            if(imgRecommendations.mapToItem(mainWindow,0,0).y > mainWindow.height/2)
                            {
                                mappedPoint.y = mappedPoint.y - 300
                            }
                            chartVotes.x = mappedPoint.x
                            chartVotes.y = mappedPoint.y

                            mainWindow.chartDisplay = true

                        }
                        onExited: {
                            mainWindow.chartDisplay = false
                        }
                    }
                }

                Text {
                    id: txtRecCount
                    width: recommendations > 0 ? 20 : 0
                    height: 24
                    text: { return recommendations + " Ratio: 1/" + Math.round(favourites/recommendations)}
                    //text: { return recommendations + " Ratio: 1/" + chapters}
                    //text: { return recommendations + " Faves: " + favourites}
                    visible: recommendations > 0
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 16
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

            RowLayout{
                Text{
                    id: txtTagsExpanations
                    text: "Tags:"
                    visible:false
                    Layout.fillHeight: true
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
                        hoverEnabled: true
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: qsTr("Tag: Liked")
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
                        hoverEnabled: true
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: qsTr("Tag: Disliked")
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
                    id: imgDead
                    width: 20
                    height: 24
                    sourceSize.height: 20
                    sourceSize.width: 20
                    opacity:  0.5
                    source: tags.indexOf("Limbo") !== -1 ? "qrc:/icons/icons/ghost.png" : "qrc:/icons/icons/ghost_gray.png"

                    MouseArea{
                        hoverEnabled: true
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: qsTr("Tag: Limbo")
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(tags.indexOf("Limbo") === -1)
                                lvFics.tagAdded("Limbo",indexOfThisDelegate)
                            else
                                lvFics.tagDeleted("Limbo",indexOfThisDelegate)
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
                        hoverEnabled: true
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: qsTr("Tag: Hide")
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
                Image {
                    id: imgMeh
                    width: 20
                    height: 24
                    sourceSize.height: 24
                    sourceSize.width: 24
                    opacity: tags.indexOf("Meh") !== -1  ? 1 : 0.7
                    source: tags.indexOf("Meh") !== -1 ? "qrc:/icons/icons/lock_yellow.png" : "qrc:/icons/icons/lock_white.png"

                    MouseArea{
                        hoverEnabled: true
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: qsTr("Tag: Meh")
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(tags.indexOf("Meh") === -1)
                                lvFics.tagAdded("Meh",indexOfThisDelegate)
                            else
                                lvFics.tagDeleted("Meh",indexOfThisDelegate)
                        }
                    }
                }

                Image {
                    id: imgWait
                    width: 20
                    height: 24
                    sourceSize.height: 20
                    sourceSize.width: 20
                    opacity: tags.indexOf("Wait") !== -1  ? 1 : 0.5
                    source: tags.indexOf("Wait") !== -1 ? "qrc:/icons/icons/clock_green.png" : "qrc:/icons/icons/clock_gray.png"

                    MouseArea{
                        hoverEnabled: true
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: qsTr("Tag: Wait")
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(tags.indexOf("Wait") === -1)
                                lvFics.tagAdded("Wait",indexOfThisDelegate)
                            else
                                lvFics.tagDeleted("Wait",indexOfThisDelegate)
                        }
                    }
                }
                Image {
                    id: imgStem
                    width: 20
                    height: 24
                    sourceSize.height: 20
                    sourceSize.width: 20
                    opacity: tags.indexOf("Stem") !== -1  ? 1 : 0.5
                    source: tags.indexOf("Stem") !== -1 ? "qrc:/icons/icons/stem_green.png" : "qrc:/icons/icons/stem_gray.png"

                    MouseArea{
                        hoverEnabled: true
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: qsTr("Tag: Stem")
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(tags.indexOf("Stem") === -1)
                                lvFics.tagAdded("Stem",indexOfThisDelegate)
                            else
                                lvFics.tagDeleted("Stem",indexOfThisDelegate)
                        }
                    }
                }
                Image {
                    id: imgQueue
                    width: 20
                    height: 24
                    sourceSize.height: 20
                    sourceSize.width: 20
                    opacity: tags.indexOf("Queue") !== -1 ? 0.8 : 0.5
                    source: tags.indexOf("Queue") !== -1 ? "qrc:/icons/icons/book.png" : "qrc:/icons/icons/book_gray.png"

                    MouseArea{
                        hoverEnabled: true
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: qsTr("Tag: Queue")
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(tags.indexOf("Queue") === -1)
                                lvFics.tagAdded("Queue",indexOfThisDelegate)
                            else
                                lvFics.tagDeleted("Queue",indexOfThisDelegate)
                        }
                    }
                }
                Image {
                    id: imgReading
                    width: 20
                    height: 24
                    sourceSize.height: 20
                    sourceSize.width: 20
                    opacity: tags.indexOf("Reading") !== -1 ? 0.8 : 0.5
                    source: tags.indexOf("Reading") !== -1 ? "qrc:/icons/icons/open_book.png" : "qrc:/icons/icons/open_book_gray.png"

                    MouseArea{
                        hoverEnabled: true
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: qsTr("Tag: Reading")
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(tags.indexOf("Reading") === -1)
                                lvFics.tagAdded("Reading",indexOfThisDelegate)
                            else
                                lvFics.tagDeleted("Reading",indexOfThisDelegate)
                        }
                    }
                }
                Image {
                    id: imgFinishedReading
                    width: 20
                    height: 24
                    sourceSize.height: 20
                    sourceSize.width: 20
                    opacity: tags.indexOf("Finished") !== -1 ? 0.8 : 0.5
                    source: tags.indexOf("Finished") !== -1 ? "qrc:/icons/icons/ok.png" : "qrc:/icons/icons/ok_grayed.png"

                    MouseArea{
                        hoverEnabled: true
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: qsTr("Tag: Finished")
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(tags.indexOf("Finished") === -1)
                                lvFics.tagAdded("Finished",indexOfThisDelegate)
                            else
                                lvFics.tagDeleted("Finished",indexOfThisDelegate)
                        }
                    }
                }
                Image {
                    id: imgGem
                    width: 20
                    height: 24
                    sourceSize.height: 20
                    sourceSize.width: 20
                    opacity: tags.indexOf("Gems") !== -1 ? 0.8 : 0.5
                    source: tags.indexOf("Gems") !== -1 ? "qrc:/icons/icons/gem_blue.png" : "qrc:/icons/icons/gem_gray.png"

                    MouseArea{
                        hoverEnabled: true
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: qsTr("Tag: Gems")
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(tags.indexOf("Gems") === -1)
                                lvFics.tagAdded("Gems",indexOfThisDelegate)
                            else
                                lvFics.tagDeleted("Gems",indexOfThisDelegate)
                        }
                    }
                }
                Image {
                    id: imgLove
                    width: 20
                    height: 24
                    sourceSize.height: 20
                    sourceSize.width: 20
                    opacity: tags.indexOf("Rec") !== -1  ? 1 : 0.5
                    source: tags.indexOf("Rec") !== -1 ? "qrc:/icons/icons/heart_tag.png" : "qrc:/icons/icons/heart_tag_gray.png"

                    MouseArea{
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        hoverEnabled: true
                        ToolTip.text: qsTr("Tag: Rec")
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(tags.indexOf("Rec") === -1)
                                lvFics.tagAdded("Rec",indexOfThisDelegate)
                            else
                                lvFics.tagDeleted("Rec",indexOfThisDelegate)
                        }
                    }
                }
                Image {
                    id: imgSeries
                    width: 20
                    height: 24
                    sourceSize.height: 20
                    sourceSize.width: 20
                    opacity: tags.indexOf("Series") !== -1  ? 1 : 0.5
                    source: tags.indexOf("Series") !== -1 ? "qrc:/icons/icons/link.png" : "qrc:/icons/icons/link.png"

                    MouseArea{
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        hoverEnabled: true
                        ToolTip.text: qsTr("Tag: Series")
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(tags.indexOf("Series") === -1)
                                lvFics.tagAdded("Series",indexOfThisDelegate)
                            else
                                lvFics.tagDeleted("Series",indexOfThisDelegate)
                        }
                    }
                }

                Image {
                    id: imgMagnet
                    width: 20
                    height: 24
                    sourceSize.height: 20
                    sourceSize.width: 20
                    opacity: tags.indexOf(mainWindow.magnetTag) !== -1  ? 1 : 0.5
                    source: {
                        return tags.indexOf(mainWindow.magnetTag) !== -1 ? "qrc:/icons/icons/magnet.png" : "qrc:/icons/icons/magnet_gray.png"
                    }

                    MouseArea{
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        hoverEnabled: true
                        ToolTip.text: qsTr("Currently: " + mainWindow.magnetTag)
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(tags.indexOf(mainWindow.magnetTag) === -1)
                                lvFics.tagAdded(mainWindow.magnetTag,indexOfThisDelegate)
                            else
                                lvFics.tagDeleted(mainWindow.magnetTag,indexOfThisDelegate)
                        }
                    }
                }

                Image {
                    id: imgSkip
                    width: 20
                    height: 24
                }
                Image {
                    id: imgSnooze
                    width: 20
                    height: 24
                    sourceSize.height: 20
                    sourceSize.width: 20
                    opacity: tags.indexOf("Snoozed") !== -1  ? 1 : 0.5
                    source: tags.indexOf("Snoozed") !== -1 ? "qrc:/icons/icons/bell.png" : "qrc:/icons/icons/bell_gray.png"

                    MouseArea{
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        hoverEnabled: true
                        ToolTip.text: qsTr("Tag: Snoozed")
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            if(tags.indexOf("Snoozed") === -1)
                                lvFics.tagAdded("Snoozed",indexOfThisDelegate)
                            else
                                lvFics.tagDeleted("Snoozed",indexOfThisDelegate)
                        }
                    }
                }
                Rectangle{
                    id: rectNotes
                    property bool active : false
                    //border.width: active ? 2 : 0
                    //border.color: Qt.rgba(0, 0, 1, 0.4)//radius: 5
                    //border.color: Qt.rgba(0, 0, 1, 0.4)
                    color: {
                        var color;
                        color = purged == 0 ?  "#e4f4f6FF" : "#B0FFE6FF"
                        if(snoozeExpired)
                            color = "#1100FF00"
                        return color;
                    }


                    width:24
                    height: 24

                    Image {
                        opacity: (notes || rectNotes.active)  ? 1 : 0.4
                        //anchors.fill : parent
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                        id: imgNotes
                        width: 20
                        height: 20
                        sourceSize.height: 20
                        sourceSize.width:20
                        source: {
                            if(rectNotes.active)
                                return "qrc:/icons/icons/save.png"

                            return notes ? "qrc:/icons/icons/note.png" : "qrc:/icons/icons/note_gray.png"
                        }

                        //source: "qrc:/icons/icons/pencil.png"
                        MouseArea{
//                            ToolTip.delay: 200
//                            ToolTip.visible: containsMouse
//                            ToolTip.text: "<FONT COLOR=black>" + tiNotes.text + "</FONT>"
                            id:maNotes

                            hoverEnabled: true
                            anchors.fill : parent
                            propagateComposedEvents : true
                            onClicked : {
                                //                                console.log("Notes type: ", typeof notes);
                                //                                console.log("Notes: ", notes);

                                if(!rectNotes.active)
                                {
                                    rectNotes.active = true
                                    displayNotes = true
                                }
                                else
                                {
                                    rectNotes.active = false
                                    displayNotes = false
                                }
                            }


                            onEntered: {
                                var point = imgNotes.mapToGlobal(maNotes.mouseX,maNotes.mouseY)
                                var mappedPoint = mainWindow.mapFromGlobal(point.x,point.y)

                                if(imgNotes.mapToItem(mainWindow,0,0).y > mainWindow.height/2)
                                {
                                    mappedPoint.y = mappedPoint.y - 300
                                }
                                noteTooltip.x = mappedPoint.x
                                noteTooltip.y = mappedPoint.y
                                noteTooltip.text = notes


                                //console.log("Displaying tooltip at: ", svNoteTooltip.x, svNoteTooltip.y )
                                mainWindow.displayNoteTooltip = true

                            }
                            onExited: {
                                console.log("Hiding tooltip" )
                                mainWindow.displayNoteTooltip = false
                            }

                        }
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

        Column {
            spacing: 4
            anchors.left: parent.right
            ///anchors.rightMargin: 10
            anchors.top: mainLayout.top
            anchors.leftMargin: 5

            Repeater {
                model: 5

                Image  {
                    id: rect
                    opacity: score >= index+1 ? 1 : 0.3
                    //radius: 5
                    width: 24; height: 24
                    source: {
                        //console.log("score changed, new score", score)
                        score > index ? "qrc:/icons/icons/star.png" : "qrc:/icons/icons/star_gray.png"
                    }
                    //color: "red"
                    //border.width: 1

                    MouseArea{
                        anchors.fill : parent
                        propagateComposedEvents : true
                        onClicked : {
                            lvFics.scoreAdjusted(indexOfThisDelegate, index + 1, score);

                        }
                    }
                }
            }
        }
    }



    Rectangle{
        id: snoozePart
        width: 320
        height: 30
        anchors.top: ficSheet.bottom
        anchors.right: ficSheet.right
        anchors.topMargin: 2
        //z:parent.z
        visible: delegateItem.snoozed && !snoozeExpired
        border.width: 2
        border.color: Qt.rgba(0, 0, 1, 0.4)

        color: {
            var color;
            color = purged == 0 ?  "#e4f4f6FF" : "#B0FFE6FF"
            return color;
        }

        RowLayout{
            height: parent.height
            Rectangle{
                width:5
            }
            Text {
                id: lblSnooze
                width: 40
                height: 21
                textFormat: Text.RichText;
                text: "Snooze until: "
                verticalAlignment: Text.AlignVCenter
                font.pointSize: 12
                font.family: "Verdana"
                font.bold: false
                color: "black"
            }
            Rectangle{
                anchors.topMargin: 5
                id:snoozeTypeRect
                border.width: 1
                border.color: Qt.rgba(0.5, 0.5, 0.5, 1)
                radius: 2
                width: lblSnoozeType.width + 2
                height: 21
                color: {
                    var color;
                    color = purged == 0 ?  "#e4f4f6FF" : "#B0FFE6FF"
                    if(snoozeExpired)
                        color = "#5500FF00"
                    return color;
                }
                Text {
                    id: lblSnoozeType
                    height: 20
                    anchors.leftMargin: 3
                    textFormat: Text.RichText;
                    text: {
                        if(snoozeMode === 0)
                            return "Next Chapter"
                        if(snoozeMode === 1)
                            return "Until Finished"
                        if(snoozeMode === 2)
                            return "Until Chapter"
                    }
                    verticalAlignment: Text.AlignVCenter
                    font.pointSize: 12
                    font.family: "Verdana"
                    font.bold: false
                    color: "black"
                }
                MouseArea{
                    anchors.fill : parent
                    propagateComposedEvents : true
                    onClicked : {
                        if(snoozeMode === 0)
                        {

                            lvFics.snoozeTypeChanged(indexOfThisDelegate, 1, -1);
                            snoozeMode = 1;
                        }
                        else if(snoozeMode === 1)
                        {

                            lvFics.snoozeTypeChanged(indexOfThisDelegate, 2, chapters+1);
                            snoozeMode = 2;
                        }
                        else if(snoozeMode === 2)
                        {

                            lvFics.snoozeTypeChanged(indexOfThisDelegate, 0, -1);
                            snoozeMode = 0;
                        }
                    }
                }
            }

            Rectangle{
                id: rtiChapter
                color: "lightyellow"
                width: 60
                height:row.height - 5
                visible: lblSnoozeType.text == "Until Chapter"
                TextInput{
                    id: tiFicNotes
                    horizontalAlignment:  TextInput.AlignRight
                    font.pixelSize: mainWindow.textSize
                    anchors.fill: parent
                    visible: lblSnoozeType.text == "Until Chapter"
                    color: "black"
                    text: snoozeLimit === -1 ? chapters : snoozeLimit
                    onEditingFinished: {
                        console.log("Edited text is: ", text)
                        lvFics.snoozeTypeChanged(indexOfThisDelegate, 2, parseInt(text));
                        snoozeLimit = parseInt(text)
                    }
                }
            }
        }

    }
    Rectangle{
        id: notesPart
        width: 850
        height:200

        anchors.top: ficSheet.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        radius: 5

        border.width: 2
        border.color: Qt.rgba(0, 0, 1, 0.4)

        clip: true

        color: "lightyellow"
        visible: rectNotes.active

        ScrollView{
            id: areaView
            anchors.fill: parent
            TextArea {
                id: tiNotes
                //anchors.fill: parent
                placeholderText: qsTr("Type here")
                text: notes
                horizontalAlignment:  TextInput.AlignLeft
                onEditingFinished: {
                    lvFics.notesEdited(indexOfThisDelegate, tiNotes.text)
                }
                wrapMode: TextEdit.WordWrap
            }
        }


    }

}


