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
import "BaseDefines.qbs" as App


App{
    name: "schema_updater"
    consoleApplication:false
    type:"application"
    qbsSearchPaths: [sourceDirectory + "/modules", sourceDirectory + "/repo_modules"]
    Depends { name: "Qt.core"}
    Depends { name: "Qt.sql" }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.concurrent" }
    Depends { name: "cpp" }
    Depends { name: "logger" }

    cpp.defines: base.concat(["L_LOGGER_LIBRARY"])
    cpp.includePaths: [
        sourceDirectory,
        sourceDirectory + "/../",
        sourceDirectory + "/include",
        sourceDirectory + "/proto",
        sourceDirectory + "/libs",
        sourceDirectory + "/libs/Logger/include",
    ]
    cpp.minimumWindowsVersion: "6.0"
    files: [
        "src/db_synch.cpp",
        "src/main_schema_updater.cpp",
        "src/sqlcontext.cpp",
        "src/transaction.cpp",
    ]

    cpp.staticLibraries: {
        var libs = []
        libs = ["logger"]

        if(qbs.toolchain.contains("msvc"))
            libs = libs.concat(["User32","Ws2_32", "gdi32", "Advapi32"])
        return libs
    }
}
