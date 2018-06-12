import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as App


App{
    name: "flipper"
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
    Depends { name: "UniversalModels" }
    Depends { name: "logger" }

    cpp.defines: base.concat(["L_TREE_CONTROLLER_LIBRARY", "L_LOGGER_LIBRARY",
                              //"QT_QML_DEBUG"
                              // ,"QT_LOGGING_TO_CONSOLE=1"
                             ])
    cpp.includePaths: [
        sourceDirectory,
        sourceDirectory + "/include",
        sourceDirectory + "/libs",
        sourceDirectory + "/third_party/zlib",
        sourceDirectory + "/libs/Logger/include",


    ]

    files: [
        "forms.qrc",
        "icons.qrc",
        "include/Interfaces/authors.h",
        "include/Interfaces/base.h",
        "include/Interfaces/db_interface.h",
        "include/Interfaces/fandoms.h",
        "include/Interfaces/fanfics.h",
        "include/Interfaces/ffn/ffn_authors.h",
        "include/Interfaces/ffn/ffn_fanfics.h",
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
        "src/tasks/fandom_task_processor.cpp",
        "src/tasks/author_task_processor.cpp",
        "src/pageconsumer.cpp",
        "third_party/sqlite/sqlite3.c",
        "third_party/sqlite/sqlite3.h",
        "src/Interfaces/authors.cpp",
        "src/Interfaces/base.cpp",
        "src/Interfaces/db_interface.cpp",
        "src/Interfaces/fandoms.cpp",
        "src/Interfaces/fanfics.cpp",
        "src/Interfaces/ffn/ffn_authors.cpp",
        "src/Interfaces/ffn/ffn_fanfics.cpp",
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
    ]

    cpp.staticLibraries: ["UniversalModels", "logger", "zlib", "quazip"]
}
