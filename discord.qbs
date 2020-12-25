import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as App


App{
    name: "discord"
    //cpp.cxxLanguageVersion: "c++14"
    Depends { name: "Qt.core"}
    Depends { name: "Qt.sql" }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.concurrent" }
    Depends { name: "cpp" }
    Depends { name: "Environment" }
    Depends { name: "logger" }
    Depends { name: "proto_generation" }
    Depends { name: "grpc_generation" }

    files: [
        "include/Interfaces/pagetask_interface.h",
        "include/core/db_entity.h",
        "include/core/fanfic.h",
        "include/core/fav_list_details.h",
        "include/core/identity.h",
        "include/core/slash_data.h",
        "include/discord/actions.h",
        "include/discord/client_v2.h",
        "include/discord/command.h",
        "include/discord/command_controller.h",
        "include/discord/command_generators.h",
        "include/discord/db_vendor.h",
        "include/discord/discord_ffn_page.h",
        "include/discord/discord_init.h",
        "include/discord/discord_message_token.h",
        "include/discord/discord_pagegetter.h",
        "include/discord/discord_server.h",
        "include/discord/discord_user.h",
        "include/discord/fandom_filter_token.h",
        "include/discord/fetch_filters.h",
        "include/discord/help_generator.h",
        "include/discord/limits.h",
        "include/discord/task.h",
        "include/discord/task_environment.h",
        "include/discord/task_runner.h",
        "include/discord/type_functions.h",
        "include/discord/type_strings.h",
        "include/environment.h",
        "include/page_utils.h",
        "include/pageconsumer.h",
        "include/parsers/ffn/desktop_favparser.h",
        "include/parsers/ffn/discord/discord_mobile_favparser.h",
        "include/parsers/ffn/fandomparser.h",
        "include/parsers/ffn/favparser_wrapper.h",
        "include/parsers/ffn/mobile_favparser.h",
        "include/sql/discord/discord_queries.h",
        "include/sql_abstractions/sql_context.h",
        "include/sql_abstractions/sql_database.h",
        "include/sql_abstractions/sql_database_impl_base.h",
        "include/sql_abstractions/sql_database_impl_null.h",
        "include/sql_abstractions/sql_database_impl_pq.h",
        "include/sql_abstractions/sql_database_impl_sqlite.h",
        "include/sql_abstractions/sql_error.h",
        "include/sql_abstractions/sql_query.h",
        "include/sql_abstractions/sql_query_impl_base.h",
        "include/sql_abstractions/sql_query_impl_null.h",
        "include/sql_abstractions/sql_query_impl_pq.h",
        "include/sql_abstractions/sql_query_impl_sqlite.h",
        "include/sql_abstractions/sql_transaction.h",
        "include/sql_abstractions/sql_variant.h",
        "include/sql_abstractions/string_hasher.h",
        "include/sql_abstractions/string_streamer.h",
        "include/sql_abstractions/string_trimmer.h",
        "include/tasks/author_cache_reprocessor.h",
        "include/tasks/author_task_processor.h",
        "include/tasks/fandom_task_processor.h",
        "src/Interfaces/fandom_lists.cpp",
        "src/Interfaces/pagetask_interface.cpp",
        "src/Interfaces/tags.cpp",
        "src/core/fandom.cpp",
        "src/core/fanfic.cpp",
        "src/core/fav_list_details.cpp",
        "src/discord/actions.cpp",
        "src/discord/client_v2.cpp",
        "src/discord/command_controller.cpp",
        "src/discord/command_generators.cpp",
        "src/discord/db_vendor.cpp",
        "src/discord/discord_ffn_page.cpp",
        "src/discord/discord_init.cpp",
        "src/discord/discord_pagegetter.cpp",
        "src/discord/discord_server.cpp",
        "src/discord/discord_user.cpp",
        "src/discord/favourites_fetching.cpp",
        "src/discord/fetch_filters.cpp",
        "src/discord/help_generator.cpp",
        "src/discord/task_environment.cpp",
        "src/discord/task_runner.cpp",
        "src/environment.cpp",
        "src/main_discord.cpp",
        "include/grpc/grpc_source.h",
        "src/grpc/grpc_source.cpp",
        "include/url_utils.h",
        "src/page_utils.cpp",
        "src/pageconsumer.cpp",
        "src/parsers/ffn/desktop_favparser.cpp",
        "src/parsers/ffn/discord/discord_mobile_favparser.cpp",
        "src/parsers/ffn/fandomparser.cpp",
        "src/parsers/ffn/favparser_wrapper.cpp",
        "src/parsers/ffn/mobile_favparser.cpp",
        "src/servers/token_processing.cpp",
        "src/sql/discord/discord_queries.cpp",
        "src/sql_abstractions/sql_context.cpp",
        "src/sql_abstractions/sql_database.cpp",
        "src/sql_abstractions/sql_database_impl_base.cpp",
        "src/sql_abstractions/sql_database_impl_null.cpp",
        "src/sql_abstractions/sql_database_impl_pq.cpp",
        "src/sql_abstractions/sql_database_impl_sqlite.cpp",
        "src/sql_abstractions/sql_query.cpp",
        "src/sql_abstractions/sql_query_impl_null.cpp",
        "src/sql_abstractions/sql_query_impl_pq.cpp",
        "src/sql_abstractions/sql_query_impl_sqlite.cpp",
        "src/sql_abstractions/sql_transaction.cpp",
        "src/sql_abstractions/sql_variant.cpp",
        "src/tasks/author_cache_reprocessor.cpp",
        "src/tasks/author_task_processor.cpp",
        "src/tasks/fandom_task_processor.cpp",
        "src/url_utils.cpp",
        "src/storyfilter.cpp",
        "include/storyfilter.h",
        "include/querybuilder.h",
        "src/querybuilder.cpp",
        "src/pure_sql.cpp",
        "include/pure_sql.h",
        "src/sqlcontext.cpp",
        "src/regex_utils.cpp",
        "src/rng.cpp",
        "include/rng.h",
        "src/core/section.cpp",
        "include/core/section.h",
        "src/transaction.cpp",
        "include/transaction.h",
        "include/in_tag_accessor.h",
        "src/in_tag_accessor.cpp",
        "src/pagetask.cpp",
        "include/pagetask.h",
        "src/sqlitefunctions.cpp",
        "include/timeutils.h",
        "include/parsers/ffn/ffnparserbase.h",
        "src/parsers/ffn/ffnparserbase.cpp",
        "include/Interfaces/data_source.h",
        "src/Interfaces/data_source.cpp",
        "include/Interfaces/base.h",
        "include/Interfaces/fandoms.h",
        "src/Interfaces/fandoms.cpp",
        "include/Interfaces/fanfics.h",
        "include/Interfaces/ffn/ffn_fanfics.h",
        "src/Interfaces/fanfics.cpp",
        "src/Interfaces/ffn/ffn_fanfics.cpp",
        "include/Interfaces/ffn/ffn_authors.h",
        "include/Interfaces/db_interface.h",
        "include/Interfaces/interface_sqlite.h",
        "include/Interfaces/recommendation_lists.h",
        "src/Interfaces/authors.cpp",
        "src/Interfaces/base.cpp",
        "src/Interfaces/genres.cpp",
        "include/Interfaces/genres.h",
        "src/Interfaces/db_interface.cpp",
        "src/Interfaces/ffn/ffn_authors.cpp",
        "src/Interfaces/interface_sqlite.cpp",
        "src/Interfaces/recommendation_lists.cpp",
        "src/pagegetter.cpp",
        "include/pagegetter.h",
        "src/Interfaces/discord/users.cpp",
        "include/Interfaces/discord/users.h",
        "include/core/author.h",
        "src/core/author.cpp",
        "include/core/recommendation_list.h",
        "src/core/recommendation_list.cpp",
    ]
    cpp.defines: base.concat(["FMT_HEADER_ONLY"])
    Group{
    name: "sqlite"
    files: [
        Environment.sqliteFolder + "/sqlite3.c",
        Environment.sqliteFolder + "/sqlite3.h"
    ]
    cpp.cFlags: {
        var flags = []
        flags = [ "-Wno-unused-variable", "-Wno-unused-parameter", "-Wno-cast-function-type", "-Wno-implicit-fallthrough"]
        return flags
    }
    }

    Group{
    name: "nanobench"
    files: [
        "third_party/nanobench/nanobench.cpp",
        "third_party/nanobench/nanobench.h"
    ]
    cpp.cFlags: {
        var flags = []
        flags = [ "-Wno-unused-variable", "-Wno-unused-parameter", "-Wno-cast-function-type", "-Wno-implicit-fallthrough"]
        return flags
    }
    }

    consoleApplication:false
    type:"application"
    qbsSearchPaths: [sourceDirectory + "/modules", sourceDirectory + "/repo_modules"]

    cpp.includePaths: [
        sourceDirectory,
        sourceDirectory + "/../",
        sourceDirectory + "/include",
        sourceDirectory + "/libs",
        sourceDirectory + "/third_party/zlib",
        sourceDirectory + "/libs/Logger/include",
        sourceDirectory + "/third_party",
        sourceDirectory + "/third_party/fmt/include",
        sourceDirectory + "/third_party/sleepy-discord/include",
        sourceDirectory + "/third_party/sleepy-discord/deps",
        sourceDirectory + "/third_party/sleepy-discord/deps/asio/asio/include",
        sourceDirectory + "/third_party/sleepy-discord/deps/websocketpp",
        sourceDirectory + "/third_party/sleepy-discord/deps/cpr/include",
        Environment.sqliteFolder,
    ]

    cpp.systemIncludePaths: [
        sourceDirectory +"/proto",
        sourceDirectory + "/third_party",
        sourceDirectory + "/third_party/sqlite3",
        "/home/zeks/grpc/third_party/protobuf/src/",
        sourceDirectory + "/../"]

    cpp.staticLibraries: {
        var libs = []
        libs = ["dl", "protobuf", "sleepy-discord", "cpr", "curl", "crypto", "ssl", "pqxx-7.3", "pq"]
        libs = libs.concat(["grpc", "grpc++", "gpr"])
        return libs
    }


    Group{
        name:"grpc files"
        proto_generation.rootDir: {
            var result = project.rootFolder  + "/proto"
//            /console.error(project.rootFolder);
            return result
        }
        grpc_generation.rootDir: project.rootFolder  + "/proto"
        proto_generation.protobufDependencyDir: project.rootFolder  + "../"
        grpc_generation.protobufDependencyDir: project.rootFolder  + "../"
        proto_generation.toolchain : qbs.toolchain
        grpc_generation.toolchain : qbs.toolchain
        files: [
            "proto/feeder_service.proto",
        ]
        fileTags: ["grpc", "proto"]
    }
    Group{
        name:"proto files"
        proto_generation.rootDir: {
            var result = project.rootFolder  + "/proto"
            //console.error(project.rootFolder);
            return result
        }
        proto_generation.protobufDependencyDir: project.rootFolder  + "../"
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
