import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as Application

Project {
    name: "discord_proj"
    references: [
        "discord.qbs",
        "core_condition.qbs",
        "libs/Logger/logger.qbs",
    ]
}
