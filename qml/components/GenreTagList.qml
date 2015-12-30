import QtQuick 2.4
import QtQuick.Controls 1.4

Item {
    width: 100
    height: 700
    id:tagGenreList
    TagSection{
        anchors.top: parent.top
        id:tsGenre
        tagName: "Genre"
        currentChoices: genre
        active: true

    }
    TagSection{
        id:tsTags
        anchors.top: tsGenre.bottom
        anchors.topMargin: 6
        tagName: "Tags"
        currentChoices: tags
        canAdd: true
        allChoices: tagModel

    }
    Component.onCompleted: {
            tsGenre.activated.connect(tsTags.deactivate)
            tsTags.activated.connect(tsGenre.deactivate)
            tsTags.activated.connect(delegateItem.tagListActivated)
            tsTags.tagToggled.connect(delegateItem.tagToggled)
            tagColumn.y = tsTags.y + tsTags.height + 20
            tagColumn.x = tagGenreList.x
        }
}

