import qbs 1.0
import qbs.Process
import qbs.File
import qbs.Environment
import "BaseDefines.qbs" as Application

Project {
    name: "discord_proj"

    qbsSearchPaths: [sourceDirectory + "/modules", sourceDirectory + "/repo_modules"]
    property string rootFolder: {
        var rootFolder = File.canonicalFilePath(sourceDirectory).toString();
        console.error("Source:" + rootFolder)
        return rootFolder.toString()
    }
    property bool usePostgres: true
    property bool useWebview: false

    references: [
        "discord.qbs",
        "core_condition.qbs",
        "environment_plugs.qbs",
        "libs/Logger/logger.qbs",
        "libs/sql/sql.qbs",
    ]
}
