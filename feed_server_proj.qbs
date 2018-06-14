import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as Application

Project {
    name: "feed_proj"
    references: [
        "feed_server.qbs",
        "core_condition.qbs",
        "libs/Logger/logger.qbs",
    ]
}
