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
import "Precompiled.qbs" as Precompiled

App{
    name: "flipper"
    qbsSearchPaths: [sourceDirectory + "/modules", sourceDirectory + "/repo_modules"]
    consoleApplication:false
    type:"application"
    Depends { name: "Qt.core"}
    Depends { name: "Qt.sql" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.gui" }

    Depends { name: "Qt.quick" }
    Depends { name: "Qt.concurrent" }
    Depends { name: "Qt.quickwidgets" }
    Depends { name: "cpp" }
    Depends { name: "UniversalModels" }
    Depends { name: "logger" }
    Depends { name: "proto_generation" }
    Depends { name: "grpc_generation" }
    Depends { name: "projecttype" }

    Precompiled{condition:localvariables.usePrecompiledHeader}

    cpp.defines: base.concat(["L_TREE_CONTROLLER_LIBRARY", "L_LOGGER_LIBRARY", "_WIN32_WINNT=0x0601"])
    cpp.includePaths: [
        sourceDirectory,
        sourceDirectory + "/../",
        sourceDirectory + "/include",
        sourceDirectory + "/libs",
        sourceDirectory + "/third_party/zlib",
        sourceDirectory + "/libs/Logger/include",
    ]
    cpp.minimumWindowsVersion: "6.0"

    files: [
        "forms.qrc",
        "icons.qrc",
        "include/Interfaces/authors.h",
        "include/Interfaces/base.h",
        "include/Interfaces/db_interface.h",
        "include/Interfaces/interface_sqlite.h",
        "include/Interfaces/fandoms.h",
        "include/Interfaces/fanfics.h",
        "include/Interfaces/ffn/ffn_fanfics.h",
        "include/Interfaces/ffn/ffn_authors.h",
        "include/Interfaces/genres.h",
        "include/Interfaces/pagetask_interface.h",
        "include/Interfaces/recommendation_lists.h",
        "include/Interfaces/tags.h",
        "include/container_utils.h",
        "include/generic_utils.h",
        "include/pageconsumer.h",
        "include/pagetask.h",
        "include/db_fixers.h",
        "include/parsers/ffn/fandomparser.h",
        "include/parsers/ffn/favparser.h",
        "include/parsers/ffn/ffnparserbase.h",
        "include/parsers/ffn/ficparser.h",
        "include/parsers/ffn/fandomindexparser.h",
        "include/qml/imageprovider.h",
        "include/tasks/author_cache_reprocessor.h",
        "include/tasks/fandom_task_processor.h",
        "include/tasks/author_task_processor.h",
        "include/timeutils.h",
        "include/webpage.h",
        "src/actionprogress.cpp",
        "include/actionprogress.h",
        "UI/actionprogress.ui",
        "src/generic_utils.cpp",
        "src/parsers/ffn/fandomindexparser.cpp",
        "include/mainwindow.h",
        "include/parse.h",
        "include/querybuilder.h",
        "include/queryinterfaces.h",
        "include/core/section.h",
        "include/service_functions.h",
        "include/storyfilter.h",
        "include/tagwidget.h",
        "include/fanficdisplay.h",
        "include/transaction.h",
        "include/url_utils.h",
        "qml_ficmodel.cpp",
        "qml_ficmodel.h",
        "src/qml/imageprovider.cpp",
        "src/servers/token_processing.cpp",
        "src/tasks/author_cache_reprocessor.cpp",
        "src/tasks/fandom_task_processor.cpp",
        "src/tasks/author_task_processor.cpp",
        "src/pageconsumer.cpp",
        "third_party/sqlite3/sqlite3.c",
        "third_party/sqlite3/sqlite3.h",
        "src/Interfaces/authors.cpp",
        "src/Interfaces/base.cpp",
        "src/Interfaces/db_interface.cpp",
        "src/Interfaces/interface_sqlite.cpp",
        "src/Interfaces/fandoms.cpp",
        "src/Interfaces/fanfics.cpp",
        "src/Interfaces/ffn/ffn_fanfics.cpp",
        "src/Interfaces/ffn/ffn_authors.cpp",
        "src/Interfaces/genres.cpp",
        "src/Interfaces/pagetask_interface.cpp",
        "src/Interfaces/recommendation_lists.cpp",
        "src/Interfaces/tags.cpp",
        "src/pagetask.cpp",
        "src/db_fixers.cpp",
        "src/sqlcontext.cpp",
        "src/parsers/ffn/fandomparser.cpp",
        "src/parsers/ffn/favparser.cpp",
        "src/parsers/ffn/ffnparserbase.cpp",
        "src/parsers/ffn/ficparser.cpp",
        "src/main_flipper.cpp",
        "src/mainwindow_utility.cpp",
        "src/pagegetter.cpp",
        "include/pagegetter.h",
        "src/pure_sql.cpp",
        "src/querybuilder.cpp",
        "src/regex_utils.cpp",
        "src/core/section.cpp",
        "src/service_functions.cpp",
        "src/sqlitefunctions.cpp",
        "src/storyfilter.cpp",
        "src/transaction.cpp",
        "src/url_utils.cpp",
        "UI/mainwindow.ui",
        "UI/tagwidget.ui",
        "UI/fanficdisplay.ui",
        "src/mainwindow.cpp",
        "src/tagwidget.cpp",
        "src/fanficdisplay.cpp",
        "src/page_utils.cpp",
        "include/page_utils.h",
        "src/environment.cpp",
        "include/environment.h",
        "src/statistics_utils.cpp",
        "include/statistics_utils.h",
        "src/tasks/author_stats_processor.cpp",
        "include/tasks/author_stats_processor.h",
        "src/tasks/slash_task_processor.cpp",
        "include/tasks/slash_task_processor.h",
        "src/tasks/humor_task_processor.cpp",
        "include/tasks/humor_task_processor.h",
        "src/tasks/recommendations_reload_precessor.cpp",
        "include/tasks/recommendations_reload_precessor.h",
        "src/tasks/fandom_list_reload_processor.cpp",
        "include/tasks/fandom_list_reload_processor.h",
        "include/grpc/grpc_source.h",
        "include/Interfaces/data_source.h",
        "src/grpc/grpc_source.cpp",
        "src/Interfaces/data_source.cpp",
        "include/in_tag_accessor.h",
        "src/in_tag_accessor.cpp",
        "src/rng.cpp",
        "include/rng.h",
    ]
    Group{
    name: "qr"
    files: [
        "third_party/qr/BitBuffer.cpp",
        "third_party/qr/BitBuffer.hpp",
        "third_party/qr/QrCode.cpp",
        "third_party/qr/QrCode.hpp",
        "third_party/qr/QrSegment.cpp",
        "third_party/qr/QrSegment.hpp"
    ]
    }
    cpp.staticLibraries: {
        //var libs = ["UniversalModels", "logger", "quazip"]
        var libs = []
        if(qbs.toolchain.contains("msvc"))
            libs = ["logger"]
        else{
            libs = ["logger", "dl", "protobuf"]
        }
        libs = libs.concat(localvariables.zlib)
        libs = libs.concat(localvariables.ssl)

        if(qbs.toolchain.contains("msvc"))
            libs = libs.concat(["User32","Ws2_32", "gdi32", "Advapi32"])
        if(qbs.toolchain.contains("msvc"))
            libs = libs.concat([localvariables.protobufName,"grpc", "grpc++", "gpr"])
        else
            libs = libs.concat(["grpc", "grpc++", "gpr"])
        return libs
    }


    cpp.systemIncludePaths: [
        sourceDirectory +"/proto",
        sourceDirectory + "/third_party",
        "/home/zeks/grpc/third_party/protobuf/src",
        sourceDirectory + "/../"]

    Group{
        name:"grpc files"
        proto_generation.rootDir: localvariables.projectPath + "/proto"
        grpc_generation.rootDir: localvariables.projectPath + "/proto"
        proto_generation.protobufDependencyDir: localvariables.projectPath + "../"
        grpc_generation.protobufDependencyDir: localvariables.projectPath + "../"
        proto_generation.toolchain : qbs.toolchain
        grpc_generation.toolchain : qbs.toolchain
        files: [
            "proto/feeder_service.proto",
        ]
        fileTags: ["grpc", "proto"]
    }
    Group{
        name:"proto files"
        proto_generation.rootDir: localvariables.projectPath + "/proto"
        proto_generation.protobufDependencyDir: localvariables.projectPath + "../"
        proto_generation.toolchain : qbs.toolchain
        files: [
            "proto/filter.proto",
            "proto/fanfic.proto",
            "proto/fandom.proto",
            "proto/favlist.proto",
        ]
        fileTags: ["proto"]
    }
}
