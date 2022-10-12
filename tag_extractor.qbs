import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as App


App{
name: "TagExtractor"
consoleApplication:true
type:"application"
qbsSearchPaths: sourceDirectory + "/modules"
Depends { name: "Qt.core"}
Depends { name: "Qt.sql" }
Depends { name: "cpp" }


cpp.defines: base.concat([])
cpp.includePaths: [
                sourceDirectory,
                sourceDirectory + "/include",
                sourceDirectory + "/libs",
                sourceDirectory + "/third_party/zlib",
                sourceDirectory + "/libs/Logger/include",
]

files: [
        
        "include/Interfaces/db_interface.h",
        "include/Interfaces/fanfics.h",
        "include/Interfaces/ffn/ffn_fanfics.h",
        "include/Interfaces/tags.h",
        
        "include/pure_sql.h",
        "include/queryinterfaces.h",
        "include/regex_utils.h",
        "include/core/section.h",
        "include/transaction.h",
        "include/sqlitefunctions.h",
        "include/url_utils.h",
        "include/pagetask.h",
        "third_party/sqlite/sqlite3.c",
        "third_party/sqlite/sqlite3.h",
        
        "src/Interfaces/db_interface.cpp",
        "src/Interfaces/tags.cpp",
        "src/pure_sql.cpp",
        "src/regex_utils.cpp",
        "src/core/section.cpp",
        "src/sqlitefunctions.cpp",
        "src/url_utils.cpp",
        "src/transaction.cpp",
        "src/pagetask.cpp",
        "src/main_tag_extractor.cpp",
        
    ]

cpp.staticLibraries: ["zlib", "quazip"]
}
