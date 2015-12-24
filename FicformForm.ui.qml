import QtQuick 2.4
import QtQuick.Controls 1.4

Item {
    width: 800
    height: 300

    Text {
        id: txtSummary
        x: 78
        y: 82
        width: 518
        height: 132
        text: qsTr("Text")
        font.pixelSize: 12
    }

    ComboBox {
        id: cbChapters
        x: 462
        y: 227
        width: 99
        height: 16
    }

    Text {
        id: txtChapters
        x: 401
        y: 227
        text: qsTr("At chapter:")
        font.pixelSize: 12
    }

    Text {
        id: txtOf
        x: 571
        y: 227
        text: qsTr("of")
        font.pixelSize: 12
    }

    ListView {
        id: lvGenres
        x: 79
        y: 224
        width: 294
        height: 17
        model: ListModel {
            ListElement {
                name: "Grey"
                colorCode: "grey"
            }

            ListElement {
                name: "Red"
                colorCode: "red"
            }

            ListElement {
                name: "Blue"
                colorCode: "blue"
            }

            ListElement {
                name: "Green"
                colorCode: "green"
            }
        }
    }

    Text {
        id: txtCharacters
        x: 633
        y: 28
        text: qsTr("Characters")
        font.pixelSize: 12
    }

    ListView {
        id: lvCharacters
        x: 611
        y: 47
        width: 114
        height: 170
        model: ListModel {
            ListElement {
                name: "Grey"
                colorCode: "grey"
            }

            ListElement {
                name: "Red"
                colorCode: "red"
            }

            ListElement {
                name: "Blue"
                colorCode: "blue"
            }

            ListElement {
                name: "Green"
                colorCode: "green"
            }
        }
    }

    Label {
        id: lblTitle
        x: 80
        y: 30
        width: 520
        height: 21
        text: qsTr("Label")
    }

    ToolButton {
        id: tbTrack
        x: 39
        y: 30
    }

    ToolButton {
        id: tbHide
        x: 39
        y: 74
    }

    ListView {
        id: lvTags
        x: 77
        y: 252
        width: 519
        height: 18
        model: ListModel {
            ListElement {
                name: "Grey"
                colorCode: "grey"
            }

            ListElement {
                name: "Red"
                colorCode: "red"
            }

            ListElement {
                name: "Blue"
                colorCode: "blue"
            }

            ListElement {
                name: "Green"
                colorCode: "green"
            }
        }
    }

    ToolButton {
        id: tbUpdate
        x: 39
        y: 125
    }

    ToolButton {
        id: tbAddGenre
        x: 39
        y: 211
    }

    ToolButton {
        id: tbAddTag
        x: 43
        y: 252
    }

    ToolButton {
        id: tbAddCharacters
        x: 614
        y: 26
        width: 13
        height: 17
    }

    ListView {
        id: lvFandoms
        x: 109
        y: 57
        width: 331
        height: 22
        model: ListModel {
            ListElement {
                name: "Grey"
                colorCode: "grey"
            }

            ListElement {
                name: "Red"
                colorCode: "red"
            }

            ListElement {
                name: "Blue"
                colorCode: "blue"
            }

            ListElement {
                name: "Green"
                colorCode: "green"
            }
        }
    }

    Text {
        id: text1
        x: 613
        y: 225
        text: qsTr("Pub:")
        font.pixelSize: 12
    }

    Text {
        id: text2
        x: 613
        y: 246
        text: qsTr("Upd:")
        font.pixelSize: 12
    }

    Item {
        id: itRating
        x: 80
        y: 55
        width: 19
        height: 22
    }

    Text {
        id: txtWords
        x: 525
        y: 58
        width: 70
        height: 19
        text: qsTr("Words:")
        font.pixelSize: 12
    }
}

