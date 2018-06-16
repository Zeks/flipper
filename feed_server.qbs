import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as App
import "Precompiled.qbs" as Precompiled


App{
    name: "feed_server"
    consoleApplication:false
    type:"application"
    qbsSearchPaths: [sourceDirectory + "/modules", sourceDirectory + "/repo_modules"]
    Depends { name: "Qt.core"}
    Depends { name: "Qt.sql" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.concurrent" }
    Depends { name: "cpp" }
    Depends { name: "logger" }
    Depends { name: "proto_generation" }
    Depends { name: "grpc_generation" }
    Depends { name: "grpc_generation" }
    Depends { name: "projecttype" }

    Precompiled{condition:conditionals.usePrecompiledHeader}
    cpp.minimumWindowsVersion: "6.0"

    cpp.defines: base.concat(["L_LOGGER_LIBRARY"])
    cpp.includePaths: [
        sourceDirectory,
        sourceDirectory + "/../",
        sourceDirectory + "/include",
        sourceDirectory + "/libs",
        sourceDirectory + "/third_party/zlib",
        sourceDirectory + "/libs/Logger/include",
    ]

    files: [
        "include/grpc/grpc_source.h",
        "include/Interfaces/data_source.h",
        "src/grpc/grpc_source.cpp",
        "src/Interfaces/data_source.cpp",
        "include/Interfaces/base.h",
        "include/Interfaces/genres.h",
        "include/Interfaces/db_interface.h",
        "include/Interfaces/interface_sqlite.h",
        "include/Interfaces/recommendation_lists.h",
        "src/Interfaces/authors.cpp",
        "src/Interfaces/base.cpp",
        "src/Interfaces/genres.cpp",
        "src/Interfaces/db_interface.cpp",
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
        "third_party/sqlite/sqlite3.c",
        "third_party/sqlite/sqlite3.h",
        "src/sqlcontext.cpp",
        "src/main_feed_server.cpp",
        "src/pure_sql.cpp",
        "src/querybuilder.cpp",
        "src/regex_utils.cpp",
        "src/core/section.cpp",
        "src/sqlitefunctions.cpp",
        "src/storyfilter.cpp",
        "src/url_utils.cpp",
        "src/feeder_environment.cpp",
        "include/feeder_environment.h",
        "src/transaction.cpp",
        "include/transaction.h",
        "src/pagetask.cpp",
        "include/pagetask.h",
    ]

    cpp.staticLibraries: {
        var libs = ["UniversalModels", "logger", "quazip"]
        libs = libs.concat(conditionals.zlib)
        if(qbs.toolchain.contains("msvc"))
            libs = libs.concat(["User32","Ws2_32", "gdi32", "Advapi32"])
        if(conditionals.grpc)
            libs = libs.concat([conditionals.protobufName,"grpc", "grpc++", "gpr"])
        return libs
    }

    Group{
        name:"grpc files"
        proto_generation.rootDir: conditionals.projectPath + "/proto"
        grpc_generation.rootDir: conditionals.projectPath + "/proto"
        proto_generation.protobufDependencyDir: conditionals.projectPath + "../"
        grpc_generation.protobufDependencyDir: conditionals.projectPath + "../"
        proto_generation.toolchain : qbs.toolchain
        grpc_generation.toolchain : qbs.toolchain
        files: [
            "proto/feeder_service.proto",
        ]
        fileTags: ["grpc", "proto"]
    }
    Group{
        name:"proto files"
        proto_generation.rootDir: conditionals.projectPath + "/proto"
        proto_generation.protobufDependencyDir: conditionals.projectPath + "../"
        proto_generation.toolchain : qbs.toolchain
        files: [
            "proto/filter.proto",
            "proto/fanfic.proto",
        ]
        fileTags: ["proto"]
    }
}
