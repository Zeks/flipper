import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as App


App{
name: "ffsse_app"
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

cpp.defines: base.concat(["L_TREE_CONTROLLER_LIBRARY"])
cpp.includePaths: [
                sourceDirectory,
                sourceDirectory + "/include",
                sourceDirectory + "/libs",

]

files: [
//        "debug/Ficform.qml",
//        "debug/Funcs.js",
//        "debug/HighlightedListview.qml",
//        "debug/TagCloud.qml",
//        "debug/ficview.qml",
        "forms.qrc",
        "icons.qrc",
        "include/mainwindow.h",
        "include/tagwidget.h",
        "include/genericeventfilter.h",
        "include/fanficdisplay.h",
        "qml_ficmodel.cpp",
        "qml_ficmodel.h",
        "ui/mainwindow.ui",
        "ui/tagwidget.ui",
        "ui/fanficdisplay.ui",
        "src/main.cpp",
        "src/mainwindow.cpp",
        "src/tagwidget.cpp",
        "src/genericeventfilter.cpp",
        "src/fanficdisplay.cpp",
    ]

cpp.dynamicLibraries: ["UniversalModels", "logger"]

}