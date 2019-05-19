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

    property bool active : false
    //border.width: active ? 2 : 0
    //border.color: Qt.rgba(0, 0, 1, 0.4)//radius: 5
    //border.color: Qt.rgba(0, 0, 1, 0.4)
    color: {
        var color;
        color = purged == 0 ?  "#e4f4f6FF" : "#B0FFE6FF"
        if(snoozeExpired)
            color = "#1100FF00"
        return color;
    }


    width:24
    height: 24

    Image {
        opacity: (notes || rectNotes.active)  ? 1 : 0.4
        //anchors.fill : parent
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        id: imgNotes
        width: 20
        height: 20
        sourceSize.height: 20
        sourceSize.width:20
        source: {
            if(rectNotes.active)
                return "qrc:/icons/icons/save.png"

            return notes ? "qrc:/icons/icons/note.png" : "qrc:/icons/icons/note_gray.png"
        }

        //source: "qrc:/icons/icons/pencil.png"
        MouseArea{
            ToolTip.delay: 1000
            ToolTip.visible: containsMouse && !notes
            ToolTip.text: qsTr("Use this to add notes to fics: where you've stopped, reviews etc...")
            id:maNotes

            hoverEnabled: true
            anchors.fill : parent
            propagateComposedEvents : true
            onClicked : {
                //                                console.log("Notes type: ", typeof notes);
                //                                console.log("Notes: ", notes);

                if(!rectNotes.active)
                {
                    rectNotes.active = true
                    displayNotes = true
                }
                else
                {
                    rectNotes.active = false
                    displayNotes = false
                }
            }


            onEntered: {
                var point = imgNotes.mapToGlobal(maNotes.mouseX,maNotes.mouseY)
                var mappedPoint = mainWindow.mapFromGlobal(point.x,point.y)

                if(imgNotes.mapToItem(mainWindow,0,0).y > mainWindow.height/2)
                {
                    mappedPoint.y = mappedPoint.y - 200
                }
                noteTooltip.x = mappedPoint.x
                noteTooltip.y = mappedPoint.y
                noteTooltip.text = notes


                //console.log("Displaying tooltip at: ", svNoteTooltip.x, svNoteTooltip.y )
                if(notes)
                    mainWindow.displayNoteTooltip = true

            }
            onExited: {
                console.log("Hiding tooltip" )
                mainWindow.displayNoteTooltip = false
            }

        }
    }

}
