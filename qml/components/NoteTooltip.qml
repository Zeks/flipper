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
import QtQuick.Window 2.2
import QtQuick.Layouts 1.11
import QtCharts 2.0
import QtGraphicalEffects 1.0

    Rectangle{
        id: rectNoteTooltip
        property string text
        color: "white"
        border.color: "#641C34FF"
        border.width: 2
        radius: 5
        ScrollView{
            id: svNoteTooltip
            anchors.fill: parent
            TextArea{
                id: tiNotesTooltip
                z: 120
                anchors.fill: parent
                visible: parent.visible
                horizontalAlignment:  TextInput.AlignLeft
                wrapMode: TextEdit.WordWrap
                color: "black"
                placeholderText: qsTr("Type here")
                text: rectNoteTooltip.text
            }
        }
    }
