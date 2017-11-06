import QtQuick 2.4
import QtQuick.Controls 1.4

Item {
    width: 100
    height: 700
    id:tagGenreList
    TagSection{
        anchors.top: tsTags.rotation === 0 ? parent.top : tsTags.bottom;
        id:tsGenre
        tagName: "Genre"
        currentChoices: genre
        active: true
        rotation:tagGenreList.rotation

    }
    TagSection{
        id:tsTags
        anchors.top: tsTags.rotation === 0 ? tsGenre.bottom : parent.top;
        anchors.topMargin: 6
        tagName: "Tags"
        currentChoices: tags
        canAdd: true
        allChoices: tagModel
    }
    function makeSelection(){
        console.log(tsTags.currentChoices);
        lvFics.currentIndex = delegateItem.indexOfThisDelegate
        if(tsTags.currentChoices.length === 0)
        {
            console.debug("Activating genres");
            tsTags.deactivateIntoAllChoices();
            tsGenre.activate();
            tsGenre.activated(delegateItem.indexOfThisDelegate)
        }
        else
        {
            console.debug("Activating tags");
            tsTags.activate();
            tsGenre.deactivate();
            tsTags.activated(delegateItem.indexOfThisDelegate)
            tsTags.showCurrrentTags();
        }
    }

    Component.onCompleted: {
            tsGenre.activated.connect(tsTags.deactivate)
            tsTags.activated.connect(tsGenre.deactivate)
            tsTags.activated.connect(delegateItem.tagListActivated)
            delegateItem.mouseClicked.connect(tagGenreList.makeSelection)
            tsTags.tagToggled.connect(delegateItem.tagToggled)
            tagColumn.y = tsTags.y + tsTags.height + 20
            tagColumn.x = tagGenreList.x
            //tagGenreList.makeSelection();
        }
}

