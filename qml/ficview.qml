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

import QtQuick 2.10
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import QtCharts 2.2
import "components"
Rectangle {
    id:mainWindow
    property int textSize: 22
    property int selectedIndex: -1
    property string selectedUrl

    property string currentPage: "0"
    property string totalPages: "0"
    property string magnetTag
    property bool magnetTop: true

    property bool actionTakenSinceNavigation: false

    property color leadingColor:  "#fceaef"

    property bool havePagesBefore: false
    property bool havePagesAfter: false
    property bool authorFilterActive: false

    property bool displaySnoozed: true
    property bool displayNoteTooltip: false

    signal pageRequested(int page)
    signal backClicked()
    signal forwardClicked()
    signal shuffleClicked()

    property bool chartDisplay: false
    property bool qrDisplay: false
    property int chartValueCommon: 100
    property int chartValueUncommon: 100
    property int chartValueRare: 100
    property int chartValueUnique: 100

    property string chartValueCountCommon
    property string chartValueCountUncommon
    property string chartValueCountRare
    property string chartValueCountUnique
    function centerOnSelection(index) {
        console.log("centering on: ", index)
       lvFics.positionViewAtIndex(index, ListView.Center);
    }
    Image {
        id: imgQRCode
        objectName: "imgQRCode"
        width: 200
        height: 200
        //verticalAlignment: Text.AlignVCenter
        source: "image://qrImageProvider/qrcode"
        //source: "qrc:/icons/icons/add.png"

        visible: qrDisplay
        z: 100
    }
    Connections {
        target: main
        onQrChange: {
            console.log("In source change slot")
            imgQRCode.source = "image://qrImageProvider/qrcode?" + Math.random()
            // change URL to refresh image. Add random URL part to avoid caching
        }
    }

    ChartView {
        id: chartVotes
        title: "Vote Distribution"
        antialiasing: true
        z: 100
        width:500
        height:300
        visible: mainWindow.chartDisplay
        BarSeries {
            labelsVisible :false

            id: chrtBreakdown
            BarSet {  label: "Average(" + chartValueCountCommon + ")"; values: [chartValueCommon] }
            BarSet {  label: "Uncommon(" + chartValueCountUncommon+ ")"; values: [chartValueUncommon] }
            BarSet {  label: "Near(" + chartValueCountRare + ")"; values: [chartValueRare] }
            BarSet {  label: "Closest(" + chartValueCountUnique + ")" ; values: [chartValueUnique] }
        }
    }

    NoteTooltip{
       id: noteTooltip
       visible: mainWindow.displayNoteTooltip
       width: 500
       height:200
       z: 100
    }

    Rectangle{
        id:leadingLine
        width: parent.width
        height:2
        color: "green"
        anchors.top: parent.top
    }
    Item {
        id:spacerBeforeRow
        anchors.top: leadingLine.bottom
        height:3
    }
    Rectangle
    {
        id: coloredRect
        anchors.top: spacerBeforeRow.bottom
        color: leadingColor
        height: info.height - 2
        width: parent.width
        RowLayout{
            id:row
            ListNavigationButton{
                id: imgTarget
                anchors.bottomMargin: 3
                iconsize: mainWindow.textSize
                iconsource: mainWindow.selectedIndex == -1 ? "qrc:/icons/icons/target_grey.png" : "qrc:/icons/icons/target_blue.png"
                tooltip: "This will center the list on the recently selected fic.\nTo select a fic click somewhere on its summary, but NOT on any of the control buttons\n." +
                         "Selection persists between refilters so if a fic is still in the list after Reload clocking this button will navigate to it."
                onClickSlot: function(){
                    if(mainWindow.selectedIndex == -1)
                        return
                    mainWindow.actionTakenSinceNavigation = false
                    mainWindow.centerOnSelection(mainWindow.selectedIndex);
                }
            }
            ListNavigationButton{
                id: imgBack
                anchors.bottomMargin: 3
                iconsize: mainWindow.textSize
                iconsource: mainWindow.havePagesBefore ? "qrc:/icons/icons/back_blue.png" :  "qrc:/icons/icons/back_grey.png"
                clickable: mainWindow.havePagesBefore
                tooltip: "Navigates to the previous page of the current search results if you aren't on the first one."
                onClickSlot: function(){
                        mainWindow.backClicked();
                }
            }
            ListNavigationButton{
                id: imgForward
                anchors.bottomMargin: 3
                iconsize: mainWindow.textSize
                iconsource: mainWindow.havePagesAfter ? "qrc:/icons/icons/forward_blue.png" :  "qrc:/icons/icons/forward_grey.png"
                clickable: mainWindow.havePagesAfter
                tooltip: "Navigates to the next page of the current search results if you aren't on the last one."
                onClickSlot: function(){
                        mainWindow.forwardClicked();
                }
            }
            ListNavigationButton{
                id: imgShuffle
                anchors.bottomMargin: 3
                iconsize: mainWindow.textSize
                iconsource: "qrc:/icons/icons/shuffle_2x.png"
                clickable: true
                tooltip: "Shuffles the currently displayed fic results."
                onClickSlot: function(){
                        mainWindow.shuffleClicked();
                }
            }
            Label {
                id:info
                font.pixelSize: mainWindow.textSize
                text: "At page:"
            }
            Rectangle{
                color: "lightyellow"
                width: 80
                height:row.height - 5
                TextInput{
                    font.pixelSize: mainWindow.textSize
                    anchors.fill: parent
                    text: parseInt(mainWindow.currentPage) + parseInt(1)
                    validator: IntValidator{}
                    horizontalAlignment: TextInput.AlignRight
                    onEditingFinished: {
                        mainWindow.pageRequested(text)
                    }
                }
            }
            Label {
                text: "of:"
                font.pixelSize: mainWindow.textSize
//                anchors.bottom: row.bottom
//                anchors.bottomMargin: 2
            }
            Label {
                id:total
                font.pixelSize: mainWindow.textSize
                text: mainWindow.totalPages
//                anchors.bottom: row.bottom
//                anchors.bottomMargin: 2
            }

            Item {
                // spacer item
                Layout.fillWidth: true
                Layout.fillHeight: true
                //Rectangle { anchors.fill: parent; color: "#ffaaaa" } // to visualize the spacer
            }
            width: parent.width
        }
    }
    Item {
        id:spacerBeforeSeparator
        anchors.top: coloredRect.bottom
        height:3
    }
    Rectangle{
        id:separator
        width: parent.width
        height:2
        color: "green"
        anchors.top: spacerBeforeSeparator.bottom
    }
    Item {
        id:spacerBeforeListview
        anchors.top: separator.bottom
        height:5
    }
    ScrollView {
        //highlightOnFocus: false
        //frameVisible: false
        anchors.top: spacerBeforeListview.bottom
        width: parent.width
        anchors.bottom:  parent.bottom
        //verticalScrollBarPolicy: Qt.ScrollBarAsNeeded
        ListView{
            cacheBuffer: 6000
            displayMarginBeginning: 50
            displayMarginEnd:  50
            id:lvFics
            z:1
            objectName: "lvFics"
            //snapMode: ListView.NoSnap
            property int previousIndex: -1
            property bool showUrlCopyIcon: urlCopyIconVisible
            property bool showScanIcon: scanIconVisible
            property bool authorFilterActive: false
            property bool displayAuthorName : displayAuthorNameInList
            property bool displayListDifference: displayListDifferenceInList
            property int idDisplayMode: idDisplayModeInList
            property bool detailedGenreMode: detailedGenreModeInList

            spacing: 5
            clip:true
            model:ficModel
            delegate:Ficform{}
            //delegate:Text{text:title}
            anchors.fill: parent
            //signal chapterChanged(var chapter, var author, var title)
            signal chapterChanged(var ficId, var chapter)
            signal tagDeleted(var tag, var row)
            signal tagAdded(var tag, var row)
            signal tagDeletedInTagWidget(var tag, var row)
            signal tagAddedInTagWidget(var tag, var row)
            signal tagClicked(var tag, var currentMode, var title, var author)
            signal callTagWindow()
            signal urlCopyClicked(string msg)
            signal findSimilarClicked(var id)
            signal newQRSource(var id)
            signal recommenderCopyClicked(string msg)
            signal refilter()
            signal fandomToggled(var id)
            signal authorToggled(var id, var toggled)
            signal refilterClicked()
            signal heartDoubleClicked(var id)
            signal scoreAdjusted(var id, var value, var currentScore)
            signal addSnooze(var id)
            signal removeSnooze(var id)
            signal snoozeTypeChanged(var id, var value, var chapter)
            signal notesEdited(var id, var value)
        }
    }

}

