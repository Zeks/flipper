import qbs
import qbs.Process
import qbs.Environment

Product{
    cpp.debugInformation: true
    cpp.cxxLanguageVersion: "c++17"

    // I literally can't do anything about deprecated copy warning within sleepy-discord
    // have to shitcode here to see actual relevant warnings in my code
    // and not a shitton of deprecations warnings I can do nothing about
    cpp.cxxFlags: ["-Wno-unused-function", "-Wno-deprecated-copy"]
    //cpp.cxxFlags: ["-Wno-unused-function"]
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
