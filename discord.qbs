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
    Depends { name: "Qt.concurrent" }
    Depends { name: "cpp" }
    Depends { name: "logger" }
    Depends { name: "proto_generation" }
    Depends { name: "grpc_generation" }

    files: [
        "include/core/db_entity.h",
        "include/core/fanfic.h",
        "include/core/identity.h",
        "include/core/slash_data.h",
        "include/discord/client.h",
        "include/discord/limits.h",
        "include/discord/task.h",
        "include/sql/discord/discord_queries.h",
        "src/discord/client.cpp",
        "src/discord/limits.cpp",
        "src/discord/task.cpp",
        "src/main_discord.cpp",
        "include/grpc/grpc_source.h",
        "src/grpc/grpc_source.cpp",
        "include/url_utils.h",
        "src/sql/discord/discord_queries.cpp",
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
        "include/parsers/ffn/favparser.h",
        "include/parsers/ffn/ffnparserbase.h",
        "src/parsers/ffn/favparser.cpp",
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
        sourceDirectory + "/third_party/sleepy-discord/include",
        sourceDirectory + "/third_party/sleepy-discord/deps",
        sourceDirectory + "/third_party/sleepy-discord/deps/asio/asio/include",
        sourceDirectory + "/third_party/sleepy-discord/deps/websocketpp",
        sourceDirectory + "/third_party/sleepy-discord/deps/cpr/include",
    ]

    cpp.systemIncludePaths: [
        sourceDirectory +"/proto",
        sourceDirectory + "/third_party",
        sourceDirectory + "/third_party/sqlite3",
        "/home/zeks/grpc/third_party/protobuf/src/",
        sourceDirectory + "/../"]

    cpp.staticLibraries: {
        var libs = []
        
        libs = ["logger","dl", "protobuf", "sleepy-discord", "cpr", "curl", "crypto", "ssl", "pthread"]
        libs = libs.concat(localvariables.zlib)
        libs = libs.concat(localvariables.ssl)
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
            "proto/server_base_structs.proto",
        ]
        fileTags: ["proto"]
    }

}
