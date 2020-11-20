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
    Depends { name: "Environment" }
    Depends { name: "proto_generation" }
    Depends { name: "grpc_generation" }
    Depends { name: "projecttype" }

    Precompiled{condition:Environment.usePrecompiledHeader}

    cpp.defines: base.concat(["L_TREE_CONTROLLER_LIBRARY", "L_LOGGER_LIBRARY", "_WIN32_WINNT=0x0601", "CLIENT_VERSION=1.4.2", "FMT_HEADER_ONLY"])
    cpp.includePaths: [
        sourceDirectory,
        sourceDirectory + "/../",
        sourceDirectory + "/include",
        sourceDirectory + "/libs",
        sourceDirectory + "/third_party/zlib",
        sourceDirectory + "/libs/Logger/include",
        sourceDirectory + "/third_party/fmt/include",
    ]
    cpp.systemIncludePaths: [
        sourceDirectory +"/proto",
        sourceDirectory + "/third_party",
        "/home/zeks/grpc/third_party/protobuf/src",
        Environment.sqliteFolder,
        sourceDirectory + "/../"]


    cpp.minimumWindowsVersion: "6.0"

    files: [
        "include/core/db_entity.h",
        "include/core/experimental/fic_relations.h",
        "include/core/fandom.h",
        "include/core/fanfic.h",
        "include/core/fav_list_details.h",
        "include/core/identity.h",
        "include/core/slash_data.h",
        "include/core/url.h",
        "include/parsers/ffn/desktop_favparser.h",
        "include/parsers/ffn/favparser_wrapper.h",
        "include/parsers/ffn/mobile_favparser.h",
        "src/Interfaces/fandom_lists.cpp",
        "src/core/fandom.cpp",
        "src/core/fanfic.cpp",
        "src/core/fav_list_details.cpp",
        "src/parsers/ffn/desktop_favparser.cpp",
        "src/parsers/ffn/favparser_wrapper.cpp",
        "src/parsers/ffn/mobile_favparser.cpp",
        "include/backups.h",
        "src/backups.cpp",
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
        "include/parsers/ffn/ffnparserbase.h",
        "include/parsers/ffn/ficparser.h",
        "include/parsers/ffn/fandomindexparser.h",
        "include/qml/imageprovider.h",
        "include/tasks/author_cache_reprocessor.h",
        "include/tasks/fandom_task_processor.h",
        "include/tasks/author_task_processor.h",
        "include/timeutils.h",
        "include/webpage.h",
        "src/generic_utils.cpp",
        "src/parsers/ffn/fandomindexparser.cpp",
        "include/parse.h",
        "include/querybuilder.h",
        "include/queryinterfaces.h",
        "include/core/section.h",
        "include/service_functions.h",
        "include/storyfilter.h",
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
        "src/parsers/ffn/ffnparserbase.cpp",
        "src/parsers/ffn/ficparser.cpp",
        "src/main_flipper.cpp",
        "src/ui/mainwindow_utility.cpp",
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
        "src/page_utils.cpp",
        "include/page_utils.h",
        "src/environment.cpp",
        "include/environment.h",
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
        "include/core/author.h",
        "src/core/author.cpp",
        "include/core/recommendation_list.h",
        "src/core/recommendation_list.cpp",
    ]


    Group{
    name: "UI"
    files: [
        "src/ui/fandomlistwidget.cpp",
        "include/ui/fandomlistwidget.h",
        "UI/fandomlistwidget.ui",
        "UI/mainwindow.ui",
        "include/ui/mainwindow.h",
        "src/ui/mainwindow.cpp",
        "UI/tagwidget.ui",
        "include/ui/tagwidget.h",
        "src/ui/tagwidget.cpp",
        "UI/fanficdisplay.ui",
        "include/ui/fanficdisplay.h",
        "src/ui/fanficdisplay.cpp",
        "UI/actionprogress.ui",
        "src/ui/actionprogress.cpp",
        "include/ui/actionprogress.h",
        "UI/initialsetupdialog.ui",
        "src/ui/initialsetupdialog.cpp",
        "include/ui/initialsetupdialog.h",
        "UI/welcomedialog.ui",
        "src/ui/welcomedialog.cpp",
        "include/ui/welcomedialog.h",
    ]

    }

    Group{
    name: "sqlite"
    files: [
        Environment.sqliteFolder + "/sqlite3.c",
        Environment.sqliteFolder + "/sqlite3.h"
    ]
    cpp.cFlags: {
        var flags = []
        if(!qbs.toolchain.contains("msvc"))
            flags = [ "-Wno-unused-variable", "-Wno-unused-parameter", "-Wno-cast-function-type", "-Wno-implicit-fallthrough"]
        return flags
    }
    }
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
        var libs = []
        if(qbs.toolchain.contains("msvc"))
            libs = []
        else{
            libs = ["dl", "protobuf"]
        }
        if(qbs.toolchain.contains("msvc"))
            libs = libs.concat(["User32","Ws2_32", "gdi32", "Advapi32"])
        if(qbs.toolchain.contains("msvc"))
            libs = libs.concat(["grpc", "grpc++", "gpr", "zlib", "cares", "crypto", "ssl", "libprotobufd"])
        else
            libs = ["dl", "protobuf", "grpc", "grpc++", "gpr","cpr", "curl", "crypto", "ssl", "pthread"]
        return libs
    }




    Group{
        name:"grpc files"
        proto_generation.rootDir: project.rootFolder + "/proto"
        grpc_generation.rootDir: project.rootFolder + "/proto"
        proto_generation.protobufDependencyDir: project.rootFolder + "../"
        grpc_generation.protobufDependencyDir: project.rootFolder + "../"
        proto_generation.toolchain : qbs.toolchain
        grpc_generation.toolchain : qbs.toolchain
        files: [
            "proto/feeder_service.proto",
        ]
        fileTags: ["grpc", "proto"]
    }
    Group{
        name:"proto files"
        proto_generation.rootDir: project.rootFolder + "/proto"
        proto_generation.protobufDependencyDir: project.rootFolder + "../"
        proto_generation.toolchain : qbs.toolchain
        files: [
            "proto/search/filter.proto",
            "proto/search/fanfic.proto",
            "proto/search/fandom.proto",
            "proto/statistics/favlist.proto",
            "proto/recommendations/diagnostic_recommendations.proto",
            "proto/recommendations/recommendations.proto",
            "proto/server_base_structs.proto",
        ]
        fileTags: ["proto"]
    }
}
