import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as App


App{
    name: "servitor"
    consoleApplication:false
    type:"application"
    qbsSearchPaths: sourceDirectory + "/modules"
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
    Depends { name: "logger" }

    cpp.defines: base.concat(["L_TREE_CONTROLLER_LIBRARY", "L_LOGGER_LIBRARY"])
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
        "UI/servitorwindow.ui",
        "include/core/fic_genre_data.h",
        "include/servitorwindow.h",
        "src/main_servitor.cpp",
        "src/servitorwindow.cpp",
        "include/Interfaces/data_source.h",
        "src/Interfaces/data_source.cpp",
        "include/Interfaces/base.h",
        "include/Interfaces/genres.h",
        "include/Interfaces/fandoms.h",
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
        "third_party/sqlite3/sqlite3.c",
        "third_party/sqlite3/sqlite3.h",
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
        "src/parsers/ffn/ffnparserbase.cpp",
        "include/parsers/ffn/ffnparserbase.h",
        "include/page_utils.h",
        "src/page_utils.cpp",
    ]

    cpp.staticLibraries: {
        //var libs = ["UniversalModels", "logger", "quazip"]
        var libs = []
        if(qbs.toolchain.contains("msvc"))
            libs = ["logger"]
        else{
            libs = ["logger", "dl", "protobuf"]
        }
        libs = libs.concat(conditionals.zlib)
        libs = libs.concat(conditionals.ssl)

        if(qbs.toolchain.contains("msvc"))
            libs = libs.concat(["User32","Ws2_32", "gdi32", "Advapi32"])
        if(qbs.toolchain.contains("msvc"))
            libs = libs.concat([conditionals.protobufName,"grpc", "grpc++", "gpr"])
        else
            libs = libs.concat(["grpc", "grpc++", "gpr"])
        return libs
    }
}
