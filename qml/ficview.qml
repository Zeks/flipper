import QtQuick 2.10
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import "components"
Rectangle {
    id:mainWindow
    property int textSize: 22
    property string currentPage: "1"
    property string totalPages: "1"
    property bool havePagesBefore: false
    property bool havePagesAfter: false
    signal backClicked()
    signal forwardClicked()
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
    RowLayout{
        id:row
        Item {
            // spacer item
            Layout.fillWidth: true
            Layout.fillHeight: true
            //Rectangle { anchors.fill: parent; color: "#ffaaaa" } // to visualize the spacer
        }
        Label {
            id:info
            font.pixelSize: mainWindow.textSize
            text: "At page:"
            anchors.top: row.top
        }
        Rectangle{
            color: "lightyellow"
            anchors.bottom: info.bottom
            //anchors.bottom: row.bottom
            width: 80
            height:row.height - 5
            Item {
                // spacer item
                Layout.fillWidth: true
                Layout.fillHeight: true
                //Rectangle { anchors.fill: parent; color: "#ffaaaa" } // to visualize the spacer
            }
            TextInput{
                font.pixelSize: mainWindow.textSize
                anchors.fill: parent
                text: mainWindow.currentPage
                validator: IntValidator{}
                horizontalAlignment: TextInput.AlignRight

                //placeholderText: qsTr("")
            }
        }
        Label {
            text: "of:"
            font.pixelSize: mainWindow.textSize
            anchors.top: row.top
        }
        Label {
            id:total
            font.pixelSize: mainWindow.textSize
            text: mainWindow.totalPages
            anchors.top: row.top
        }
        Image {
            id: imgBack
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
        width: parent.width
    }
    Item {
        id:spacerBeforeSeparator
        anchors.top: row.bottom
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
            objectName: "lvFics"
            //snapMode: ListView.NoSnap
            property int previousIndex: -1
            property bool showUrlCopyIcon: urlCopyIconVisible

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
            signal tagClicked(var tag, var currentMode, var title, var author)
            signal callTagWindow()
            signal urlCopyClicked(string msg)
            signal recommenderCopyClicked(string msg)

        }
    }

}

