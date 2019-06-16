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
Rectangle{
    width: 20
    height: 20
    color: {
        var color;
        color = purged == 0 ?  "#e4f4f6FF" : "#B0FFE6FF"
        if(snoozeExpired)
            color = "#1100FF00"
        return color;
    }
    property string icon_colored
    property string icon_gray
    property double opacity_full : 0.8
    property double opacity_gray : 0.5
    property bool clickable : true
    property bool custom_opacity : false
    property double custom_opacity_value
    property string tooltip
    property var delegateTags
    property string delegateTag
    property string also_delete_tag
    function actOnAFic(ficIndex, ficUrl) {
        //console.log("acting on a fic")
        mainWindow.selectedIndex = ficIndex
        mainWindow.selectedUrl = ficUrl
        mainWindow.actionTakenSinceNavigation = true
    }
    Image {
        width: 20
        height: 20
        sourceSize.height: 20
        sourceSize.width: 20

        opacity:{
            if(!custom_opacity)
                return delegateTags.indexOf(delegateTag) !== -1 ? opacity_full : opacity_gray
            else
                return custom_opacity_value
        }
        source: {
            var selection = delegateTags.indexOf(delegateTag) !== -1 ? icon_colored : icon_gray
            return selection;
        }

        MouseArea{
            hoverEnabled: true
            ToolTip.delay: 1000
            ToolTip.visible: containsMouse
            ToolTip.text: tooltip
            anchors.fill : parent
            propagateComposedEvents : true
            onClicked : {
                actOnAFic(indexOfThisDelegate, url)
                if(!clickable)
                    return;
                if(delegateTags.indexOf(delegateTag) === -1)
                {
                    lvFics.tagAdded(delegateTag,indexOfThisDelegate)
                    if(also_delete_tag)
                    {
                        console.log("Also deleting: ", also_delete_tag)
                        lvFics.tagDeleted(also_delete_tag,indexOfThisDelegate)
                    }
                }
                else
                    lvFics.tagDeleted(delegateTag,indexOfThisDelegate)
            }
        }
    }
}
