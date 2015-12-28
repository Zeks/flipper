import QtQuick 2.5
import QtQuick.Controls 1.4
import "qrc:components"
Rectangle {
    id:mainWindow

    ScrollView {
        highlightOnFocus: false
        frameVisible: false
        anchors.fill: parent
        verticalScrollBarPolicy: Qt.ScrollBarAsNeeded
    ListView{
        cacheBuffer: 4000

        id:lvFics
        objectName: "lvFics"
        //snapMode: ListView.SnapToItem
    spacing: 5
    clip:true
    model:ficModel
    delegate:Ficform{}
    //delegate:Text{text:title}
    anchors.fill: parent
    signal chapterChanged(var chapter, var author, var title)
    signal tagDeleted(var tag, var row)
    signal tagAdded(var tag, var row)
    signal tagClicked(var tag, var currentMode, var title, var author)
    signal callTagWindow()
    }
    }
}

