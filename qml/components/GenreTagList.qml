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
        tooltip: "Displays the genres of the fic. By default implied genres are displayed where possible.\n" +
                 "'Implied' means genres detected using statistics.\n" +
                 "Those are often more accurate than what the author sets or don't contradict those.\n"+
                 "If you want to see the original genres, double click here.\n"
        currentChoices: {
            if(realGenre.length > 0)
                delegateItem.hasRealGenres = true
            else
                delegateItem.hasRealGenres = false
            //console.log(realGenre, " Length: ", realGenre.length , " Has real genres: ", delegateItem.hasRealGenres)
            if(lvFics.detailedGenreMode && delegateItem.hasRealGenres)
                return realGenre
            else
                return genre
        }
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
        tooltip: "Displays the tags you've set on a fic and allows you to set custom ones.\n" +
                 "If you want to tag a fic with something not present on the quick tag panel click 'Plus' button."
        plusTooltip: "Opens up custom tag selector.\n" +
                     "Generally it's quicker to use quick tag panel,\nbut if something isn't on it you can set this tag through here.\n" +
                     "If you click 'Magnet' button next to a tag here,\nyou will be able to set that tag with Magnet on the quick tagger panel."


        //minSlashLevel: minSlashLevel
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
        tsGenre.activated.connect(tsTags.restore)
        tsTags.activated.connect(tsGenre.deactivate)
        tsTags.activated.connect(delegateItem.tagListActivated)
        delegateItem.mouseClicked.connect(tagGenreList.makeSelection)
        tsTags.tagToggled.connect(delegateItem.tagToggled)
        tagColumn.y = tsTags.y + tsTags.height + 20
        tagColumn.x = tagGenreList.x
        //tagGenreList.makeSelection();
    }
}

