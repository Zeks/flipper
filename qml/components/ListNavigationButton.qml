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

Image {
    property int iconsize
    property string iconsource
    property string tooltip
    property bool clickable : true
    property var onClickSlot

    width: iconsize
    height: iconsize
    sourceSize.height: iconsize
    sourceSize.width: iconsize

    source: iconsource

    MouseArea{
        hoverEnabled: true
        ToolTip.delay: 1000
        ToolTip.visible: containsMouse
        ToolTip.text: tooltip
        anchors.fill : parent
        propagateComposedEvents : true
        enabled: clickable
        onClicked : {
            onClickSlot();
        }
    }
}

