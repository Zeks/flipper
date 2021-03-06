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

import qbs 1.0
import qbs.Process
import qbs.File
import qbs.Environment
import "BaseDefines.qbs" as Application

Project {
    name: "flipper_proj"
    qbsSearchPaths: [sourceDirectory + "/modules", sourceDirectory + "/repo_modules"]
    property string rootFolder: {
        var rootFolder = File.canonicalFilePath(sourceDirectory).toString();
        console.error("Source:" + rootFolder)
        return rootFolder.toString()
    }
    property bool usePostgres: false
    property bool useWebview: true
    references: [
        "flipper.qbs",
        "gui_condition.qbs",
        "environment_plugs.qbs",
        "libs/ui-models/UniversalModels.qbs",
        "libs/Logger/logger.qbs",
        "libs/sql/sql.qbs",
    ]
}
