import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as App


App{
name: "ffsse"
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

cpp.defines: base.concat(["L_TREE_CONTROLLER_LIBRARY", "L_LOGGER_LIBRARY"])
cpp.includePaths: [
                sourceDirectory,
                sourceDirectory + "/include",
                sourceDirectory + "/libs",
                sourceDirectory + "/zlib",
                sourceDirectory + "/libs/Logger/include",


]

files: [
        "forms.qrc",
        "icons.qrc",
        "include/db.h",
        "include/db_ffn.h",
        "include/fandomparser.h",
        "include/favparser.h",
        "include/ffnparserbase.h",
        "include/ficparser.h",
        "include/mainwindow.h",
        "include/pagegetter.h",
        "include/parse.h",
        "include/querybuilder.h",
        "include/queryinterfaces.h",
        "include/section.h",
        "include/service_functions.h",
        "include/storyfilter.h",
        "include/tagwidget.h",
        "include/fanficdisplay.h",
        "include/url_utils.h",
        "qml_ficmodel.cpp",
        "qml_ficmodel.h",
        "sqlite/sqlite3.c",
        "sqlite/sqlite3.h",
        "src/db.cpp",
        "src/db_ffn.cpp",
        "src/fandomparser.cpp",
        "src/favparser.cpp",
        "src/ffnparserbase.cpp",
        "src/ficparser.cpp",
        "src/main_ffsse.cpp",
        "src/pagegetter.cpp",
        "src/querybuilder.cpp",
        "src/regex_utils.cpp",
        "src/section.cpp",
        "src/service_functions.cpp",
        "src/storyfilter.cpp",
        "src/url_utils.cpp",
        "ui/mainwindow.ui",
        "ui/tagwidget.ui",
        "ui/fanficdisplay.ui",
        "src/mainwindow.cpp",
        "src/tagwidget.cpp",
        "src/fanficdisplay.cpp",
    ]

cpp.staticLibraries: ["UniversalModels", "logger", "zlib", "quazip"]
}
