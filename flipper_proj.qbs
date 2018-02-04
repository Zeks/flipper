import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as Application

Project {
    name: "flipper_proj"
    references: [
        "flipper.qbs",
        "libs/UniversalModels/UniversalModels.qbs",
        "libs/Logger/logger.qbs",
    ]
}
