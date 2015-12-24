import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as Application

Project {
    name: "ffsse"
    references: [
        "ffsse.qbs",
        "libs/UniversalModels/UniversalModels.qbs",
        "libs/Logger/logger.qbs",
    ]
}
