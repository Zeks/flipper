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
    name: "servitor"
    consoleApplication:false
    type:"application"
    qbsSearchPaths: [sourceDirectory + "/modules", sourceDirectory + "/repo_modules"]
    Depends { name: "Qt.core"}
    Depends { name: "Qt.sql" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.quick" }
    Depends { name: "Qt.charts" }
    Depends { name: "Qt.concurrent" }
    Depends { name: "Qt.quickwidgets" }
    Depends { name: "cpp" }
    Depends { name: "logger" }
    Depends { name: "UniversalModels" }
    Depends { name: "proto_generation" }
    Depends { name: "grpc_generation" }

    cpp.defines: base.concat(["L_TREE_CONTROLLER_LIBRARY", "L_LOGGER_LIBRARY"])
    cpp.includePaths: [
        sourceDirectory,
        sourceDirectory + "/../",
        sourceDirectory + "/include",
        sourceDirectory + "/proto",
        sourceDirectory + "/libs",
        sourceDirectory + "/third_party/zlib",
        sourceDirectory + "/libs/Logger/include",
    ]
    cpp.systemIncludePaths: [
        sourceDirectory +"/proto",
        sourceDirectory + "/third_party",
        "/home/zeks/grpc/third_party/protobuf/src",
        sourceDirectory + "/../"]
    cpp.minimumWindowsVersion: "6.0"
    files: [
        "UI/servitorwindow.ui",
        "include/Interfaces/authors.h",
        "include/calc_data_holder.h",
        "include/core/db_entity.h",
        "include/core/fanfic.h",
        "include/core/fav_list_details.h",
        "include/core/fic_genre_data.h",
        "include/core/identity.h",
        "include/core/slash_data.h",
        "include/servers/token_processing.h",
        "include/ui/servitorwindow.h",
        "include/tasks/author_cache_reprocessor.h",
        "include/tasks/author_genre_iteration_processor.h",
        "include/tasks/slash_task_processor.h",
        "include/threaded_data/common_traits.h",
        "include/threaded_data/threaded_load.h",
        "include/threaded_data/threaded_save.h",
        "src/calc_data_holder.cpp",
        "src/core/fanfic.cpp",
        "src/core/fav_list_details.cpp",
        "src/data_code/rec_calc_data.cpp",
        "src/main_servitor.cpp",
        "src/rec_calc/rec_calculator_base.cpp",
        "src/rec_calc/rec_calculator_mood_adjusted.cpp",
        "src/rec_calc/rec_calculator_weighted.cpp",
        "src/servers/token_processing.cpp",
        "src/ui/servitorwindow.cpp",
        "src/Interfaces/data_source.cpp",
        "include/Interfaces/base.h",
        "include/Interfaces/genres.h",
        "include/Interfaces/fandoms.h",
        "include/Interfaces/tags.h",
        "src/Interfaces/tags.cpp",
        "include/Interfaces/fanfics.h",
        "include/Interfaces/ffn/ffn_fanfics.h",
        "include/Interfaces/ffn/ffn_authors.h",
        "include/Interfaces/db_interface.h",
        "include/Interfaces/interface_sqlite.h",
        "include/Interfaces/recommendation_lists.h",
        "src/Interfaces/authors.cpp",
        "src/Interfaces/base.cpp",
        "src/Interfaces/genres.cpp",
        "src/Interfaces/fandoms.cpp",
        "src/Interfaces/db_interface.cpp",
        "src/Interfaces/ffn/ffn_authors.cpp",
        "src/Interfaces/interface_sqlite.cpp",
        "src/Interfaces/recommendation_lists.cpp",
        "include/container_utils.h",
        "include/generic_utils.h",
        "include/timeutils.h",
        "src/generic_utils.cpp",
        "include/querybuilder.h",
        "include/queryinterfaces.h",
        "include/core/section.h",
        "include/storyfilter.h",
        "include/url_utils.h",
        "src/tasks/author_cache_reprocessor.cpp",
        "src/tasks/author_genre_iteration_processor.cpp",
        "src/tasks/slash_task_processor.cpp",
        "src/threaded_data/threaded_load.cpp",
        "src/threaded_data/threaded_save.cpp",
        "third_party/roaring/roaring.c",
        "third_party/roaring/roaring.h",
        "third_party/roaring/roaring.hh",
        "src/sqlcontext.cpp",
        "src/pure_sql.cpp",
        "src/querybuilder.cpp",
        "src/regex_utils.cpp",
        "src/core/section.cpp",
        "src/sqlitefunctions.cpp",
        "src/storyfilter.cpp",
        "src/url_utils.cpp",
        "src/rng.cpp",
        "include/rng.h",
        "src/feeder_environment.cpp",
        "include/feeder_environment.h",
        "src/transaction.cpp",
        "include/transaction.h",
        "src/pagetask.cpp",
        "include/pagetask.h",
        "include/Interfaces/pagetask_interface.h",
        "src/Interfaces/pagetask_interface.cpp",
        "include/favholder.h",
        "src/favholder.cpp",
        "include/tokenkeeper.h",
        "include/in_tag_accessor.h",
        "src/in_tag_accessor.cpp",
        "src/Interfaces/fanfics.cpp",
        "src/Interfaces/ffn/ffn_fanfics.cpp",
        "src/servers/database_context.cpp",
        "include/servers/database_context.h",
        "include/pagegetter.h",
        "src/pagegetter.cpp",
        "include/parsers/ffn/favparser.h",
        "src/parsers/ffn/favparser.cpp",
        "src/parsers/ffn/fandomparser.cpp",
        "include/parsers/ffn/fandomparser.h",
        "src/parsers/ffn/ffnparserbase.cpp",
        "include/parsers/ffn/ffnparserbase.h",
        "include/page_utils.h",
        "include/pageconsumer.h",
        "src/pageconsumer.cpp",
        "src/page_utils.cpp",
        "src/tasks/recommendations_reload_precessor.cpp",
        "include/tasks/recommendations_reload_precessor.h",
        "src/tasks/author_task_processor.cpp",
        "include/tasks/author_task_processor.h",
        "src/tasks/fandom_task_processor.cpp",
        "include/tasks/fandom_task_processor.h",
        "src/environment.cpp",
        "include/environment.h",
        "include/grpc/grpc_source.h",
        "include/Interfaces/data_source.h",
        "src/grpc/grpc_source.cpp",
        "include/core/author.h",
        "src/core/author.cpp",
        "include/core/recommendation_list.h",
        "src/core/recommendation_list.cpp",
    ]
    Group{
    name: "sqlite"
    files: [
        "third_party/sqlite3/sqlite3.c",
        "third_party/sqlite3/sqlite3.h"
    ]
    cpp.cFlags: {
        var flags = []
        flags = [ "-Wno-unused-variable", "-Wno-unused-parameter", "-Wno-cast-function-type", "-Wno-implicit-fallthrough"]
        return flags
    }
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
            "proto/search/filter.proto",
            "proto/search/fanfic.proto",
            "proto/search/fandom.proto",
            "proto/statistics/favlist.proto",
            "proto/recommendations/diagnostic_recommendations.proto",
            "proto/recommendations/recommendations.proto",
            "proto/server_base_structs.proto"

        ]
        fileTags: ["proto"]
    }
}
