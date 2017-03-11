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
        "include/fandomparser.h",
        "include/favparser.h",
        "include/init_database.h",
        "include/mainwindow.h",
        "include/pagegetter.h",
        "include/parse.h",
        "include/section.h",
        "include/tagwidget.h",
        "include/fanficdisplay.h",
        "qml_ficmodel.cpp",
        "qml_ficmodel.h",
        "src/fandomparser.cpp",
        "src/favparser.cpp",
        "src/init_database.cpp",
        "src/pagegetter.cpp",
        "ui/mainwindow.ui",
        "ui/tagwidget.ui",
        "ui/fanficdisplay.ui",
        "src/main.cpp",
        "src/mainwindow.cpp",
        "src/tagwidget.cpp",
        "src/fanficdisplay.cpp",
    ]

cpp.staticLibraries: ["UniversalModels", "logger", "zlib", "quazip"]
}
