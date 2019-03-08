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
import QtQuick 2.0
import QtQuick.Controls 1.4
Rectangle {
    id: tagCloud

    SystemPalette { id: palette }   //we get the system default colors from this

    //public API
    property variant model
    property color baseColor: palette.base
    property color textColor: palette.text
    property int   textFontSize: 16

    color: baseColor

    Flow {
        id: flow
        flow:Flow.LeftToRight
        width: parent.width
        spacing: 5
        anchors.margins: 4
        anchors.verticalCenter: parent.verticalCenter
        property int maxHeight: 0

        Repeater {
            id: repeater
            model: tagCloud.model
            Text {
                id: textBlock
                text: modelData
                font.pointSize: tagCloud.textFontSize;
                onTextChanged: {
                    if(y > flow.maxHeight)
                    {
                        flow.maxHeight = y
                        print("Max:" + flow.maxHeight )
                    }
                }
            }
        }
    }
}
