import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as Application

Project {
    name: "servitor_proj"
    references: [
        "servitor.qbs",
        "gui_condition.qbs",
        "libs/Logger/logger.qbs",
    ]
}
