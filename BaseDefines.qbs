import qbs
import qbs.Process
import qbs.Environment

Product{
    cpp.debugInformation: true
    cpp.cxxLanguageVersion: "c++17"

    cpp.cxxFlags: ["-Wno-unused-function"]
    destinationDirectory: {
        var path = project.rootFolder
        path += qbs.buildVariant == "release" ? "/release" : "/debug"
        return path
    }
    cpp.libraryPaths:  {
        env = Environment.getEnv("LD_LIBRARY_PATH").split(':')
        console.error(env);
        return env
    }
    cpp.compilerWrapper: {
            return ["ccache"]
    }
    cpp.defines: ["MAJOR_PROTOCOL_VERSION=2", "MINOR_PROTOCOL_VERSION=0","NONEXISTENT_OPUS"]
}
