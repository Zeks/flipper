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
        if(!snoozeExpired && ficIsSnoozed)
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
    signal mouseClicked
    clip: false



    function tagToggled(tag, state) {
        if(state)
            lvFics.tagAddedInTagWidget(tag,rownum)
        else
            lvFics.tagDeletedInTagWidget(tag,rownum)
    }

    function getDiffAndSecondIndex(displayMode,
                                   positionInFirstList, positionInSecondList,
                                   placeOnFirstPedestal, placeOnSecondPedestal){
        var diff = 0;
        var secondIndex = 0;

        if(displayMode === 0){
            diff = positionInFirstList - positionInSecondList;
            secondIndex = positionInSecondList;
        }
        else if (displayMode === 1){
            // intentionally empty
        }
        else{
            diff = placeOnFirstPedestal - placeOnSecondPedestal;
            secondIndex = placeOnSecondPedestal;
        }
        return [diff, secondIndex]
    }


    function actOnAFic(ficIndex, ficUrl) {
        //console.log("acting on a fic")
        mainWindow.selectedIndex = ficIndex
        mainWindow.selectedUrl = ficUrl
        mainWindow.actionTakenSinceNavigation = true
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

                if(mainWindow.selectedIndex == indexOfThisDelegate){
                    mainWindow.actionTakenSinceNavigation = !mainWindow.actionTakenSinceNavigation
                }
                else
                    mainWindow.actionTakenSinceNavigation = false
                mainWindow.selectedIndex = indexOfThisDelegate
                mainWindow.selectedUrl = url
                delegateItem.mouseClicked();
                // deliberately not using act function here to allow manual selection where it is needed

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
        border.width: !mainWindow.actionTakenSinceNavigation && (mainWindow.selectedIndex == indexOfThisDelegate) ?  2 : 2
        border.color: !mainWindow.actionTakenSinceNavigation && (mainWindow.selectedIndex == indexOfThisDelegate) ?  Qt.lighter("darkGreen") : Qt.rgba(0, 0, 1, 0.4)

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
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: "Clicking this copies the url into the clipboard and sends the url into the list below the fics."

                        onClicked : {
                            actOnAFic(indexOfThisDelegate, url)
                            lvFics.urlCopyClicked("http://www.fanfiction.net/s/" + url);
                        }
                        onEntered: {
                            console.log("Entered copy image")
                            lvFics.newQRSource(indexOfThisDelegate);

                            var point = imgCopy.mapToGlobal(maCopy.mouseX,maCopy.mouseY)
                            var mappedPoint = mainWindow.mapFromGlobal(point.x,point.y)

                            if(imgCopy.mapToItem(mainWindow,0,0).y > mainWindow.height/2)
                            {
                                mappedPoint.y = mappedPoint.y - 200
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

                    MouseArea{
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: "Clicking this will display fics that are most favourited with this one.\n" +
                                      "This would create an automatic recommendation list #similarfics to do that.\n"+
                                      "Please select your own list again in Recommendation List combobox\nor press Back button after you are done with this mode."

                        anchors.fill : parent
                        propagateComposedEvents : true
                        hoverEnabled :true
                        onClicked : {
                            actOnAFic(indexOfThisDelegate, url)
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
                        hoverEnabled :true
                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: "This will display other fics from the author of this one that Flipper's database knows of.\n" +
                                      "After you're done with them, please tick off the `ID search` element that is used to display these things and Reload or press Back button."

                        onClicked : {
                            actOnAFic(indexOfThisDelegate, url)
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
                    text: {
                        var usedUrl = "http://www.fanfiction.net/s/" + url
                        if(tiAtChapter.visible)
                            usedUrl += "/" + tiAtChapter.text
                        var displayedId = -1;
                        if(lvFics.idDisplayMode === 0)
                            displayedId = placeMain;
                        else if (lvFics.idDisplayMode === 1)
                            displayedId = indexOfThisDelegate;
                        else
                            displayedId = placeOnFirstPedestal;

                        return " <html><style>a:link{ color: 	#CD853F33      ;}</style><a href=\"" + usedUrl +"\">" + displayedId + "."  + title + "</a></body></html>"
                    }
                    verticalAlignment: Text.AlignVCenter
                    style: Text.Raised
                    font.pointSize: 16
                    font.family: "Verdana"
                    font.bold: true
                    color: "red"
                    onLinkActivated: {
                        Qt.openUrlExternally(link)
                        actOnAFic(indexOfThisDelegate, url)
                    }
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
                        MouseArea{
                            anchors.fill : parent
                            propagateComposedEvents : true
                            hoverEnabled :true
                            ToolTip.delay: 1000
                            ToolTip.visible: containsMouse
                            ToolTip.text: "This indicates how long ago the fic was last updated.\n" +
                                          "If the icon is fully gray - it was updated more than a year ago.\n" +
                                          "If the icon is yellow - it was updated within last year.\n" +
                                          "If the icon is green - it was updated within last month."

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
                        MouseArea{
                            anchors.fill : parent
                            propagateComposedEvents : true
                            hoverEnabled :true
                            ToolTip.delay: 1000
                            ToolTip.visible: containsMouse
                            ToolTip.text: "This indicates if the fic is finished or not.\n" +
                                          "If the icon is gray - it is unfinished.\n" +
                                          "If the icon is green - it is finished."

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

                            ToolTip.delay: 1000
                            ToolTip.visible: containsMouse
                            ToolTip.text: "This displays fanfom(s) for the fic.\n" +
                                          "Clicking here will send the fandom name to the ignores section\n" +
                                          "There, you can press Ignore Fandom to exclude it from appearing in the list.\n" +
                                          "Clicking again will cycle between first fandom and crossover fandom."

                            onClicked : {
                                actOnAFic(indexOfThisDelegate, url)
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
                            actOnAFic(indexOfThisDelegate, url)
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
                    textFormat: {
                        if(author_id > 0 )
                            return Text.RichText;
                        return Text.PlainText;
                    }
                    text: {
                        var authorUrl = ""
                        if(author_id > 0 )
                            authorUrl  = " <html><style>a:link{ color: 	#99853F33      ;}</style><a href=\"http://www.fanfiction.net/u/" +  author_id.toString() + "\">" +"By: " + author + "</a></body></html>"
                        else
                            authorUrl  = "By: " + author
                        return authorUrl
                    }
                    verticalAlignment: Text.AlignVCenter
                    style: {
                        if(author_id > 0 )
                            return Text.Raised
                    }
                    font.pointSize: 12
                    font.family: "Verdana"
                    font.bold: false
                    onLinkActivated: {
                        actOnAFic(indexOfThisDelegate, url)
                        Qt.openUrlExternally(link)
                    }

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
                    width: recommendationsMain > 0 ? 20 : 0
                    height: 24
                    sourceSize.height: 24
                    sourceSize.width: 24
                    property bool chartVisible: false
                    visible: recommendationsMain > 0
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

                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: "If this is half-green, this fic is from the author whose fic(s) you've already liked."


                        onClicked : {
                            actOnAFic(indexOfThisDelegate, url)
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
                    width: recommendationsMain > 0 ? 20 : 0
                    height: 24
                    text: {
                        return recommendationsMain
                    }
                    //text: { return recommendations + " Ratio: 1/" + chapters}
                    //text: { return recommendations + " Faves: " + favourites}
                    visible: recommendationsMain > 0
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 16
                    MouseArea{

                        hoverEnabled :true
                        anchors.fill : parent
                        propagateComposedEvents : true

                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: "The metascore assigned to the fic is based on:\nThe amount of people who liked it and have tastes similar to yours.\n" +
                                      "How closely their lists are aligned to yours.\n" +
                                      "How closely genres in their lists are aligned to yours.\n" +
                                      "To control which of these factors are enabled when lsit is created see Advanced Mode in list creation."

                    }
                }
                Text {
                    id: txtPositionDiff
                    visible: {
                        var values = getDiffAndSecondIndex(lvFics.idDisplayMode, placeMain,placeSecond, placeOnFirstPedestal,placeOnSecondPedestal);
                        var diff = values[0];
                        var secondIndex = values[1];

                        lvFics.displayListDifference && diff !== 0 && secondIndex !== 0
                    }
                    width: recommendationsMain > 0 ? 20 : 0
                    height: 24
                    text: {
                        var values = getDiffAndSecondIndex(lvFics.idDisplayMode, placeMain,placeSecond, placeOnFirstPedestal,placeOnSecondPedestal);
                        var diff = values[0];
                        var secondIndex = values[1];

                        if(lvFics.displayListDifference && recommendationsSecond != 0)
                            return "O:" + secondIndex + "(" + recommendationsSecond + ")"

                        var str = ""
                        if(diff >= 0)
                            str+="-";
                        else if(diff < 0)
                            str+="+";
                        str+=Math.abs(diff);
                        if(secondIndex === 0)
                            return "new";
                        return str
                    }
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 16
                    color: {
                        var values = getDiffAndSecondIndex(lvFics.idDisplayMode, placeMain,placeSecond, placeOnFirstPedestal,placeOnSecondPedestal);
                        var diff = values[0];
                        var secondIndex = values[1];

                        var color = Qt.lighter("darkRed")
                        if(diff > 0)
                         color = Qt.lighter("darkRed")
                        else
                         color = Qt.lighter("darkGreen")
                        if(secondIndex === 0)
                            color = Qt.lighter("darkGreen")
                        return color;
                    }
                }
                Text {
                    id: txtRatio
                    width: recommendationsMain > 0 ? 20 : 0
                    height: 24
                    text: { return " Ratio: 1/" + Math.round(favourites/recommendationsMain)}
                    visible: recommendationsMain > 0
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 16
                    MouseArea{
                        hoverEnabled :true
                        anchors.fill : parent
                        propagateComposedEvents : true

                        ToolTip.delay: 1000
                        ToolTip.visible: containsMouse
                        ToolTip.text: "This displays how much FFN favourites per one point of metascore a fic has.\n" +
                                      "if the second number is really high(500+), the fic is probably not something you'd like.\n" +
                                      "This is more a hint than a rule though. There are a lot of exceptions."

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

            RowLayout{
                Text{
                    id: txtTagsExpanations
                    text: "Tags:"
                    visible:false
                    Layout.fillHeight: true
                    font.pixelSize: 16
                }
                QuickTagger{
                    id: imgLike
                    delegateTag: "Liked"
                    also_delete_tag: "Disliked"
                    delegateTags: tags
                    tooltip:  qsTr("Tag: Liked")
                    icon_colored: "qrc:/icons/icons/like_green.png"
                    icon_gray: "qrc:/icons/icons/like.png"
                    opacity_full: 1
                    opacity_gray: 0.3
                    custom_opacity: true
                    custom_opacity_value: ((tags.indexOf("Disliked") === -1 && tags.indexOf("Hide") === -1) || tags.indexOf("Liked")  !== -1)  ?  opacity_full : opacity_gray

                }

                QuickTagger{
                    id: imgDislike
                    delegateTag: "Disliked"
                    also_delete_tag: "Liked"
                    delegateTags: tags
                    tooltip:  qsTr("Tag: Disliked")
                    icon_colored: "qrc:/icons/icons/dislike_red.png"
                    icon_gray: "qrc:/icons/icons/dislike.png"
                    opacity_full: 1
                    opacity_gray: 0.3
                    custom_opacity: true
                    custom_opacity_value: ((tags.indexOf("Liked") === -1 && tags.indexOf("Hide") === -1) || tags.indexOf("Disliked")  !== -1) ?  opacity_full : opacity_gray

                }
                QuickTagger{
                    id: imgDead
                    delegateTag: "Limbo"
                    delegateTags: tags
                    tooltip:  qsTr("Tag: Limbo")
                    icon_colored: "qrc:/icons/icons/ghost.png"
                    icon_gray: "qrc:/icons/icons/ghost_gray.png"
                    opacity_full: 0.5
                    opacity_gray: 0.5
                }

                QuickTagger{
                    id: imgDiscard
                    delegateTag: "Hide"
                    delegateTags: tags
                    tooltip:  qsTr("Tag: Hide")
                    icon_colored: "qrc:/icons/icons/trash_darker.png"
                    icon_gray: "qrc:/icons/icons/trash.png"
                    opacity_full: 1
                    opacity_gray: 0.3
                    custom_opacity: true
                    custom_opacity_value: ((tags.indexOf("Liked") === -1 && tags.indexOf("Disliked") === -1) || tags.indexOf("Hide")  !== -1) ?  opacity_full : opacity_gray
                }

                QuickTagger{
                    id: imgMeh
                    delegateTag: "Meh"
                    delegateTags: tags
                    tooltip:  qsTr("Tag: Meh")
                    icon_colored: "qrc:/icons/icons/lock_yellow.png"
                    icon_gray: "qrc:/icons/icons/lock_white.png"
                    opacity_full: 1
                    opacity_gray: 0.7
                }
                QuickTagger{
                    id: imgSpoiler
                    delegateTag: "Spoiler"
                    delegateTags: tags
                    tooltip:  qsTr("Tag: Spoiler")
                    icon_colored: "qrc:/icons/icons/spoilers.png"
                    icon_gray: "qrc:/icons/icons/spoilers_gray.png"
                    opacity_full: 1
                    opacity_gray: 0.7
                }
                QuickTagger{
                    id: imgWait
                    delegateTag: "Wait"
                    delegateTags: tags
                    tooltip:  qsTr("Tag: Wait")
                    icon_colored: "qrc:/icons/icons/clock_green.png"
                    icon_gray: "qrc:/icons/icons/clock_gray.png"
                    opacity_full: 1
                }
                QuickTagger{
                    id: imgStem
                    delegateTag: "Stem"
                    delegateTags: tags
                    tooltip:  qsTr("Tag: Stem")
                    icon_colored: "qrc:/icons/icons/stem_green.png"
                    icon_gray: "qrc:/icons/icons/stem_gray.png"
                    opacity_full: 1
                }
                QuickTagger{
                    id: imgQueue
                    delegateTag: "Queue"
                    delegateTags: tags
                    tooltip:  qsTr("Tag: Queue")
                    icon_colored: "qrc:/icons/icons/book.png"
                    icon_gray: "qrc:/icons/icons/book_gray.png"
                    opacity_full: 0.8
                }
                QuickTagger{
                    id: imgFinishedReading
                    delegateTag: "Finished"
                    delegateTags: tags
                    tooltip:  qsTr("Tag: Finished")
                    icon_colored: "qrc:/icons/icons/ok.png"
                    icon_gray: "qrc:/icons/icons/ok_grayed.png"
                    opacity_full: 0.8
                }

                QuickTagger{
                    id: imgGem
                    delegateTag: "Gems"
                    delegateTags: tags
                    tooltip:  qsTr("Tag: Gems")
                    icon_colored: "qrc:/icons/icons/gem_blue.png"
                    icon_gray: "qrc:/icons/icons/gem_gray.png"
                    opacity_full: 0.8
                }

                QuickTagger{
                    id: imgLove
                    delegateTag: "Rec"
                    delegateTags: tags
                    tooltip:  qsTr("Tag: Rec")
                    icon_colored: "qrc:/icons/icons/heart_tag.png"
                    icon_gray: "qrc:/icons/icons/heart_tag_gray.png"
                    opacity_full: 1
                }

                QuickTagger{
                    id: imgSeries
                    delegateTag: "Series"
                    delegateTags: tags
                    tooltip:  qsTr("Tag: Series")
                    icon_colored: "qrc:/icons/icons/link.png"
                    icon_gray: "qrc:/icons/icons/link.png"
                    opacity_full: 1
                }

                QuickTagger{
                    id: imgMagnet
                    delegateTag: mainWindow.magnetTag
                    delegateTags: tags
                    opacity_full: 1
                    tooltip:  {
                        if(!magnetTag)
                            qsTr("Used to quickly assign a user defined tag. Select magnet in tag dropdown menu on a tag to link this tag to a magnet")
                        else
                            qsTr("Currently: " + mainWindow.magnetTag)
                    }
                    icon_colored: "qrc:/icons/icons/magnet.png"
                    icon_gray: "qrc:/icons/icons/magnet_gray.png"
                    clickable: magnetTag
                }

                Image {
                    id: imgSkip
                    width: 20
                    height: 24
                }
                Image {
                    id: imgSnoozePlaceholder
                    width: 20
                    height: 24
                    visible: complete && !snoozeExpired
                }
                SnoozeToggle{
                    id: imgSnooze
                    tooltip: qsTr("Use this to remove fics from tag search results until they update")
                    icon_colored: "qrc:/icons/icons/bell.png"
                    icon_gray: "qrc:/icons/icons/bell_gray.png"
                    opacity_full: 1
                    visible: !complete || snoozeExpired
                }

                NotesTagger{
                    id: rectNotes
                }
                QuickTagger{
                    id: imgReading
                    delegateTag: "Reading"
                    delegateTags: tags
                    tooltip: qsTr("Use this to tag fic as 'Reading' and set a chapter you're at.")
                    icon_colored: "qrc:/icons/icons/open_book.png"
                    icon_gray: "qrc:/icons/icons/open_book_gray.png"
                }

                Rectangle{
                    id: rtiAtChapter
                    color: "lightyellow"
                    width: 38
                    border.width: 2
                    border.color: Qt.rgba(0, 0, 1, 0.4)
                    radius: 5
                    height:row.height - 5
                    visible: tags.indexOf("Reading") !== -1
                    TextInput{
                        id: tiAtChapter
                        horizontalAlignment:  TextInput.AlignRight
                        font.pixelSize: mainWindow.textSize - 2
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: -1
                        anchors.rightMargin: 4

                        width: 35
                        color: "black"
                        text: atChapter === 0 ? chapters : atChapter
                        onEditingFinished: {
                            actOnAFic(indexOfThisDelegate, url)
                            console.log("Edited text is: ", text)
                            lvFics.chapterChanged(indexOfThisDelegate, parseInt(text));
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
                            actOnAFic(indexOfThisDelegate, url)
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
        visible: ficIsSnoozed && !snoozeExpired
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
                            return "Until Chapter"
                        if(snoozeMode === 2)
                            return "Until Finished"

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
                visible: snoozeMode === 1
                TextInput{
                    id: tiFicNotes
                    horizontalAlignment:  TextInput.AlignRight
                    font.pixelSize: mainWindow.textSize
                    anchors.fill: parent
                    visible: snoozeMode === 1
                    color: "black"
                    text: snoozeLimit === -1 ? chapters + 1 : snoozeLimit
                    onEditingFinished: {
                        console.log("Edited text is: ", text)
                        lvFics.snoozeTypeChanged(indexOfThisDelegate, 1, parseInt(text));
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
                onFocusChanged: {
                    lvFics.notesEdited(indexOfThisDelegate, tiNotes.text)
                }
            }
        }


    }

}


