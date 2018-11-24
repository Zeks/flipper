import QtQuick 2.10
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import QtCharts 2.2
import "components"
Rectangle {
    id:mainWindow
    property int textSize: 22
    property string currentPage: "0"
    property string totalPages: "0"
    property color leadingColor:  "#fceaef"
    property bool havePagesBefore: false
    property bool havePagesAfter: false
    property bool displayAuthorName: true
    property bool authorFilterActive: false
    property bool detailedGenreMode: false
    signal pageRequested(int page)
    signal backClicked()
    signal forwardClicked()

    property bool chartDisplay: false
    property int chartValueCommon: 100
    property int chartValueRare: 100
    property int chartValueUnique: 100

    ChartView {
        id: chartVotes
         title: "Vote Distribution"
        antialiasing: true
        z: 100
        width:300
        height:300
        margins.top: 0
        margins.bottom: 0
        margins.left: 0
        margins.right: 0
        //legend.visible: false
        visible: mainWindow.chartDisplay
        BarSeries {
            labelsVisible :false

            id: chrtBreakdown
            BarSet {  label: "Closest"; values: [chartValueUnique] }
            BarSet {  label: "Near"; values: [chartValueRare] }
            BarSet {  label: "Average"; values: [chartValueCommon] }
        }
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
            Image {
                id: imgBack
                anchors.bottomMargin: 3
                width: mainWindow.textSize
                height: mainWindow.textSize
                sourceSize.height: mainWindow.textSize
                sourceSize.width: mainWindow.textSize
                visible: true
                source: mainWindow.havePagesBefore ? "qrc:/icons/icons/back_blue.png" :  "qrc:/icons/icons/back_grey.png"
                MouseArea{
                    enabled: mainWindow.havePagesBefore
                    anchors.fill : parent
                    propagateComposedEvents : true
                    onClicked : {
                        mainWindow.backClicked();
                    }
                }
            }
            Image {
                id: imgForward
                anchors.bottomMargin: 3
                width: mainWindow.textSize
                height: mainWindow.textSize
                sourceSize.height: mainWindow.textSize
                sourceSize.width: mainWindow.textSize
                visible: true
                source: mainWindow.havePagesAfter ? "qrc:/icons/icons/forward_blue.png" :  "qrc:/icons/icons/forward_grey.png"
                MouseArea{
                    enabled: mainWindow.havePagesAfter
                    anchors.fill : parent
                    propagateComposedEvents : true
                    onClicked : {
                        mainWindow.forwardClicked();
                    }
                }
            }
            Label {
                id:info
                font.pixelSize: mainWindow.textSize
                text: "At page:"
                anchors.bottom: row.bottom
                anchors.bottomMargin: 2
            }
            Rectangle{
                color: "lightyellow"

                anchors.top: row.top
                anchors.topMargin: 2
                anchors.bottom: row.bottom
                anchors.bottomMargin: 3
                width: 80
                height:row.height - 5
                TextInput{
                    font.pixelSize: mainWindow.textSize
                    anchors.fill: parent
                    text: mainWindow.currentPage
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
                anchors.bottom: row.bottom
                anchors.bottomMargin: 2
            }
            Label {
                id:total
                font.pixelSize: mainWindow.textSize
                text: mainWindow.totalPages
                anchors.bottom: row.bottom
                anchors.bottomMargin: 2
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
        highlightOnFocus: false
        frameVisible: false
        anchors.top: spacerBeforeListview.bottom
        width: parent.width
        anchors.bottom:  parent.bottom
        verticalScrollBarPolicy: Qt.ScrollBarAsNeeded
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

            spacing: 5
            clip:true
            model:ficModel
            delegate:Ficform{}
            //delegate:Text{text:title}
            anchors.fill: parent
            //signal chapterChanged(var chapter, var author, var title)
            signal chapterChanged(var chapter, var ficId)
            signal tagDeleted(var tag, var row)
            signal tagAdded(var tag, var row)
            signal tagDeletedInTagWidget(var tag, var row)
            signal tagAddedInTagWidget(var tag, var row)
            signal tagClicked(var tag, var currentMode, var title, var author)
            signal callTagWindow()
            signal urlCopyClicked(string msg)
            signal findSimilarClicked(var id)
            signal recommenderCopyClicked(string msg)
            signal refilter()
            signal fandomToggled(var id)
            signal authorToggled(var id, var toggled)
            signal refilterClicked()
        }
    }

}

