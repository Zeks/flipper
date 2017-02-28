import qbs 1.0
import qbs.Process
import "../../BaseDefines.qbs" as Library


Library{
name: "UniversalModels"
type:"staticlibrary"
qbsSearchPaths: sourceDirectory + "/../../modules"
Depends { name: "conditionals" }
Depends { name: "Qt.core"}
Depends { name: "Qt.sql" }
Depends { name: "Qt.widgets" }
Depends { name: "cpp" }

cpp.defines: base.concat(["L_TREE_CONTROLLER_LIBRARY"])
cpp.includePaths: [
                sourceDirectory,
                sourceDirectory + "/include",
                "../../libs/Logger/include",
]

files: [
        "include/TreeModel.h",
        "include/TreeItemInterface.h",
        "include/l_tree_controller_global.h",
        "include/AdaptingTableModel.h",
        "include/TreeItem.h",
        "include/TableDataInterface.h",
        "include/TableDataListHolder.h",
        "include/ItemController.h",
        "include/treeviewfunctions.h",
        "include/treeviewtemplatefunctions.h",
        "include/treeitemfunctions.h",
        "include/genericeventfilter.h",
        "src/TreeModel.cpp",
        "src/TreeItemInterface.cpp",
        "src/InterfaceAwareTreeData.cpp",
        "src/AddressTemplateData.cpp",
        "src/AddressTemplateTreeData.cpp",
        "src/AdaptingTableModel.cpp",
        "src/TableDataInterface.cpp",
        "src/AdaptingTableModelPrivate.cpp",
        "src/treeviewfunctions.cpp",
        "src/treeviewtemplatefunctions.cpp",
        "src/genericeventfilter.cpp",
    ]

cpp.dynamicLibraries: ["logger"]
Depends { name: "logger" }
//destinationDirectory: "../../build"
}
