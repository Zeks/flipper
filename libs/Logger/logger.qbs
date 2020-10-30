import "../../BaseDefines.qbs" as Library
import "../../Precompiled.qbs" as Precompiled
Library{
    qbsSearchPaths: sourceDirectory + "/../../modules"
    type:  "staticlibrary"
    Depends { name: "Qt.core"}
    Depends { name: "cpp" }
    Depends { name: "projecttype"}
    Depends { name: "Environment"}
    Depends { name: "Qt.sql"}
    Depends { name: "Qt.concurrent"}
    Depends {
        name: "Qt.widgets"
        condition: projecttype.useGuiLib
    }
    Depends {
        name: "Qt.gui"
        condition: projecttype.useGuiLib
    }
    Precompiled{condition:Environment.usePrecompiledHeader}
    Export{
        Depends { name: "cpp" }
        cpp.includePaths: [product.sourceDirectory + "/include"]
    }

    cpp.defines: base.concat(["L_LOGGER_LIBRARY"])
    cpp.includePaths: [
        "../",
        "../../",
        "include/logger",
        "include"
    ]

    files: [
        "src/QsLog.cpp",
        "src/QsLogDest.cpp",
        "src/QsLogDestConsole.cpp",
        "src/QsLogDestFile.cpp",
        "include/logger/l_logger_global.h",
        "include/logger/QsLog.h",
        "include/logger/QsLogDest.h",
        "include/logger/QsLogDestConsole.h",
        "include/logger/QsLogDestFile.h",
        "include/logger/QsLogDisableForThisFile.h",
        "include/logger/QsLogLevel.h",
        "include/logger/Tracer.h",
        "include/logger/QsLogger.h",
    ]

}
