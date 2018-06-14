import qbs
import qbs.File
import "BuildHelpers.js" as Funcs

Probe {
    // Input
    property string sourceDirectory

    // Output
    property string hash
    property string tag
    property string commitdate

    property var hack: File.lastModified(sourceDirectory + "/../.git/logs/HEAD") //A bit of a hack to make qbs re-resolve (see QBS-996)

    configure: {
        hash = Funcs.gitHash(sourceDirectory)
        console.error("git hash:" + hash)

        tag = Funcs.gitTag(sourceDirectory)
        console.error("git tag:" + tag)

        commitdate = Funcs.gitDate(sourceDirectory, hash)
        console.error("git date:" + commitdate)
    }
}
