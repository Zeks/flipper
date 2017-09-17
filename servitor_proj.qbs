import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as Application

Project {
    name: "servitor_proj"
    references: [
        "servitor.qbs",
        "libs/Logger/logger.qbs",
    ]
}
