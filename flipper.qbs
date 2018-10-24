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

    Precompiled{condition:conditionals.usePrecompiledHeader}

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
        "include/Interfaces/fandoms.h",
        "include/Interfaces/fanfics.h",
        "include/Interfaces/ffn/ffn_fanfics.h",
        "include/Interfaces/ffn/ffn_authors.h",
        "include/Interfaces/genres.h",
        "include/Interfaces/interface_sqlite.h",
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
        "include/pagegetter.h",
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
        "src/tasks/author_cache_reprocessor.cpp",
        "src/tasks/fandom_task_processor.cpp",
        "src/tasks/author_task_processor.cpp",
        "src/pageconsumer.cpp",
        "third_party/sqlite3/sqlite3.c",
        "third_party/sqlite3/sqlite3.h",
        "src/Interfaces/authors.cpp",
        "src/Interfaces/base.cpp",
        "src/Interfaces/db_interface.cpp",
        "src/Interfaces/fandoms.cpp",
        "src/Interfaces/fanfics.cpp",
        "src/Interfaces/ffn/ffn_fanfics.cpp",
        "src/Interfaces/ffn/ffn_authors.cpp",
        "src/Interfaces/genres.cpp",
        "src/Interfaces/interface_sqlite.cpp",
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
        "src/pure_sql.cpp",
        "src/querybuilder.cpp",
        "src/regex_utils.cpp",
        "src/core/section.cpp",
        "src/service_functions.cpp",
        "src/sqlitefunctions.cpp",
        "src/storyfilter.cpp",
        "src/transaction.cpp",
        "src/url_utils.cpp",
        "ui/mainwindow.ui",
        "ui/tagwidget.ui",
        "ui/fanficdisplay.ui",
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
    //cpp.dynamicLibraries: ["zlib"]
    cpp.staticLibraries: {
        var libs = ["UniversalModels", "logger", "quazip"]
        libs = libs.concat(conditionals.zlib)
        libs = libs.concat(conditionals.ssl)
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
            "proto/fandom.proto",
        ]
        fileTags: ["proto"]
    }
}
