import qbs 1.0
import qbs.Process
import "../../BaseDefines.qbs" as Library


Library{
name: "logger"
type:"staticlibrary"
Depends { name: "Qt.core"}
Depends { name: "cpp" }
Depends { name: "conditionals" }

qbsSearchPaths: sourceDirectory + "/../../modules"

cpp.defines: base.concat(["L_LOGGER_LIBRARY"])
cpp.includePaths: [
                "../",
                "../../",
                "include/logger",
                "include"
]

files: [
        "include/logger/version/version.h",
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
        "include/logger/QsLogger.h",
    ]
    //destinationDirectory: "../../build"
}
