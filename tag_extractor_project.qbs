import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as Application

Project {
    name: "TagExtractor_proj"
    references: [
        "tag_extractor.qbs",
        "libs/Logger/logger.qbs",
    ]
}
